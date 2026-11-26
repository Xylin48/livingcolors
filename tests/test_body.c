#include "body.h"
#include "command.h"
#include "perception.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void aim(LCWorld *w,LCVec target){
    LCPose p=lc_body_camera_pose(w);LCVec e=lc_eye(&p);
    float x=target.x-e.x,y=target.y-e.y,z=target.z-e.z;
    w->player.yaw=atan2f(x,-z);w->player.pitch=atan2f(y,hypotf(x,z));
}
static LCCommand use(uint32_t id,LCAction a){
    return (LCCommand){.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_USE,.input.use={id,a}};
}
int main(void){
    LCWorld w;lc_house(&w);LCCommandReceipt r;LCView v;
    assert(lc_body_valid(&w));
    LCCommand cmd=use(101,LC_ACTION_SIT);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_REJECTED);
    w.player=(LCPose){{1.9f,0,6.5f},0,0};
    aim(&w,(LCVec){1.1f,.8f,6.5f});
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(r.before_posture==LC_STANDING&&r.after_posture==LC_SEATED&&r.after_support_id==101);
    assert(lc_body_valid(&w)&&fabsf(w.player.position.y-.8f)<.001f);
    assert(lc_world_view(&w,&v)&&fabsf(v.origin.y-1.52f)<.001f);
    LCHit h;
    LCHit other;
    bool focused=lc_focus(&w,10,&h),sampled=lc_view_sample(&w,.5f,.5f,10,&other);
    assert(focused==sampled);
    if(focused)assert(h.object_id==other.object_id&&fabsf(h.distance-other.distance)<.001f);
    LCCommand walk={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_WALK,.input.walk={1,0,1}};
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_REJECTED);
    assert(!lc_move(&w,&w.player,1,0,.2f));
    assert(w.support_id==101);
    const char *path="/tmp/livingcolors_body_test.save";
    assert(lc_save(&w,path));
    LCWorld fresh;lc_house(&fresh);assert(lc_load(&fresh,path));
    assert(fresh.posture==LC_SEATED&&fresh.support_id==101&&lc_body_valid(&fresh));
    cmd=use(101,LC_ACTION_STAND);
    assert(lc_execute(&fresh,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(r.before_posture==LC_SEATED&&r.after_posture==LC_STANDING&&!fresh.support_id);
    assert(fabsf(fresh.player.position.x-1.9f)<.001f);
    assert(lc_add_box(&w,(LCBox){{1.7f,0,6.3f},{2.1f,.5f,6.7f}}));
    assert(!lc_body_stand(&w)&&w.posture==LC_SEATED);
    lc_house(&w);
    w.player=(LCPose){{2.9f,3,1.5f},0,0};
    aim(&w,(LCVec){2.0f,3.65f,1.5f});
    cmd=use(201,LC_ACTION_REST);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(w.posture==LC_RESTING&&lc_body_valid(&w));
    assert(lc_world_view(&w,&v)&&fabsf(v.origin.y-3.95f)<.001f);
    assert(lc_save(&w,path));lc_house(&fresh);assert(lc_load(&fresh,path));
    assert(fresh.posture==LC_RESTING&&fresh.support_id==201);
    assert(lc_body_stand(&fresh));
    assert(fabsf(fresh.player.position.x-2.9f)<.001f);
    w=fresh;w.player=(LCPose){{2.75f,3,3.85f},0,0};
    aim(&w,(LCVec){2.2f,3.45f,3.85f});
    cmd=use(215,LC_ACTION_SIT);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_APPLIED&&w.support_id==215);
    assert(lc_body_stand(&w));
    assert(remove(path)==0);
    puts("Living Colors: furniture, posture, body perception and persistence tests passed");
    return 0;
}
