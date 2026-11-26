#include "session.h"
#include "body.h"
#include <stdlib.h>

struct LCSession {
    LCWorld world;
    uint64_t id,generation,revision;
};
static LCSessionStamp stamp(const LCSession *s){
    return (LCSessionStamp){s->id,s->generation,s->revision,s->world.environment_elapsed_ms};
}
LCSession *lc_session_create(uint64_t id){
    if(!id)return NULL;
    LCSession *s=calloc(1,sizeof(*s));
    if(!s)return NULL;
    lc_house(&s->world);
    if(!lc_body_valid(&s->world)){free(s);return NULL;}
    s->id=id;s->generation=1;
    return s;
}
void lc_session_destroy(LCSession *s){free(s);}
bool lc_session_stamp(const LCSession *s,LCSessionStamp *out){
    if(!s||!out)return false;
    *out=stamp(s);return true;
}
bool lc_session_execute(LCSession *s,const LCCommand *cmd,LCSessionReceipt *out){
    if(!s||!cmd||!out||s->revision==UINT64_MAX)return false;
    LCSessionReceipt r={.before=stamp(s)};
    LCCommandStatus status=lc_execute(&s->world,cmd,&r.command);
    if(status==LC_COMMAND_APPLIED)s->revision++;
    r.after=stamp(s);*out=r;
    return true;
}
bool lc_session_advance(LCSession *s,uint32_t ms){
    if(!s||(ms&&s->revision==UINT64_MAX)||!lc_environment_advance(&s->world,ms))return false;
    if(ms)s->revision++;
    return true;
}
bool lc_session_save(const LCSession *s,const char *path){return s&&lc_save(&s->world,path);}
bool lc_session_load(LCSession *s,const char *path){
    if(!s||s->revision==UINT64_MAX||s->generation==UINT64_MAX)return false;
    /* Load against fresh definitions: additions absent from an old snapshot
     * get their defaults, rather than state from the abandoned timeline. */
    LCWorld staged;
    lc_house(&staged);
    if(!lc_load(&staged,path))return false;
    s->world=staged;s->generation++;s->revision++;
    return true;
}
bool lc_session_capture(const LCSession *s,const LCCaptureRequest *q,LCCaptureResult *out){
    if(!s||!q||!out||!q->flags||(q->flags&~(LC_CAPTURE_RGB|LC_CAPTURE_AUDIO|LC_CAPTURE_ENVIRONMENT)))return false;
    if((q->flags&LC_CAPTURE_RGB)&&(!q->rgb||q->rgb_bytes<LC_FRAME_RGB_BYTES||
       !q->depth||q->depth_count<LC_FRAME_PIXELS))return false;
    if((q->flags&LC_CAPTURE_AUDIO)&&(!q->pcm||!q->audio_frames||
       q->audio_frames>LC_AUDIO_MAX_FRAMES||q->pcm_samples<q->audio_frames*LC_AUDIO_CHANNELS))return false;
    LCCaptureResult r={.stamp=stamp(s),.flags=q->flags};
    if((q->flags&LC_CAPTURE_ENVIRONMENT)&&!lc_environment_sample(&s->world,&r.environment))return false;
    if(q->flags&LC_CAPTURE_AUDIO){
        uint64_t first;
        if(!lc_audio_sample(&s->world,&r.acoustic)||!lc_audio_frame_at(r.stamp.elapsed_ms,&first)||
           !lc_audio_render(&r.acoustic,first,q->audio_frames,q->pcm,q->pcm_samples))return false;
        r.audio_rate=LC_AUDIO_RATE;r.audio_channels=LC_AUDIO_CHANNELS;r.audio_frames=q->audio_frames;
    }
    if(q->flags&LC_CAPTURE_RGB){
        if(!lc_frame_render(&s->world,q->rgb,q->rgb_bytes,q->depth,q->depth_count))return false;
        r.width=LC_VIEW_WIDTH;r.height=LC_VIEW_HEIGHT;
    }
    *out=r;return true;
}
