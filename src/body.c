#include "body.h"
#include "interaction.h"
#include <math.h>

static bool finite_vec(LCVec p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static bool same_floor(LCVec p){
    return finite_vec(p)&&p.x>=LC_PLAYER_RADIUS&&p.x<=LC_WORLD_WIDTH-LC_PLAYER_RADIUS&&
        p.z>=LC_PLAYER_RADIUS&&p.z<=LC_WORLD_DEPTH-LC_PLAYER_RADIUS&&p.y>=0&&p.y<=LC_UPSTAIRS_Y&&
        fabsf(lc_floor_height(p.x,p.z,p.y)-p.y)<=.26f;
}
static float clamp(float x,float lo,float hi){return fmaxf(lo,fminf(hi,x));}
static LCBox body_box(LCVec p,LCPosture posture){
    float x=LC_PLAYER_RADIUS,z=LC_PLAYER_RADIUS,h=LC_PLAYER_HEIGHT;
    if(posture==LC_SEATED){h=.85f;}
    if(posture==LC_RESTING){x=.30f;z=.78f;h=.40f;}
    return (LCBox){{p.x-x,p.y,p.z-z},{p.x+x,p.y+h,p.z+z}};
}
static bool intersects(LCBox a,LCBox b){
    return a.min.x<b.max.x&&a.max.x>b.min.x&&a.min.y<b.max.y&&a.max.y>b.min.y&&
           a.min.z<b.max.z&&a.max.z>b.min.z;
}
LCPosture lc_furniture_posture(const LCObject *o){
    /* Stable house IDs; furniture capabilities can become data-driven later.
     * Do not change legacy immutable flags in existing snapshots. */
    if(!o||o->kind!=LC_FURNITURE||!(o->flags&LC_OBJ_SOLID)||(o->flags&LC_OBJ_MOVABLE))
        return LC_STANDING;
    if(o->id==101||o->id==215)return LC_SEATED;
    if(o->id==201)return LC_RESTING;
    return LC_STANDING;
}
bool lc_body_intersects(const LCWorld *w,LCBox b){
    return w&&intersects(body_box(w->player.position,w->posture),b);
}
static bool supported(const LCWorld *w,const LCObject *o,LCVec p,LCPosture posture){
    if(!o||lc_furniture_posture(o)!=posture||posture==LC_STANDING||!finite_vec(p))return false;
    LCBox b=body_box(p,posture),s=o->shape;
    if(fabsf(p.y-s.max.y)>.0001f||b.min.x<s.min.x-.0001f||b.max.x>s.max.x+.0001f||
       b.min.z<s.min.z-.0001f||b.max.z>s.max.z+.0001f)return false;
    for(size_t i=0;i<w->count;i++)if(intersects(b,w->solids[i].shape))return false;
    for(size_t i=0;i<LC_STAIR_TREADS;i++){
        LCBox tread;
        if(lc_stair_tread(i,&tread)&&intersects(b,tread))return false;
    }
    for(size_t i=0;i<w->object_count;i++){
        const LCObject *other=&w->objects[i];
        if(other->id!=o->id&&other->id!=w->held_object_id&&
           intersects(b,lc_object_shape(other)))return false;
    }
    return true;
}
bool lc_body_valid(const LCWorld *w){
    if(!w||w->count>LC_MAX_SOLIDS||w->object_count>LC_MAX_OBJECTS||
       !isfinite(w->player.yaw)||!isfinite(w->player.pitch)||fabsf(w->player.pitch)>1.57f)return false;
    if(w->posture==LC_STANDING)
        return !w->support_id&&same_floor(w->player.position)&&!lc_blocked(w,w->player.position);
    if((w->posture!=LC_SEATED&&w->posture!=LC_RESTING)||!w->support_id||
       !same_floor(w->exit_position))return false;
    const LCObject *s=NULL;
    for(size_t i=0;i<w->object_count;i++)if(w->objects[i].id==w->support_id)s=&w->objects[i];
    LCVec p=w->player.position,e=w->exit_position;
    if(!s||fabsf(e.y-s->shape.min.y)>.26f||hypotf(e.x-p.x,e.z-p.z)>1.5f)return false;
    return supported(w,s,p,w->posture);
}
bool lc_body_enter(LCWorld *w,uint32_t id,LCPosture posture){
    if(!lc_body_valid(w)||w->posture!=LC_STANDING||
       (posture!=LC_SEATED&&posture!=LC_RESTING))return false;
    LCHit hit;
    if(!lc_focus(w,1.5f,&hit)||hit.object_id!=id)return false;
    LCObject *o=lc_find_object(w,id);
    if(lc_furniture_posture(o)!=posture)return false;
    LCBox s=o->shape;
    LCVec e=w->player.position;
    float rx=posture==LC_RESTING?.30f:LC_PLAYER_RADIUS;
    float rz=posture==LC_RESTING?.78f:LC_PLAYER_RADIUS;
    if(s.max.x-s.min.x<rx*2||s.max.z-s.min.z<rz*2)return false;
    LCVec p={(s.min.x+s.max.x)*.5f,s.max.y,clamp(e.z,s.min.z+rz,s.max.z-rz)};
    if(fabsf(e.y-s.min.y)>.26f||hypotf(e.x-p.x,e.z-p.z)>1.5f||!supported(w,o,p,posture))return false;
    w->exit_position=e;w->support_id=id;w->posture=posture;w->player.position=p;
    if(posture==LC_RESTING){w->player.yaw=0;w->player.pitch=1.2f;}
    else {w->player.yaw=id==215?-1.5707963f:1.5707963f;w->player.pitch=0;}
    return true;
}
bool lc_body_stand(LCWorld *w){
    if(!lc_body_valid(w)||w->posture==LC_STANDING||lc_blocked(w,w->exit_position))return false;
    w->player.position=w->exit_position;
    w->posture=LC_STANDING;w->support_id=0;w->exit_position=(LCVec){0};
    return true;
}
LCPose lc_body_camera_pose(const LCWorld *w){
    if(!w)return (LCPose){{NAN,NAN,NAN},NAN,NAN};
    LCPose p=w->player;
    if(w->posture==LC_SEATED)p.position.y+=.72f-LC_EYE_HEIGHT;
    if(w->posture==LC_RESTING)p.position.y+=.30f-LC_EYE_HEIGHT;
    return p;
}
