#include "command.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static const LCSoundCue *cue(const LCAudioSample *s,uint32_t id){
    for(size_t i=0;i<s->count;i++)if(s->cues[i].source_id==id)return &s->cues[i];
    return NULL;
}
int main(void){
    LCWorld w;lc_house(&w);LCAudioSample a,b;
    assert(lc_audio_sample(&w,&a)&&!cue(&a,103)&&!cue(&a,110));
    w.player=(LCPose){{5.2f,0,7.5f},0,0};
    LCObject *tv=lc_find_object(&w,103);assert(tv);
    tv->flags|=LC_OBJ_POWERED;
    assert(lc_audio_sample(&w,&a));
    const LCSoundCue *c=cue(&a,103);
    assert(c&&c->relative_bearing_rad>0&&c->distance_m<2&&!c->obstructed);
    float near_gain=c->gain;
    w.player.yaw=3.14159265358979323846f;
    assert(lc_audio_sample(&w,&b));
    c=cue(&b,103);assert(c&&c->relative_bearing_rad<0&&fabsf(c->gain-near_gain)<.00001f);
    w.player=(LCPose){{8,0,8.8f},0,0};
    assert(lc_audio_sample(&w,&b));
    c=cue(&b,103);assert(c&&c->gain<near_gain);
    w.player=(LCPose){{5.2f,0,7.5f},0,0};
    assert(lc_add_box(&w,(LCBox){{5.45f,.3f,6.9f},{5.55f,2,7.3f}}));
    assert(lc_audio_sample(&w,&b));
    c=cue(&b,103);assert(c&&c->obstructed&&c->gain<near_gain);
    lc_house(&w);w.player=(LCPose){{6,0,8.8f},0,0};
    assert(lc_audio_sample(&w,&a)&&!cue(&a,110));
    assert(lc_toggle_object(&w,110,w.player.position));
    assert(lc_audio_sample(&w,&b)&&cue(&b,110));
    assert(lc_toggle_object(&w,110,w.player.position));
    assert(lc_audio_sample(&w,&a)&&!cue(&a,110));
    LCCommand cmd={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_SENSE,.input.sense={LC_SENSE_AUDIO,0}};
    LCCommandReceipt r;
    assert(lc_environment_advance(&w,100));
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_OBSERVED&&r.has_audio_sample);
    assert(r.audio_sample.elapsed_ms==100&&w.environment_elapsed_ms==100);
    assert(!r.has_contact_sample&&!r.has_environment_sample);
    cmd.api_version=4;assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_INVALID&&!r.has_audio_sample);
    cmd.api_version=LC_COMMAND_API_VERSION;cmd.input.sense.expected_object_id=103;
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_INVALID&&!r.has_audio_sample);
    LCAudioSample tone={.count=1,.cues={{103,LC_SOUND_CRT_HUM,1.5707963f,1,.5f,false,0,0}}};
    int16_t pcm[320],part[320];
    assert(lc_audio_render(&tone,123,160,pcm,320));
    assert(lc_audio_render(&tone,123,80,part,160));
    assert(lc_audio_render(&tone,203,80,part+160,160));
    assert(memcmp(pcm,part,sizeof(pcm))==0);
    long l=0,rr=0;
    for(size_t i=0;i<160;i++){l+=(long)pcm[2*i]*pcm[2*i];rr+=(long)pcm[2*i+1]*pcm[2*i+1];}
    assert(rr>0&&l<rr/100);
    assert(!lc_audio_render(&tone,0,160,part,319));
    assert(!lc_audio_render(&tone,UINT64_MAX,2,part,4));
    tone.cues[0].gain=NAN;assert(!lc_audio_render(&tone,0,160,part,320));
    tone.count=0;assert(lc_audio_render(&tone,0,160,pcm,320));
    for(size_t i=0;i<320;i++)assert(pcm[i]==0);
    puts("Living Colors: spatial audio, obstruction, source state and stereo PCM tests passed");
    return 0;
}
