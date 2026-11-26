#include "environment.h"
#include "body.h"
#include "interaction.h"
#include <math.h>

#define PI 3.14159265358979323846f
static float clamp(float x,float lo,float hi){return fmaxf(lo,fminf(hi,x));}
static bool finite_vec(LCVec p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static const LCObject *find(const LCWorld *w,uint32_t id){
    if(!w||!id||w->object_count>LC_MAX_OBJECTS)return NULL;
    for(size_t i=0;i<w->object_count;i++)if(w->objects[i].id==id)return &w->objects[i];
    return NULL;
}
bool lc_environment_advance(LCWorld *w,uint32_t ms){
    if(!w||ms>LC_DAY_MS||UINT64_MAX-w->environment_elapsed_ms<ms)return false;
    w->environment_elapsed_ms+=ms;
    return true;
}
uint32_t lc_environment_time_of_day(const LCWorld *w){
    return w?(uint32_t)((w->environment_elapsed_ms%LC_DAY_MS+LC_START_MS)%LC_DAY_MS):0;
}
LCZone lc_environment_zone(LCVec p){
    if(!finite_vec(p)||p.x<0||p.x>LC_WORLD_WIDTH||p.z<0||p.z>LC_WORLD_DEPTH||
       p.y<-.12f||p.y>6)return LC_ZONE_UNKNOWN;
    if(p.x>=10.26f&&p.x<=11.73f&&p.z>=4.45f&&p.z<=9.55f)return LC_ZONE_STAIRS;
    if(p.y<3)return p.z<4.32f?LC_ZONE_KITCHEN:LC_ZONE_LIVING;
    if(p.z<4.43f)return LC_ZONE_BEDROOM;
    if(p.x<3.57f&&p.z>=5.22f)return LC_ZONE_CLOSET;
    if(p.x>=3.57f&&p.x<=9.62f&&p.z>=7.12f)return LC_ZONE_BATH;
    return LC_ZONE_HALL;
}
uint32_t lc_environment_switch(LCZone z){
    switch(z){
    case LC_ZONE_LIVING:return 301;
    case LC_ZONE_KITCHEN:return 302;
    case LC_ZONE_BEDROOM:return 303;
    case LC_ZONE_HALL:case LC_ZONE_STAIRS:return 304;
    case LC_ZONE_BATH:return 305;
    case LC_ZONE_CLOSET:return 306;
    default:return 0;
    }
}
static bool artificial(const LCWorld *w,LCZone z){
    const LCObject *o=find(w,lc_environment_switch(z));
    return o&&o->kind==LC_DEVICE&&(o->flags&LC_OBJ_POWERED);
}
float lc_environment_daylight(const LCWorld *w){
    if(!w)return 0;
    float hour=lc_environment_time_of_day(w)/3600000.0f;
    return clamp(sinf((hour-6.0f)*PI/12.0f),0,1);
}
float lc_environment_light_at(const LCWorld *w,LCVec point){
    LCZone z=lc_environment_zone(point);
    if(!w||z==LC_ZONE_UNKNOWN)return 0;
    float ambient=.08f+(z==LC_ZONE_CLOSET?.10f:z==LC_ZONE_STAIRS||z==LC_ZONE_HALL?.25f:.52f)*lc_environment_daylight(w);
    if(artificial(w,z))ambient+=.68f;
    return clamp(ambient,0,1);
}
static float door_draft(const LCWorld *w,uint32_t id,LCVec p){
    const LCObject *o=find(w,id);
    if(!o||o->kind!=LC_DOOR||!(o->flags&LC_OBJ_OPEN))return 0;
    /* Approximate only direct local airflow from an open exterior doorway.
     * A closed intervening wall/door blocks the direct path. */
    LCVec entry={(o->shape.min.x+o->shape.max.x)*.5f,1.2f,id==109?.16f:9.84f};
    LCVec d={entry.x-p.x,entry.y-p.y,entry.z-p.z};
    float n=hypotf(hypotf(d.x,d.y),d.z);
    if(n>4.0f)return 0;
    LCHit hit;
    if(n>.002f&&lc_cast(w,p,d,n-.001f,&hit))return 0;
    return .9f*(1-n/4.0f);
}
bool lc_environment_sample(const LCWorld *w,LCEnvironmentSample *out){
    if(!out||!lc_body_valid(w))return false;
    LCPose camera=lc_body_camera_pose(w);
    LCVec p=lc_eye(&camera);
    LCZone zone=lc_environment_zone(p);
    if(zone==LC_ZONE_UNKNOWN)return false;
    float daylight=lc_environment_daylight(w);
    float draft=door_draft(w,109,p)+door_draft(w,110,p);
    float outside=14.0f+12.0f*daylight;
    /* Explicit provisional scenario: no HVAC, thermal mass, weather service,
     * or actual meteorological measurements. Indoor base is 21 C. */
    *out=(LCEnvironmentSample){w->environment_elapsed_ms,lc_environment_time_of_day(w),zone,
        21.0f+(outside-21.0f)*clamp(draft*.55f,0,.8f),.04f+draft,
        lc_environment_light_at(w,p),artificial(w,zone)};
    return true;
}
