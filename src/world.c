#include "world.h"
#include "body.h"
#include "storage.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static bool finite_vec(LCVec v) { return isfinite(v.x) && isfinite(v.y) && isfinite(v.z); }
static bool valid_box(LCBox b) {
    return finite_vec(b.min) && finite_vec(b.max) &&
        b.min.x < b.max.x && b.min.y < b.max.y && b.min.z < b.max.z;
}
static LCBox box(float x, float y, float z, float X, float Y, float Z) {
    return (LCBox){{x,y,z},{X,Y,Z}};
}
static void wall(LCWorld *w, float x,float y,float z,float X,float Y,float Z, LCColor color) {
    (void)lc_add_wall(w, box(x,y,z,X,Y,Z),color);
}
static void object(LCWorld *w, uint32_t id,const char *name,LCObjectKind kind,
                   LCBox shape, LCColor color,uint32_t flags) {
    LCObject obj={0};
    obj.id=id; obj.kind=kind; obj.shape=shape; obj.color=color; obj.flags=flags;
    (void)snprintf(obj.name,sizeof(obj.name),"%s",name);
    (void)lc_add_object(w,obj);
}

bool lc_add_wall(LCWorld *w, LCBox b, LCColor color) {
    if (!w || w->count>=LC_MAX_SOLIDS || !valid_box(b)) return false;
    w->solids[w->count++]=(LCSolid){b,color};
    return true;
}
bool lc_add_box(LCWorld *w, LCBox b) { return lc_add_wall(w,b,(LCColor){180,166,146}); }
bool lc_add_object(LCWorld *w, LCObject o) {
    if (!w || w->object_count>=LC_MAX_OBJECTS || !o.id || o.container_id || o.container_slot || o.material<LC_MATERIAL_UNKNOWN ||
        o.material>LC_MATERIAL_PLASTIC || !valid_box(o.shape) ||
        !memchr(o.name,'\0',sizeof(o.name))) return false;
    for(size_t i=0;i<w->object_count;i++) if(w->objects[i].id==o.id) return false;
    w->objects[w->object_count++]=o;
    return true;
}
LCObject *lc_find_object(LCWorld *w,uint32_t id) {
    if(!w||w->object_count>LC_MAX_OBJECTS)return NULL;
    for(size_t i=0;i<w->object_count;i++) if(w->objects[i].id==id)return &w->objects[i];
    return NULL;
}
/* One physical shape for open doors, used by collision, rendering and sight. */
LCBox lc_object_shape(const LCObject *o) {
    LCBox b=o->shape;
    if(o->kind!=LC_DOOR || !(o->flags&LC_OBJ_OPEN))return b;
    float width=b.max.x-b.min.x, depth=b.max.z-b.min.z;
    if(width>=depth) {
        b.max.x=b.min.x+.09f;
        if(b.min.z>5.0f)b.min.z=fmaxf(.13f,b.min.z-width);
        else b.max.z=fminf(9.87f,b.min.z+width);
    } else {
        b.max.z=b.min.z+.09f;
        b.max.x=fminf(11.87f,b.min.x+depth);
    }
    return b;
}
bool lc_stair_tread(size_t i, LCBox *out) {
    if (!out || i>=LC_STAIR_TREADS) return false;
    const float start=9.55f,end=4.70f;
    if (i==LC_STAIR_TREADS-1) {
        *out=box(10.28f,0,4.45f,11.72f,LC_UPSTAIRS_Y,end);
    } else {
        const float n=(float)(LC_STAIR_TREADS-1);
        *out=box(10.28f,0,start-(start-end)*(i+1)/n,
                 11.72f,LC_UPSTAIRS_Y*(i+1)/n,start-(start-end)*i/n);
    }
    return true;
}
static bool boxes_overlap(LCBox a,LCBox b) {
    return a.min.x<b.max.x&&a.max.x>b.min.x&&a.min.y<b.max.y&&a.max.y>b.min.y&&
           a.min.z<b.max.z&&a.max.z>b.min.z;
}
static bool overlaps(LCBox b,LCVec feet) {
    const float r=LC_PLAYER_RADIUS;
    return feet.x+r>b.min.x && feet.x-r<b.max.x &&
        feet.y+LC_PLAYER_HEIGHT>b.min.y && feet.y<b.max.y &&
        feet.z+r>b.min.z && feet.z-r<b.max.z;
}
bool lc_blocked(const LCWorld *w, LCVec p) {
    if(!w || w->count>LC_MAX_SOLIDS || w->object_count>LC_MAX_OBJECTS || !finite_vec(p) || p.x<LC_PLAYER_RADIUS ||
       p.z<LC_PLAYER_RADIUS || p.x>LC_WORLD_WIDTH-LC_PLAYER_RADIUS ||
       p.z>LC_WORLD_DEPTH-LC_PLAYER_RADIUS || p.y<-.01f || p.y>LC_UPSTAIRS_Y+.01f)
        return true;
    for(size_t i=0;i<w->count;i++) if(overlaps(w->solids[i].shape,p)) return true;
    for(size_t i=0;i<w->object_count;i++) {
        const LCObject *o=&w->objects[i];
        if(o->id!=w->held_object_id && !o->container_id && (o->flags&LC_OBJ_SOLID) && overlaps(lc_object_shape(o),p)) return true;
    }
    return false;
}
/* Stairs run along the right wall from the entrance (z=9.55) towards the upper landing (z=4.70), continuing level to z=4.45.
 * Outside stairs, use the closest floor to prevent instantaneous level changes. */
float lc_floor_height(float x,float z,float current_y) {
    if(!isfinite(x)||!isfinite(z)||!isfinite(current_y))return NAN;
    if(x>=10.26f && x<=11.73f && z>=4.45f && z<=9.55f)
        return fminf(LC_UPSTAIRS_Y,LC_UPSTAIRS_Y*(9.55f-z)/(9.55f-4.70f));
    return current_y>=1.5f?LC_UPSTAIRS_Y:0.0f;
}
bool lc_move(const LCWorld *w,LCPose *p,float dx,float dz,float dt) {
    if(!w||!p||(p==&w->player&&w->posture!=LC_STANDING)||!finite_vec(p->position)||!isfinite(dx)||!isfinite(dz)||
       !isfinite(dt)||dt<0||dt>1||lc_blocked(w,p->position)) return false;
    float length=hypotf(dx,dz);
    if(!isfinite(length))return false;
    if(length>1.0f) {dx/=length;dz/=length;}
    /* Subdivide even for low framerates: no tunneling across thin walls. */
    int steps=(int)ceilf(dt*100.0f);
    if(steps==0)return true;
    for(int i=0;i<steps;i++) {
        LCVec next=p->position;
        next.x+=dx*(2.0f*dt/steps);
        float floor=lc_floor_height(next.x,next.z,p->position.y);
        if(fabsf(floor-p->position.y)<=.24f) {
            next.y=floor;
            if(!lc_blocked(w,next))p->position=next;
        }
        next=p->position;
        next.z+=dz*(2.0f*dt/steps);
        floor=lc_floor_height(next.x,next.z,p->position.y);
        if(fabsf(floor-p->position.y)<=.24f) {
            next.y=floor;
            if(!lc_blocked(w,next))p->position=next;
        }
    }
    return true;
}
static bool nearby(LCVec a, LCBox b, float distance) {
    float x=fmaxf(b.min.x,fminf(b.max.x,a.x));
    float y=fmaxf(b.min.y,fminf(b.max.y,a.y+1.0f));
    float z=fmaxf(b.min.z,fminf(b.max.z,a.z));
    return hypotf(hypotf(a.x-x,a.y+1.0f-y),a.z-z)<=distance;
}
bool lc_toggle_object(LCWorld *w,uint32_t id,LCVec from) {
    LCObject *o=lc_find_object(w,id);
    if(!o||id==w->held_object_id||o->container_id||!finite_vec(from)||!(o->flags&LC_OBJ_INTERACTIVE)||
       !nearby(from,lc_object_shape(o),1.5f))return false;
    if(o->kind==LC_DOOR) {
        LCObject next=*o; next.flags^=LC_OBJ_OPEN;
        if(lc_body_intersects(w,lc_object_shape(&next)))return false;
    }
    if(o->kind==LC_DOOR||o->kind==LC_CONTAINER)
        o->flags^=LC_OBJ_OPEN;
    else if(o->kind==LC_DEVICE)
        o->flags^=LC_OBJ_POWERED;
    else return false;
    return true;
}
bool lc_placement_clear(const LCWorld *w,uint32_t id,LCBox a,bool all_objects) {
    if(!w||!id||w->count>LC_MAX_SOLIDS||w->object_count>LC_MAX_OBJECTS||!valid_box(a)||
       a.min.x<0||a.min.y<0||a.min.z<0||a.max.x>LC_WORLD_WIDTH||
       a.max.y>LC_UPSTAIRS_Y+3||a.max.z>LC_WORLD_DEPTH)return false;
    const LCObject *item=NULL;
    for(size_t i=0;i<w->object_count;i++)if(w->objects[i].id==id)item=&w->objects[i];
    if(!item||!(item->flags&LC_OBJ_MOVABLE))return false;
    if(((item->flags&LC_OBJ_SOLID)||w->posture!=LC_STANDING)&&lc_body_intersects(w,a))return false;
    for(size_t i=0;i<w->count;i++)if(boxes_overlap(a,w->solids[i].shape))return false;
    for(size_t i=0;i<LC_STAIR_TREADS;i++){
        LCBox tread;
        if(lc_stair_tread(i,&tread)&&boxes_overlap(a,tread))return false;
    }
    for(size_t i=0;i<w->object_count;i++){
        const LCObject *o=&w->objects[i];
        if(o->id==id||o->id==w->held_object_id||o->container_id||(!all_objects&&!(o->flags&LC_OBJ_SOLID)))continue;
        if(boxes_overlap(a,lc_object_shape(o)))return false;
    }
    return true;
}
bool lc_move_object(LCWorld *w,uint32_t id,LCVec from,LCVec target) {
    LCObject *o=lc_find_object(w,id);
    if(!o||id==w->held_object_id||o->container_id||!finite_vec(from)||!finite_vec(target)||
       !(o->flags&LC_OBJ_MOVABLE)||!nearby(from,o->shape,1.5f)||
       hypotf(target.x-from.x,target.z-from.z)>1.6f||
       target.y>LC_UPSTAIRS_Y+2)return false;
    LCBox a={target,{target.x+(o->shape.max.x-o->shape.min.x),
                     target.y+(o->shape.max.y-o->shape.min.y),
                     target.z+(o->shape.max.z-o->shape.min.z)}};
    if(!lc_placement_clear(w,id,a,false))return false;
    o->shape=a;
    return true;
}
void lc_house(LCWorld *w) {
    if(!w)return;
    *w=(LCWorld){0};
    const LCColor cream={216,198,165}, wallblue={158,173,177}, tile={185,192,184};
    const LCColor wood={118,75,43},dark={70,64,59},cloth={100,104,121},metal={103,112,120};
    /* Physical ground surface, shared with ray casting and rendering. */
    wall(w,0,-.12f,0,12,0,10,(LCColor){125,102,78});
    /* All dimensions provisional: 12x10m exterior. Two stacked storeys, not a ranch. */
    wall(w,0,0,0,.13f,2.82f,10,cream);wall(w,11.87f,0,0,12,2.82f,10,cream);
    wall(w,0,0,0,6.2f,2.82f,.13f,cream);wall(w,7.2f,0,0,12,2.82f,.13f,cream);
    wall(w,0,0,9.87f,5.5f,2.82f,10,cream);wall(w,6.5f,0,9.87f,12,2.82f,10,cream);
    /* Opening x8.5..10 in boundary between living room and kitchen/dining. */
    wall(w,0,0,4.2f,8.5f,2.82f,4.32f,cream);
    wall(w,10,0,4.2f,12,2.82f,4.32f,cream);
    /* Stair guide wall, permits entry near front and exit near upstairs hallway. */
    wall(w,9.92f,0,5.15f,10.04f,2.65f,9.0f,cream);
    /* Physical upper floor; same geometry used by perception and rendering. */
    wall(w,.13f,2.88f,.13f,10.20f,3.0f,9.87f,wood);
    wall(w,10.20f,2.88f,.13f,11.87f,3.0f,4.30f,wood);
    /* Upstairs external walls and inner circulation. */
    wall(w,0,3,0,.13f,5.82f,10,wallblue);
    wall(w,11.87f,3,0,12,5.82f,10,wallblue);
    wall(w,0,3,0,12,5.82f,.13f,wallblue);
    wall(w,0,3,9.87f,12,5.82f,10,wallblue);
    /* Bedroom occupies full rear width, accessible from hallway. */
    wall(w,0,3,4.3f,4.1f,5.82f,4.43f,wallblue);
    wall(w,5.65f,3,4.3f,12,5.82f,4.43f,wallblue);
    /* Linen closet at far left, two doorways opening to hallway and bathroom. */
    wall(w,3.45f,3,5.1f,3.57f,5.82f,5.75f,tile);
    wall(w,3.45f,3,6.85f,3.57f,5.82f,7.85f,tile);
    wall(w,3.45f,3,8.8f,3.57f,5.82f,9.87f,tile);
    wall(w,0,3,5.1f,3.57f,5.82f,5.22f,tile);
    /* Bathroom central-right; bathroom door on top/north wall. */
    wall(w,3.57f,3,7.0f,5.4f,5.82f,7.12f,tile);
    wall(w,6.45f,3,7.0f,9.5f,5.82f,7.12f,tile);
    wall(w,9.5f,3,7.0f,9.62f,5.82f,9.87f,tile);
    /* Key ground floor furniture */
    object(w,101,"Living room sofa",LC_FURNITURE,box(.5f,0,5.2f,1.55f,.8f,8.6f),cloth,LC_OBJ_SOLID);
    object(w,102,"Coffee table",LC_FURNITURE,box(2.8f,0,6.1f,4.5f,.48f,7.6f),wood,LC_OBJ_SOLID);
    object(w,103,"Living room CRT",LC_DEVICE,box(6.0f,.68f,6.1f,6.8f,1.37f,7.0f),dark,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,104,"TV stand",LC_FURNITURE,box(5.85f,0,5.95f,6.95f,.68f,7.15f),wood,LC_OBJ_SOLID);
    object(w,105,"Kitchen counter",LC_FURNITURE,box(.35f,0,.35f,4.6f,.9f,.95f),wood,LC_OBJ_SOLID);
    object(w,106,"Kitchen side counter",LC_FURNITURE,box(.35f,0,1.0f,.95f,.9f,3.7f),wood,LC_OBJ_SOLID);
    object(w,107,"Fridge",LC_CONTAINER,box(5.2f,0,.3f,6.15f,1.95f,1.1f),metal,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,108,"Dining table",LC_FURNITURE,box(8.5f,0,1.3f,10.7f,.78f,3.2f),wood,LC_OBJ_SOLID);
    object(w,109,"Back door",LC_DOOR,box(6.23f,0,.04f,7.18f,2.2f,.12f),wood,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,110,"Front door",LC_DOOR,box(5.52f,0,9.9f,6.48f,2.25f,9.98f),wood,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    /* Upstairs: bed left, desk below bed facing left wall, top-wall TV & adjacent shelf. */
    object(w,201,"Bed",LC_FURNITURE,box(.38f,3,.42f,2.55f,3.65f,2.55f),cloth,LC_OBJ_SOLID);
    object(w,202,"School desk",LC_FURNITURE,box(.35f,3,2.95f,1.55f,3.75f,3.95f),wood,LC_OBJ_SOLID);
    object(w,203,"Bedroom TV stand",LC_FURNITURE,box(4.15f,3,.28f,6.2f,3.65f,1.03f),wood,LC_OBJ_SOLID);
    object(w,204,"Bedroom CRT",LC_DEVICE,box(4.4f,3.65f,.36f,5.93f,4.52f,.96f),dark,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,205,"Bookshelf",LC_CONTAINER,box(6.6f,3,.25f,9.1f,4.8f,.85f),wood,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,206,"Linen closet",LC_CONTAINER,box(.3f,3,5.55f,2.85f,4.9f,6.2f),wood,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,207,"Bathtub",LC_FURNITURE,box(8.65f,3,7.42f,9.45f,3.7f,9.4f),tile,LC_OBJ_SOLID);
    object(w,208,"Bathroom sink",LC_FURNITURE,box(4.2f,3,8.9f,5.4f,3.88f,9.55f),tile,LC_OBJ_SOLID);
    object(w,209,"Toilet",LC_FURNITURE,box(6.2f,3,9.0f,6.88f,3.72f,9.55f),tile,LC_OBJ_SOLID);
    object(w,211,"Bedroom door",LC_DOOR,box(4.12f,3,4.33f,5.62f,5.2f,4.41f),wood,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,212,"Bathroom door",LC_DOOR,box(5.42f,3,7.02f,6.43f,5.2f,7.10f),wood,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,213,"Closet hall door",LC_DOOR,box(3.47f,3,5.76f,3.55f,5.2f,6.83f),wood,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,214,"Closet bathroom door",LC_DOOR,box(3.47f,3,7.86f,3.55f,5.2f,8.78f),wood,LC_OBJ_SOLID|LC_OBJ_INTERACTIVE);
    object(w,210,"School reader",LC_BOOK,box(.63f,3.76f,3.10f,1.0f,3.8f,3.4f),(LCColor){212,185,128},LC_OBJ_MOVABLE|LC_OBJ_INTERACTIVE);
    object(w,215,"School chair",LC_FURNITURE,box(1.9f,3,3.61f,2.45f,3.45f,4.16f),wood,LC_OBJ_SOLID);
    /* Wall-mounted switches use the existing power-state contract; no extra device DB. */
    const LCColor switch_color={225,220,204};
    const uint32_t switch_flags=LC_OBJ_INTERACTIVE;
    object(w,301,"Living room light switch",LC_DEVICE,box(4.9f,1.1f,9.79f,5.04f,1.27f,9.86f),switch_color,switch_flags);
    object(w,302,"Kitchen light switch",LC_DEVICE,box(8.15f,1.1f,4.12f,8.3f,1.27f,4.19f),switch_color,switch_flags);
    object(w,303,"Bedroom light switch",LC_DEVICE,box(5.8f,4.1f,4.22f,5.94f,4.27f,4.29f),switch_color,switch_flags);
    object(w,304,"Hall light switch",LC_DEVICE,box(6.0f,4.1f,4.44f,6.14f,4.27f,4.51f),switch_color,switch_flags);
    object(w,305,"Bathroom light switch",LC_DEVICE,box(6.6f,4.1f,7.13f,6.74f,4.27f,7.2f),switch_color,switch_flags);
    object(w,306,"Closet light switch",LC_DEVICE,box(3.37f,4.1f,6.95f,3.44f,4.27f,7.09f),switch_color,switch_flags);
    /* Material belongs to the definition, not mutable object state or display color.
     * Single dominant exterior material for now; composite meshes are future work. */
    for(size_t i=0;i<w->object_count;i++){
        LCObject *o=&w->objects[i];
        switch(o->id){
        case 101:case 201:o->material=LC_MATERIAL_FABRIC;break;
        case 107:o->material=LC_MATERIAL_METAL;break;
        case 210:o->material=LC_MATERIAL_PAPER;break;
        case 207:case 208:case 209:o->material=LC_MATERIAL_CERAMIC;break;
        case 103:case 204:case 301:case 302:case 303:case 304:case 305:case 306:
            o->material=LC_MATERIAL_PLASTIC;break;
        default:o->material=LC_MATERIAL_WOOD;break;
        }
    }
    w->player=(LCPose){{6.0f,0,8.8f},0,0};
}
