#include "acoustics.h"
#include "body.h"
#include "interaction.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define MAX_DISTANCE 12.0f
#define MIN_GAIN .008f

static const LCObject *find(const LCWorld *w,uint32_t id){
    for(size_t i=0;i<w->object_count;i++)if(w->objects[i].id==id)return &w->objects[i];
    return NULL;
}
static bool active(const LCObject *o,LCSoundKind kind){
    if(!o||o->container_id)return false;
    if(kind==LC_SOUND_APPLIANCE_HUM)return o->kind==LC_CONTAINER;
    if(kind==LC_SOUND_CRT_HUM)return o->kind==LC_DEVICE&&(o->flags&LC_OBJ_POWERED);
    return o->kind==LC_DOOR&&(o->flags&LC_OBJ_OPEN);
}
static LCVec emission(const LCObject *o){
    LCBox b=o->shape;
    LCVec p={(b.min.x+b.max.x)*.5f,(b.min.y+b.max.y)*.5f,b.max.z+.025f};
    switch(o->id){
    case 103:p.x=b.min.x-.025f;p.z=(b.min.z+b.max.z)*.5f;break;
    case 109:p.y=1.2f;p.z=.16f;break;
    case 110:p.y=1.2f;p.z=9.84f;break;
    default:break;
    }
    return p;
}
bool lc_audio_frame_at(uint64_t ms,uint64_t *frame){
    const uint64_t per_ms=LC_AUDIO_RATE/1000u;
    if(!frame||ms>UINT64_MAX/per_ms)return false;
    *frame=ms*per_ms;return true;
}
static unsigned duration_ms(LCSoundKind k){
    switch(k){
    case LC_SOUND_FOOTSTEP:return 140;
    case LC_SOUND_DOOR_CLICK:return 260;
    case LC_SOUND_SWITCH_CLICK:return 70;
    case LC_SOUND_CONTAINER_CLICK:return 180;
    default:return 0;
    }
}
void lc_audio_reset_transients(LCWorld *w){
    if(!w)return;
    memset(w->sound_events,0,sizeof(w->sound_events));
    w->sound_event_count=0;w->footstep_distance_m=0;
}
static void emit(LCWorld *w,LCSoundKind k,uint32_t id,LCVec p){
    if(!w||!duration_ms(k)||!isfinite(p.x)||!isfinite(p.y)||!isfinite(p.z)||
       w->sound_event_count>LC_SOUND_EVENT_CAPACITY)return;
    if(w->sound_event_count==LC_SOUND_EVENT_CAPACITY){
        memmove(w->sound_events,w->sound_events+1,
                (LC_SOUND_EVENT_CAPACITY-1)*sizeof(w->sound_events[0]));
        w->sound_event_count--;
    }
    w->sound_events[w->sound_event_count++]=(LCTransientSound){w->environment_elapsed_ms,id,k,p};
}
void lc_audio_note_walk(LCWorld *w,LCVec before){
    if(!w||w->posture!=LC_STANDING)return;
    LCVec p=w->player.position;
    float distance=hypotf(p.x-before.x,p.z-before.z);
    if(!isfinite(distance)||distance<=0||distance>3||
       !isfinite(w->footstep_distance_m)||w->footstep_distance_m<0||
       w->footstep_distance_m>=.65f)return;
    w->footstep_distance_m+=distance;
    if(w->footstep_distance_m>=.65f){
        w->footstep_distance_m=fmodf(w->footstep_distance_m,.65f);
        p.y+=.02f;
        emit(w,LC_SOUND_FOOTSTEP,0,p);
    }
}
void lc_audio_note_toggle(LCWorld *w,const LCObject *o){
    if(!w||!o||!o->id)return;
    LCSoundKind kind;
    switch(o->kind){
    case LC_DOOR:kind=LC_SOUND_DOOR_CLICK;break;
    case LC_CONTAINER:kind=LC_SOUND_CONTAINER_CLICK;break;
    case LC_DEVICE:kind=LC_SOUND_SWITCH_CLICK;break;
    default:return;
    }
    LCBox b=o->shape;
    LCVec p={(b.min.x+b.max.x)*.5f,(b.min.y+b.max.y)*.5f,(b.min.z+b.max.z)*.5f};
    emit(w,kind,o->id,p);
}
static void append_cue(const LCWorld *w,LCAudioSample *s,LCPose camera,
                       LCVec p,uint32_t id,LCSoundKind kind,float amplitude,
                       uint64_t start,uint32_t duration){
    LCVec ear=lc_eye(&camera),d={p.x-ear.x,p.y-ear.y,p.z-ear.z};
    float n=hypotf(hypotf(d.x,d.y),d.z);
    if(!isfinite(n)||n>MAX_DISTANCE||s->count>=LC_AUDIO_MAX_SOURCES)return;
    LCHit hit;
    bool blocked=n>.002f&&lc_cast(w,ear,d,n-.001f,&hit);
    /* Click sources are inside the object that emitted them; its first surface
     * is not an additional intervening obstruction. */
    if(blocked&&duration&&id&&hit.object_id==id)blocked=false;
    float gain=amplitude/(1.0f+n*n*.15f);
    if(blocked)gain*=.22f;
    if(gain<MIN_GAIN)return;
    float bearing=hypotf(d.x,d.z)>.0001f?
        remainderf(atan2f(d.x,-d.z)-camera.yaw,(float)(2*PI)):0;
    s->cues[s->count++]=(LCSoundCue){id,kind,bearing,n,gain,blocked,start,duration};
}
bool lc_audio_sample(const LCWorld *w,LCAudioSample *out){
    if(!out||!lc_body_valid(w)||w->sound_event_count>LC_SOUND_EVENT_CAPACITY)return false;
    LCAudioSample s={.elapsed_ms=w->environment_elapsed_ms};
    const struct {uint32_t id;LCSoundKind kind;float amplitude;} sources[]={
        {107,LC_SOUND_APPLIANCE_HUM,.22f},
        {103,LC_SOUND_CRT_HUM,.18f},
        {204,LC_SOUND_CRT_HUM,.18f},
        {109,LC_SOUND_DOORWAY_AIR,.32f},
        {110,LC_SOUND_DOORWAY_AIR,.32f}
    };
    LCPose camera=lc_body_camera_pose(w);
    for(size_t i=0;i<sizeof(sources)/sizeof(sources[0]);i++){
        const LCObject *o=find(w,sources[i].id);
        if(!active(o,sources[i].kind)||o->id==w->held_object_id)continue;
        append_cue(w,&s,camera,emission(o),o->id,sources[i].kind,sources[i].amplitude,0,0);
    }
    for(size_t i=0;i<w->sound_event_count;i++){
        const LCTransientSound *e=&w->sound_events[i];
        unsigned duration=duration_ms(e->kind);
        uint64_t start;
        if(!duration||e->started_ms>w->environment_elapsed_ms||
           w->environment_elapsed_ms-e->started_ms>=duration||
           !lc_audio_frame_at(e->started_ms,&start))continue;
        append_cue(w,&s,camera,e->position,e->source_id,e->kind,.30f,start,
                   duration*(LC_AUDIO_RATE/1000u));
    }
    *out=s;
    return true;
}
static uint32_t noise_hash(uint64_t frame,uint32_t id){
    uint32_t x=(uint32_t)frame^(uint32_t)(frame>>32)^(id*UINT32_C(2654435761));
    x^=x>>16;x*=UINT32_C(0x7feb352d);x^=x>>15;x*=UINT32_C(0x846ca68b);x^=x>>16;
    return x;
}
static double oscillator(uint64_t f,unsigned hz){
    /* Integer frequencies at a fixed sample rate: exact periodic reduction
     * avoids precision loss even after long-running sessions. */
    return sin(2*PI*(double)((f%LC_AUDIO_RATE)*hz%LC_AUDIO_RATE)/LC_AUDIO_RATE);
}
static double wave(const LCSoundCue *c,uint64_t frame){
    switch(c->kind){
    case LC_SOUND_APPLIANCE_HUM:return .75*oscillator(frame,60)+.25*oscillator(frame,120);
    case LC_SOUND_CRT_HUM:return .8*oscillator(frame,120)+.2*oscillator(frame,240);
    case LC_SOUND_DOORWAY_AIR:{
        /* Smooth interpolated deterministic noise, not recorded outdoor audio. */
        uint64_t k=frame/32;
        double a=(double)noise_hash(k,c->source_id)/UINT32_MAX*2-1;
        double b=(double)noise_hash(k+1,c->source_id)/UINT32_MAX*2-1;
        double t=(double)(frame%32)/32;
        return a+(b-a)*t;
    }
    case LC_SOUND_FOOTSTEP:case LC_SOUND_DOOR_CLICK:
    case LC_SOUND_SWITCH_CLICK:case LC_SOUND_CONTAINER_CLICK:{
        if(frame<c->start_frame||frame-c->start_frame>=c->duration_frames)return 0;
        uint64_t age=frame-c->start_frame;
        double t=(double)age/c->duration_frames;
        double envelope=fmin(1.0,age/16.0)*(1-t)*(1-t);
        double noise=(double)noise_hash(age/3,c->source_id+(uint32_t)c->kind)/UINT32_MAX*2-1;
        unsigned hz=c->kind==LC_SOUND_FOOTSTEP?85:c->kind==LC_SOUND_DOOR_CLICK?130:
                    c->kind==LC_SOUND_SWITCH_CLICK?650:180;
        return envelope*(.5*noise+.5*oscillator(age,hz));
    }
    default:return 0;
    }
}
static int16_t quantize(double a){
    if(a>1)a=1;
    if(a<-1)a=-1;
    return (int16_t)lround(a*32767);
}
bool lc_audio_render(const LCAudioSample *s,uint64_t first,size_t frames,int16_t *pcm,size_t count){
    if(!s||!pcm||s->count>LC_AUDIO_MAX_SOURCES||frames>LC_AUDIO_MAX_FRAMES||
       count<frames*LC_AUDIO_CHANNELS||(frames&&UINT64_MAX-first<frames-1))return false;
    double left[LC_AUDIO_MAX_SOURCES],right[LC_AUDIO_MAX_SOURCES];
    for(size_t j=0;j<s->count;j++){
        const LCSoundCue *c=&s->cues[j];
        if((!c->source_id&&c->kind!=LC_SOUND_FOOTSTEP)||c->kind<LC_SOUND_APPLIANCE_HUM||c->kind>LC_SOUND_CONTAINER_CLICK||
           (c->kind==LC_SOUND_FOOTSTEP&&c->source_id)||
           (c->kind<=LC_SOUND_DOORWAY_AIR&&c->duration_frames)||
           (c->kind>LC_SOUND_DOORWAY_AIR&&c->duration_frames!=duration_ms(c->kind)*(LC_AUDIO_RATE/1000u))||
           !isfinite(c->gain)||c->gain<0||c->gain>1||
           !isfinite(c->relative_bearing_rad)||fabsf(c->relative_bearing_rad)>(float)PI||
           !isfinite(c->distance_m)||c->distance_m<0)return false;
        double pan=sin(c->relative_bearing_rad);
        left[j]=c->gain*sqrt((1-pan)*.5);
        right[j]=c->gain*sqrt((1+pan)*.5);
    }
    for(size_t i=0;i<frames;i++){
        double l=0,r=0;
        for(size_t j=0;j<s->count;j++){
            double v=wave(&s->cues[j],first+i);
            l+=v*left[j];r+=v*right[j];
        }
        pcm[i*2]=quantize(l);pcm[i*2+1]=quantize(r);
    }
    return true;
}
