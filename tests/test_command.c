#include "command.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static LCCommand use(uint64_t seq,uint32_t id,LCAction action){
    return (LCCommand){.api_version=LC_COMMAND_API_VERSION,.correlation_id=seq,
        .kind=LC_COMMAND_USE,.input.use={id,action}};
}
static void aim(LCWorld *w,LCVec target){
    LCVec eye=lc_eye(&w->player);
    float x=target.x-eye.x,y=target.y-eye.y,z=target.z-eye.z;
    w->player.yaw=atan2f(x,-z);w->player.pitch=atan2f(y,hypotf(x,z));
}
int main(void){
    LCWorld w;lc_house(&w);LCCommandReceipt r;
    LCCommand walk={.api_version=LC_COMMAND_API_VERSION,.correlation_id=42,
        .kind=LC_COMMAND_WALK,.input.walk={1,0,.25f}};
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_APPLIED);
    assert(r.correlation_id==42&&r.kind==LC_COMMAND_WALK&&r.status==LC_COMMAND_APPLIED);
    assert(fabsf(r.before_pose.position.z-8.8f)<.001f);
    assert(fabsf(r.after_pose.position.z-8.3f)<.001f);
    assert(!r.before_object.id&&!r.after_object.id);
    walk.input.walk.seconds=0;
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_UNCHANGED);
    walk.input.walk.seconds=NAN;
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_INVALID);
    assert(r.before_pose.position.z==r.after_pose.position.z);
    walk.input.walk.seconds=1;walk.api_version=99;
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_INVALID);
    walk.api_version=LC_COMMAND_API_VERSION;
    w.player=(LCPose){{2,0,5},0,0};
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_APPLIED); /* partial travel */
    assert(lc_execute(&w,&walk,&r)==LC_COMMAND_UNCHANGED); /* wall */
    assert(r.before_pose.position.z==r.after_pose.position.z);
    LCCommand look={.api_version=LC_COMMAND_API_VERSION,.correlation_id=43,
        .kind=LC_COMMAND_LOOK,.input.look={.5f,9}};
    assert(lc_execute(&w,&look,&r)==LC_COMMAND_APPLIED);
    assert(r.before_pose.yaw!=r.after_pose.yaw&&r.after_pose.pitch<=1.4f);
    look.input.look.yaw_delta=INFINITY;
    assert(lc_execute(&w,&look,&r)==LC_COMMAND_INVALID);
    LCCommand cmd=use(44,204,LC_ACTION_TOGGLE);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_REJECTED);
    assert(!r.before_object.id&&!r.after_object.id);
    w.player=(LCPose){{4.8f,3,5.1f},0,0};cmd=use(45,211,LC_ACTION_TOGGLE);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(r.before_object.id==211&&r.after_object.id==211);
    assert(!(r.before_object.flags&LC_OBJ_OPEN)&&(r.after_object.flags&LC_OBJ_OPEN));
    assert(r.before_object.location==LC_LOCATION_WORLD&&r.after_object.location==LC_LOCATION_WORLD);
    assert(r.before_object.bounds.max.x!=r.after_object.bounds.max.x);
    w.player=(LCPose){{1.85f,3,3.25f},0,0};
    aim(&w,(LCVec){.82f,3.78f,3.25f});cmd=use(46,210,LC_ACTION_READ_NEXT);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(r.before_object.progress==0&&r.after_object.progress==1);
    cmd=use(47,210,LC_ACTION_PICK_UP);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(r.before_held_id==0&&r.after_held_id==210);
    assert(r.before_object.location==LC_LOCATION_WORLD&&r.after_object.location==LC_LOCATION_HELD);
    assert(r.after_object.bounds.max.x==0&&r.after_object.progress==1);
    cmd=use(48,210,LC_ACTION_PLACE);
    w.player.yaw=0;w.player.pitch=0;
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_REJECTED);
    assert(r.before_held_id==210&&r.after_held_id==210&&!r.after_object.id);
    aim(&w,(LCVec){1.05f,3.75f,3.5f});
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(r.before_object.location==LC_LOCATION_HELD&&r.after_object.location==LC_LOCATION_WORLD);
    assert(r.after_held_id==0&&r.after_object.progress==1);
    assert(fabsf(r.after_object.bounds.min.y-3.75f)<.001f);
    puts("Living Colors: typed command and factual receipt tests passed");
    return 0;
}
