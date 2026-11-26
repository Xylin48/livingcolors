#include "storage.h"
#include "body.h"
#include "interaction.h"
#include <math.h>

#define WALL .04f
#define ROWS 3u
static const LCObject *find(const LCWorld *w,uint32_t id){
    if(!w||!id||w->object_count>LC_MAX_OBJECTS)return NULL;
    for(size_t i=0;i<w->object_count;i++)if(w->objects[i].id==id)return &w->objects[i];
    return NULL;
}
static bool container(const LCObject *o){
    return o&&o->kind==LC_CONTAINER&&(o->flags&LC_OBJ_SOLID)&&
           !(o->flags&LC_OBJ_MOVABLE)&&!o->container_id&&
           o->shape.max.x-o->shape.min.x>.2f&&o->shape.max.y-o->shape.min.y>.3f&&
           o->shape.max.z-o->shape.min.z>.2f;
}
static unsigned columns(const LCObject *o){return o->shape.max.x-o->shape.min.x>1.5f?3u:2u;}
unsigned lc_container_capacity(const LCObject *o){return container(o)?columns(o)*ROWS:0;}
static float row_step(const LCObject *o){return (o->shape.max.y-o->shape.min.y-WALL)/ROWS;}
size_t lc_container_panels(const LCObject *o,LCBox out[LC_CONTAINER_PANELS]){
    if(!container(o)||!out)return 0;
    const LCBox b=o->shape;
    if(!(o->flags&LC_OBJ_OPEN)){out[0]=b;return 1;}
    out[0]=(LCBox){b.min,{b.max.x,b.max.y,b.min.z+WALL}};
    out[1]=(LCBox){b.min,{b.min.x+WALL,b.max.y,b.max.z}};
    out[2]=(LCBox){{b.max.x-WALL,b.min.y,b.min.z},b.max};
    out[3]=(LCBox){b.min,{b.max.x,b.min.y+WALL,b.max.z}};
    out[4]=(LCBox){{b.min.x,b.max.y-WALL,b.min.z},b.max};
    for(unsigned i=1;i<ROWS;i++){
        float y=b.min.y+row_step(o)*i;
        out[4+i]=(LCBox){{b.min.x+WALL,y,b.min.z+WALL},{b.max.x-WALL,y+WALL,b.max.z}};
    }
    return LC_CONTAINER_PANELS;
}
bool lc_container_slot(const LCObject *o,unsigned slot,LCBox *out){
    unsigned cap=lc_container_capacity(o);
    if(!slot||slot>cap||!out)return false;
    unsigned cols=columns(o),order=(slot-1)/cols,col=(slot-1)%cols;
    /* Middle row first, then upper, then lower: deterministic reachable order. */
    unsigned row=order==0?1u:order==1?2u:0u;
    LCBox b=o->shape;
    float width=(b.max.x-b.min.x-2*WALL)/cols;
    float y=b.min.y+row_step(o)*row;
    float x=b.min.x+WALL+col*width;
    *out=(LCBox){{x,y+WALL,b.min.z+WALL},{x+width,y+row_step(o),b.max.z-WALL}};
    return true;
}
bool lc_storage_visible(const LCWorld *w,const LCObject *item){
    if(!w||!item||item->id==w->held_object_id)return false;
    if(!item->container_id)return true;
    const LCObject *parent=find(w,item->container_id);
    return container(parent)&&(parent->flags&LC_OBJ_OPEN);
}
bool lc_storage_accessible(const LCWorld *w,const LCObject *o){
    if(!w||!container(o)||!(o->flags&LC_OBJ_OPEN)||!lc_body_valid(w))return false;
    LCPose camera=lc_body_camera_pose(w);
    LCVec eye=lc_eye(&camera);
    /* Only the front (+z) is open; side/back walls are not access portals. */
    return eye.z>=o->shape.max.z;
}
static bool within(LCBox a,LCBox b){
    const float e=.0001f;
    return isfinite(a.min.x)&&isfinite(a.min.y)&&isfinite(a.min.z)&&
           isfinite(a.max.x)&&isfinite(a.max.y)&&isfinite(a.max.z)&&
           a.min.x<a.max.x&&a.min.y<a.max.y&&a.min.z<a.max.z&&
           a.min.x>=b.min.x-e&&a.max.x<=b.max.x+e&&
           fabsf(a.min.y-b.min.y)<=e&&a.max.y<=b.max.y+e&&
           a.min.z>=b.min.z-e&&a.max.z<=b.max.z+e;
}
bool lc_storage_valid(const LCWorld *w){
    if(!w||w->object_count>LC_MAX_OBJECTS)return false;
    for(size_t i=0;i<w->object_count;i++){
        const LCObject *o=&w->objects[i];
        if(!o->container_id){if(o->container_slot)return false;continue;}
        if(!o->container_slot||o->id==w->held_object_id||!(o->flags&LC_OBJ_MOVABLE)||
           o->kind==LC_CONTAINER||o->id==o->container_id)return false;
        const LCObject *p=find(w,o->container_id);
        LCBox slot;
        if(!lc_container_slot(p,o->container_slot,&slot)||!within(o->shape,slot))return false;
        for(size_t j=0;j<i;j++)
            if(w->objects[j].container_id==o->container_id&&
               w->objects[j].container_slot==o->container_slot)return false;
    }
    return true;
}
bool lc_store_held(LCWorld *w,uint32_t id){
    if(!w||!w->held_object_id||!lc_storage_valid(w))return false;
    LCObject *p=lc_find_object(w,id),*o=lc_find_object(w,w->held_object_id);
    if(!o||!p||o==p||!(o->flags&LC_OBJ_MOVABLE)||o->kind==LC_CONTAINER||
       !lc_storage_accessible(w,p))return false;
    LCHit hit;
    if(!lc_focus(w,1.5f,&hit))return false;
    const LCObject *focused=find(w,hit.object_id);
    if(hit.object_id!=id&&(!focused||focused->container_id!=id))return false;
    LCVec size={o->shape.max.x-o->shape.min.x,o->shape.max.y-o->shape.min.y,
                o->shape.max.z-o->shape.min.z};
    if(!isfinite(size.x)||!isfinite(size.y)||!isfinite(size.z)||
       size.x<=0||size.y<=0||size.z<=0)return false;
    LCPose camera=lc_body_camera_pose(w);
    LCVec eye=lc_eye(&camera);
    for(unsigned k=1;k<=lc_container_capacity(p);k++){
        LCBox b;
        if(!lc_container_slot(p,k,&b))continue;
        bool occupied=false;
        for(size_t j=0;j<w->object_count;j++)
            if(w->objects[j].container_id==id&&w->objects[j].container_slot==k)occupied=true;
        if(occupied||size.x>b.max.x-b.min.x||size.y>b.max.y-b.min.y||
           size.z>b.max.z-b.min.z)continue;
        float x=(b.min.x+b.max.x-size.x)*.5f,z=(b.min.z+b.max.z-size.z)*.5f;
        LCBox a={{x,b.min.y,z},{x+size.x,b.min.y+size.y,z+size.z}};
        if(!within(a,b))continue;
        LCVec center={(a.min.x+a.max.x)*.5f,(a.min.y+a.max.y)*.5f,(a.min.z+a.max.z)*.5f};
        LCVec d={center.x-eye.x,center.y-eye.y,center.z-eye.z};
        float distance=hypotf(hypotf(d.x,d.y),d.z);
        /* A slot must be reachable and unobstructed, not just mathematically empty. */
        LCHit obstruction;
        if(distance>1.5f||distance<=.001f||lc_cast(w,eye,d,distance-.0001f,&obstruction))continue;
        o->shape=a;o->container_id=id;o->container_slot=k;w->held_object_id=0;
        return true;
    }
    return false;
}
