#include "session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rgb[LC_FRAME_RGB_BYTES],other[LC_FRAME_RGB_BYTES];
static float depth[LC_FRAME_PIXELS];
static int16_t pcm[LC_AUDIO_MAX_FRAMES*LC_AUDIO_CHANNELS],audio_copy[LC_AUDIO_MAX_FRAMES*LC_AUDIO_CHANNELS];
static LCCaptureRequest request(void){
    return (LCCaptureRequest){.flags=LC_CAPTURE_RGB|LC_CAPTURE_AUDIO|LC_CAPTURE_ENVIRONMENT,
        .rgb=rgb,.rgb_bytes=sizeof(rgb),.depth=depth,.depth_count=LC_FRAME_PIXELS,
        .pcm=pcm,.pcm_samples=sizeof(pcm)/sizeof(pcm[0]),.audio_frames=LC_AUDIO_MAX_FRAMES};
}
int main(void){
    LCSession *a=lc_session_create(17),*b=lc_session_create(18);
    assert(a&&b&&!lc_session_create(0));
    LCSessionStamp st;
    assert(lc_session_stamp(a,&st)&&st.session_id==17&&st.generation==1&&!st.revision&&!st.elapsed_ms);
    LCCaptureRequest q=request();LCCaptureResult out;
    assert(lc_session_capture(a,&q,&out)&&out.width==640&&out.height==480);
    assert(out.audio_rate==16000&&out.audio_channels==2&&out.audio_frames==1600);
    assert(out.stamp.elapsed_ms==out.environment.elapsed_ms&&out.stamp.elapsed_ms==out.acoustic.elapsed_ms);
    memcpy(other,rgb,sizeof(rgb));memcpy(audio_copy,pcm,sizeof(pcm));
    assert(lc_session_capture(a,&q,&out)&&!memcmp(rgb,other,sizeof(rgb))&&!memcmp(pcm,audio_copy,sizeof(pcm)));
    q.rgb_bytes--;assert(!lc_session_capture(a,&q,&out));q.rgb_bytes++;
    assert(out.stamp.revision==0);
    LCCommand cmd={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_WALK,.correlation_id=91,.input.walk={1,0,.4f}};
    LCSessionReceipt r;
    assert(lc_session_execute(a,&cmd,&r)&&r.command.status==LC_COMMAND_APPLIED);
    assert(r.before.revision==0&&r.after.revision==1&&r.command.correlation_id==91);
    assert(lc_session_advance(a,100)&&lc_session_stamp(a,&st)&&st.revision==2&&st.elapsed_ms==100);
    assert(lc_session_capture(a,&q,&out)&&out.stamp.revision==2&&memcmp(rgb,other,sizeof(rgb)));
    assert(lc_session_capture(b,&q,&out)&&out.stamp.session_id==18&&out.stamp.revision==0&&!memcmp(rgb,other,sizeof(rgb)));
    cmd.kind=LC_COMMAND_SENSE;cmd.input.sense.kind=LC_SENSE_AUDIO;cmd.input.sense.expected_object_id=0;
    assert(lc_session_execute(a,&cmd,&r)&&r.command.status==LC_COMMAND_OBSERVED&&r.after.revision==2);
    assert(!lc_session_advance(a,86400001)&&lc_session_advance(a,0));
    const char *path="/tmp/livingcolors_session_test.save";
    assert(lc_session_save(a,path));
    assert(lc_session_advance(a,400));
    assert(lc_session_load(a,path)&&lc_session_stamp(a,&st));
    assert(st.session_id==17&&st.generation==2&&st.revision==4&&st.elapsed_ms==100);
    assert(lc_session_capture(a,&q,&out)&&out.stamp.generation==2);
    for(size_t i=0;i<out.acoustic.count;i++)assert(!out.acoustic.cues[i].duration_frames);
    assert(remove(path)==0&&!lc_session_load(a,path));
    assert(lc_session_stamp(a,&st)&&st.generation==2&&st.revision==4);
    lc_session_destroy(a);lc_session_destroy(b);lc_session_destroy(NULL);
    puts("Living Colors: independent headless sessions, coherent capture, revisions and restore tests passed");
    return 0;
}
