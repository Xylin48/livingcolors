#include "interaction.h"
#include <math.h>
#include <stdint.h>

static bool finite_vec(LCVec v) {return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}
LCVec lc_eye(const LCPose *p) {
    if(!p)return (LCVec){NAN,NAN,NAN};
    return (LCVec){p->position.x,p->position.y+LC_EYE_HEIGHT,p->position.z};
}
LCVec lc_view_direction(const LCPose *p) {
    if(!p)return (LCVec){NAN,NAN,NAN};
    float c=cosf(p->pitch);
    return (LCVec){sinf(p->yaw)*c,sinf(p->pitch),-cosf(p->yaw)*c};
}
static bool slab(float o,float d,float min,float max,float *near,float *far) {
    if(fabsf(d)<1e-8f)return o>=min&&o<=max;
    float a=(min-o)/d,b=(max-o)/d;
    if(a>b){float t=a;a=b;b=t;}
    *near=fmaxf(*near,a);*far=fminf(*far,b);
    return *near<=*far;
}
static bool intersect(LCBox b,LCVec o,LCVec d,float limit,float *distance) {
    float near=0,far=limit;
    if(!slab(o.x,d.x,b.min.x,b.max.x,&near,&far) ||
       !slab(o.y,d.y,b.min.y,b.max.y,&near,&far) ||
       !slab(o.z,d.z,b.min.z,b.max.z,&near,&far))return false;
    *distance=near;return true;
}
bool lc_cast(const LCWorld *w,LCVec o,LCVec d,float limit,LCHit *out) {
    if(!w||!out||!finite_vec(o)||!finite_vec(d)||!isfinite(limit)||limit<=0 ||
       w->count>LC_MAX_SOLIDS||w->object_count>LC_MAX_OBJECTS)return false;
    float n=hypotf(hypotf(d.x,d.y),d.z);
    if(!isfinite(n)||n<1e-8f)return false;
    d=(LCVec){d.x/n,d.y/n,d.z/n};
    float best=limit,t;uint32_t id=0;bool found=false;
    for(size_t i=0;i<w->count;i++){
        if(intersect(w->solids[i].shape,o,d,best,&t)){best=t;found=true;}
    }
    for(size_t i=0;i<w->object_count;i++){
        const LCObject *obj=&w->objects[i];
        if(intersect(lc_object_shape(obj),o,d,best,&t)&&(!found||t<best)){
            best=t;id=obj->id;found=true;
        }
    }
    if(!found)return false;
    *out=(LCHit){id,best,{o.x+d.x*best,o.y+d.y*best,o.z+d.z*best}};
    return true;
}
bool lc_focus(const LCWorld *w,float limit,LCHit *out){
    return w && lc_cast(w,lc_eye(&w->player),lc_view_direction(&w->player),limit,out);
}
bool lc_look(LCWorld *w,float dyaw,float dpitch){
    if(!w||!isfinite(dyaw)||!isfinite(dpitch)||
       !isfinite(w->player.yaw)||!isfinite(w->player.pitch))return false;
    w->player.yaw=remainderf(remainderf(w->player.yaw,6.283185307f)+remainderf(dyaw,6.283185307f),6.283185307f);
    w->player.pitch=fmaxf(-1.4f,fminf(1.4f,w->player.pitch+dpitch));
    return true;
}
bool lc_use(LCWorld *w,uint32_t id,LCAction action){
    LCHit hit;
    if(!w||!id||lc_blocked(w,w->player.position)||!lc_focus(w,1.5f,&hit)||hit.object_id!=id)return false;
    LCObject *o=lc_find_object(w,id);
    if(!o||!(o->flags&LC_OBJ_INTERACTIVE))return false;
    if(action==LC_ACTION_TOGGLE)
        return lc_toggle_object(w,id,w->player.position);
    if(action==LC_ACTION_READ_NEXT && o->kind==LC_BOOK && o->progress<UINT32_MAX){
        o->progress++;return true;
    }
    return false;
}
