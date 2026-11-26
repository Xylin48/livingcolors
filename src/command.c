#include "command.h"
#include "body.h"

#include <math.h>

static LCObjectState describe(const LCWorld *w,const LCObject *o){
    LCObjectState s={0};
    if(!o)return s;
    s.id=o->id;s.kind=o->kind;s.flags=o->flags;s.progress=o->progress;
    s.location=w->held_object_id==o->id?LC_LOCATION_HELD:LC_LOCATION_WORLD;
    if(s.location==LC_LOCATION_WORLD)s.bounds=lc_object_shape(o);
    return s;
}
static bool position_equal(LCVec a,LCVec b){
    return a.x==b.x&&a.y==b.y&&a.z==b.z;
}
static bool pose_equal(LCPose a,LCPose b){
    return position_equal(a.position,b.position)&&a.yaw==b.yaw&&a.pitch==b.pitch;
}
LCCommandStatus lc_execute(LCWorld *w,const LCCommand *cmd,LCCommandReceipt *out){
    if(!out)return LC_COMMAND_INVALID;
    LCCommandReceipt r={.api_version=LC_COMMAND_API_VERSION,.status=LC_COMMAND_INVALID};
    if(cmd){
        r.correlation_id=cmd->correlation_id;r.kind=cmd->kind;
        if(cmd->kind==LC_COMMAND_USE){
            r.requested_action=cmd->input.use.action;
            r.requested_object_id=cmd->input.use.object_id;
        }
    }
    if(!w||!cmd||cmd->api_version!=LC_COMMAND_API_VERSION||
       !lc_body_valid(w)){
        *out=r;return r.status;
    }
    r.before_pose=r.after_pose=w->player;
    r.before_held_id=r.after_held_id=w->held_object_id;
    r.before_posture=r.after_posture=w->posture;
    r.before_support_id=r.after_support_id=w->support_id;
    switch(cmd->kind){
    case LC_COMMAND_WALK:{
        const float f=cmd->input.walk.forward,side=cmd->input.walk.right,dt=cmd->input.walk.seconds;
        if(!isfinite(f)||!isfinite(side)||!isfinite(dt)||fabsf(f)>1||fabsf(side)>1||dt<0||dt>1)break;
        if(dt==0||(f==0&&side==0)){r.status=LC_COMMAND_UNCHANGED;break;}
        if(w->posture!=LC_STANDING){r.status=LC_COMMAND_REJECTED;break;}
        const float yaw=w->player.yaw;
        const float dx=side*cosf(yaw)+f*sinf(yaw);
        const float dz=side*sinf(yaw)-f*cosf(yaw);
        if(!lc_move(w,&w->player,dx,dz,dt)){r.status=LC_COMMAND_REJECTED;break;}
        r.status=pose_equal(w->player,r.before_pose)?LC_COMMAND_UNCHANGED:LC_COMMAND_APPLIED;
        break;
    }
    case LC_COMMAND_LOOK:
        if(!isfinite(cmd->input.look.yaw_delta)||!isfinite(cmd->input.look.pitch_delta))break;
        if(!lc_look(w,cmd->input.look.yaw_delta,cmd->input.look.pitch_delta)){
            r.status=LC_COMMAND_REJECTED;break;
        }
        r.status=pose_equal(w->player,r.before_pose)?LC_COMMAND_UNCHANGED:LC_COMMAND_APPLIED;
        break;
    case LC_COMMAND_USE:{
        const uint32_t id=cmd->input.use.object_id;
        const LCAction a=cmd->input.use.action;
        if(!id||(a!=LC_ACTION_TOGGLE&&a!=LC_ACTION_READ_NEXT&&a!=LC_ACTION_PICK_UP&&a!=LC_ACTION_PLACE&&
                 a!=LC_ACTION_SIT&&a!=LC_ACTION_REST&&a!=LC_ACTION_STAND))break;
        LCObject *o=lc_find_object(w,id);
        if(!o){r.status=LC_COMMAND_REJECTED;break;}
        const LCObjectState before=describe(w,o);
        if(!lc_use(w,id,a)){r.status=LC_COMMAND_REJECTED;break;}
        r.before_object=before;
        r.after_object=describe(w,o);
        r.status=LC_COMMAND_APPLIED;
        break;
    }
    default:break;
    }
    r.after_pose=w->player;r.after_held_id=w->held_object_id;
    r.after_posture=w->posture;r.after_support_id=w->support_id;
    *out=r;
    return r.status;
}
