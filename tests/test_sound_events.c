#include "command.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const LCSoundCue *kind(const LCAudioSample *s,LCSoundKind k){
    for(size_t i=0;i<s->count;i++)if(s->cues[i].kind==k)return &s->cues[i];
    return NULL;
}
int main(void){
    LCWorld w;lc_house(&w);LCCommandReceipt r;LCAudioSample s;
    LCCommand toggle={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_USE,.input.use={110,LC_ACTION_TOGGLE}};
    assert(lc_execute(&w,&toggle,&r)==LC_COMMAND_REJECTED&&!w.sound_event_count);
    w.player.yaw=3.14159265f;
    assert(lc_environment_advance(&w,1000));
    assert(lc_execute(&w,&toggle,&r)==LC_COMMAND_APPLIED&&w.sound_event_count==1);
    assert(lc_audio_sample(&w,&s));
    const LCSoundCue *c=kind(&s,LC_SOUND_DOOR_CLICK);
    assert(c&&c->source_id==110&&c->start_frame==16000&&c->duration_frames==4160);
    LCAudioSample isolated={.count=1};isolated.cues[0]=*c;
    int16_t pcm[320];
    assert(lc_audio_render(&isolated,15840,160,pcm,320));
    for(size_t i=0;i<320;i++)assert(!pcm[i]);
    assert(lc_audio_render(&isolated,16000,160,pcm,320));
    bool nonzero=false;for(size_t i=0;i<320;i++)if(pcm[i])nonzero=true;
    assert(nonzero);
    assert(lc_audio_render(&isolated,20160,160,pcm,320));
    for(size_t i=0;i<320;i++)assert(!pcm[i]);
    assert(lc_environment_advance(&w,260)&&lc_audio_sample(&w,&s));
    assert(!kind(&s,LC_SOUND_DOOR_CLICK)&&kind(&s,LC_SOUND_DOORWAY_AIR));
    assert(lc_execute(&w,&toggle,&r)==LC_COMMAND_APPLIED);
    assert(lc_audio_sample(&w,&s)&&kind(&s,LC_SOUND_DOOR_CLICK)&&!kind(&s,LC_SOUND_DOORWAY_AIR));
    const char *path="/tmp/livingcolors_sound_event_test.save";
    assert(lc_save(&w,path));assert(lc_load(&w,path));
    assert(!w.sound_event_count&&!w.footstep_distance_m);
    assert(lc_audio_sample(&w,&s)&&!kind(&s,LC_SOUND_DOOR_CLICK));
    assert(remove(path)==0);
    w.player.yaw=0;
    LCCommand walk={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_WALK,.input.walk={1,0,.2f}};
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_APPLIED&&!w.sound_event_count);
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_APPLIED&&w.sound_event_count==1);
    assert(lc_audio_sample(&w,&s)&&kind(&s,LC_SOUND_FOOTSTEP));
    assert(kind(&s,LC_SOUND_FOOTSTEP)->source_id==0);
    walk.input.walk.seconds=0;
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_UNCHANGED&&w.sound_event_count==1);
    assert(lc_environment_advance(&w,140)&&lc_audio_sample(&w,&s)&&!kind(&s,LC_SOUND_FOOTSTEP));
    uint64_t frame;
    assert(lc_audio_frame_at(1000,&frame)&&frame==16000);
    assert(!lc_audio_frame_at(UINT64_MAX,&frame));
    lc_audio_reset_transients(&w);
    LCObject *door=lc_find_object(&w,110);
    for(unsigned i=0;i<LC_SOUND_EVENT_CAPACITY+2;i++){
        assert(lc_environment_advance(&w,1));
        lc_audio_note_toggle(&w,door);
    }
    assert(w.sound_event_count==LC_SOUND_EVENT_CAPACITY);
    assert(w.sound_events[0].started_ms==w.environment_elapsed_ms-(LC_SOUND_EVENT_CAPACITY-1));
    puts("Living Colors: action-derived transient sound, expiry, PCM envelope and reload tests passed");
    return 0;
}
