/* Local development console. Not an R2 protocol or network server. */
#include "session.h"
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t rgb[LC_FRAME_RGB_BYTES];
static float depth[LC_FRAME_PIXELS];
static int16_t pcm[LC_AUDIO_MAX_FRAMES*LC_AUDIO_CHANNELS];

static bool number(const char *s,uint32_t *out){
    if(!s||!*s)return false;
    for(const char *p=s;*p;p++)if(*p<'0'||*p>'9')return false;
    errno=0;char *end;uintmax_t n=strtoumax(s,&end,10);
    if(errno||*end||n>UINT32_MAX)return false;
    *out=(uint32_t)n;return true;
}
static bool real(const char *s,float *out){
    if(!s||!*s)return false;
    errno=0;char *end;float f=strtof(s,&end);
    if(errno||*end||!isfinite(f))return false;
    *out=f;return true;
}
static void report(const char *label,LCSessionStamp s){
    printf("%s generation=%" PRIu64 " revision=%" PRIu64 " ms=%" PRIu64 "\n",
           label,s.generation,s.revision,s.elapsed_ms);
}
static bool write_ppm(const char *path){
    FILE *f=fopen(path,"wb");
    if(!f)return false;
    bool ok=fprintf(f,"P6\n%u %u\n255\n",LC_VIEW_WIDTH,LC_VIEW_HEIGHT)>0&&
        fwrite(rgb,1,sizeof(rgb),f)==sizeof(rgb);
    if(fclose(f))ok=false;
    return ok;
}
static bool le(FILE *f,uint32_t n,unsigned bytes){
    for(unsigned i=0;i<bytes;i++)if(fputc((int)((n>>(8*i))&255u),f)==EOF)return false;
    return true;
}
static bool tag(FILE *f,const char *s){return fwrite(s,1,4,f)==4;}
static bool write_wav(const char *path,size_t frames){
    uint32_t bytes=(uint32_t)(frames*LC_AUDIO_CHANNELS*2);
    FILE *f=fopen(path,"wb");if(!f)return false;
    bool ok=tag(f,"RIFF")&&le(f,36+bytes,4)&&tag(f,"WAVE")&&tag(f,"fmt ")&&
        le(f,16,4)&&le(f,1,2)&&le(f,LC_AUDIO_CHANNELS,2)&&le(f,LC_AUDIO_RATE,4)&&
        le(f,LC_AUDIO_RATE*LC_AUDIO_CHANNELS*2,4)&&le(f,LC_AUDIO_CHANNELS*2,2)&&
        le(f,16,2)&&tag(f,"data")&&le(f,bytes,4);
    for(size_t i=0;ok&&i<frames*LC_AUDIO_CHANNELS;i++)ok=le(f,(uint16_t)pcm[i],2);
    if(fclose(f))ok=false;
    return ok;
}
/* Drain a full malformed line instead of treating its suffix as a command. */
static int line(char *buf,size_t cap){
    size_t n=0;bool bad=false;int c;
    while((c=getchar())!=EOF&&c!='\n'){
        if(c==0||n+1>=cap)bad=true;
        else buf[n++]=(char)c;
    }
    buf[n]=0;
    if(c==EOF&&!n&&!bad)return 0;
    return bad?-1:1;
}
static bool command(char **a,size_t n,LCCommand *cmd){
    *cmd=(LCCommand){.api_version=LC_COMMAND_API_VERSION};
    if(n==4&&!strcmp(a[0],"walk")){
        cmd->kind=LC_COMMAND_WALK;
        return real(a[1],&cmd->input.walk.forward)&&real(a[2],&cmd->input.walk.right)&&
               real(a[3],&cmd->input.walk.seconds);
    }
    if(n==3&&!strcmp(a[0],"look")){
        cmd->kind=LC_COMMAND_LOOK;
        return real(a[1],&cmd->input.look.yaw_delta)&&real(a[2],&cmd->input.look.pitch_delta);
    }
    if(n==3&&!strcmp(a[0],"use")){
        uint32_t id,action;
        if(!number(a[1],&id)||!number(a[2],&action)||action<1||action>LC_ACTION_STORE)return false;
        cmd->kind=LC_COMMAND_USE;cmd->input.use.object_id=id;cmd->input.use.action=(LCAction)action;
        return true;
    }
    if(n==3&&!strcmp(a[0],"sense")){
        uint32_t kind,id;
        if(!number(a[1],&kind)||!number(a[2],&id)||kind<1||kind>LC_SENSE_AUDIO)return false;
        cmd->kind=LC_COMMAND_SENSE;cmd->input.sense.kind=(LCSenseKind)kind;
        cmd->input.sense.expected_object_id=id;return true;
    }
    return false;
}
int main(int argc,char **argv){
    if(argc!=4){
        fprintf(stderr,"Usage: %s SNAPSHOT_PATH FRAME_PPM AUDIO_WAV\n",argv[0]);return 2;
    }
    if(!strcmp(argv[1],argv[2])||!strcmp(argv[1],argv[3])||!strcmp(argv[2],argv[3])){
        fputs("Output paths must differ\n",stderr);return 2;
    }
    LCSession *s=lc_session_create(1);
    if(!s)return 1;
    LCSessionStamp st;
    (void)lc_session_stamp(s,&st);report("READY",st);fflush(stdout);
    char buf[256];int read_status;
    while((read_status=line(buf,sizeof(buf)))!=0){
        if(read_status<0){puts("INVALID");fflush(stdout);continue;}
        char *a[5];size_t n=0;
        for(char *p=strtok(buf," \t\r");p&&n<5;p=strtok(NULL," \t\r"))a[n++]=p;
        if(!n)continue;
        if(n==1&&!strcmp(a[0],"quit"))break;
        if(n==1&&!strcmp(a[0],"save")){
            puts(lc_session_save(s,argv[1])?"SAVED":"ERROR save");
        }else if(n==1&&!strcmp(a[0],"load")){
            if(lc_session_load(s,argv[1])){(void)lc_session_stamp(s,&st);report("LOADED",st);}
            else puts("ERROR load");
        }else if(n==2&&!strcmp(a[0],"tick")){
            uint32_t ms;
            if(number(a[1],&ms)&&lc_session_advance(s,ms)){(void)lc_session_stamp(s,&st);report("TICK",st);}
            else puts("INVALID");
        }else if(n==1&&!strcmp(a[0],"capture")){
            LCCaptureRequest q={.flags=LC_CAPTURE_RGB|LC_CAPTURE_AUDIO|LC_CAPTURE_ENVIRONMENT,
                .rgb=rgb,.rgb_bytes=sizeof(rgb),.depth=depth,.depth_count=LC_FRAME_PIXELS,
                .pcm=pcm,.pcm_samples=sizeof(pcm)/sizeof(pcm[0]),.audio_frames=LC_AUDIO_MAX_FRAMES};
            LCCaptureResult r;
            if(lc_session_capture(s,&q,&r)&&write_ppm(argv[2])&&write_wav(argv[3],r.audio_frames))
                report("CAPTURED",r.stamp);
            else puts("ERROR capture");
        }else{
            LCCommand cmd;LCSessionReceipt r;
            if(!command(a,n,&cmd)||!lc_session_execute(s,&cmd,&r))puts("INVALID");
            else{
                printf("RESULT status=%u ",(unsigned)r.command.status);
                report("STATE",r.after);
                if(r.command.has_environment_sample){
                    LCEnvironmentSample e=r.command.environment_sample;
                    printf("ENV zone=%u temperature=%.3f air=%.3f light=%.3f\n",
                           (unsigned)e.zone,e.temperature_c,e.air_speed_mps,e.light_level);
                }
                if(r.command.has_contact_sample){
                    LCContactSample c=r.command.contact_sample;
                    printf("CONTACT object=%u material=%u known=%u\n",c.object_id,
                           (unsigned)c.surface.material,(unsigned)c.surface.known);
                }
                if(r.command.has_audio_sample)printf("AUDIO sources=%zu\n",r.command.audio_sample.count);
            }
        }
        fflush(stdout);
    }
    bool input_error=ferror(stdin)!=0;
    lc_session_destroy(s);return input_error?1:0;
}
