#include "perception.h"
#include "body.h"

#include <math.h>

#define LC_PI 3.14159265358979323846f
static bool finite_vec(LCVec v){return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}
bool lc_view(const LCPose *p,LCView *out){
    if(!p||!out||!finite_vec(p->position)||!isfinite(p->yaw)||
       !isfinite(p->pitch)||fabsf(p->pitch)>=LC_PI*.5f)return false;
    const float sy=sinf(p->yaw),cy=cosf(p->yaw),sp=sinf(p->pitch),cp=cosf(p->pitch);
    LCView v={0};
    v.origin=lc_eye(p);
    v.forward=lc_view_direction(p);
    v.right=(LCVec){cy,0,sy};
    v.up=(LCVec){-sy*sp,cp,cy*sp};
    v.vertical_fov_degrees=2.0f*atanf(tanf(LC_VIEW_REFERENCE_FOV_DEG*LC_PI/360.0f)/
                                    LC_VIEW_MAGNIFICATION)*180.0f/LC_PI;
    v.aspect=(float)LC_VIEW_WIDTH/(float)LC_VIEW_HEIGHT;
    if(!finite_vec(v.origin)||!finite_vec(v.forward)||!finite_vec(v.right)||!finite_vec(v.up))
        return false;
    *out=v;return true;
}
bool lc_world_view(const LCWorld *w,LCView *out){
    if(!w)return false;
    LCPose p=lc_body_camera_pose(w);
    return lc_view(&p,out);
}
bool lc_view_ray(const LCPose *pose,float u,float v,LCVec *origin,LCVec *direction){
    LCView camera;
    if(!origin||!direction||origin==direction||!isfinite(u)||!isfinite(v)||
       u<0||u>1||v<0||v>1||!lc_view(pose,&camera))return false;
    const float half=tanf(camera.vertical_fov_degrees*LC_PI/360.0f);
    const float x=(2.0f*u-1.0f)*half*camera.aspect,y=(1.0f-2.0f*v)*half;
    LCVec d={camera.forward.x+x*camera.right.x+y*camera.up.x,
             camera.forward.y+x*camera.right.y+y*camera.up.y,
             camera.forward.z+x*camera.right.z+y*camera.up.z};
    const float n=hypotf(hypotf(d.x,d.y),d.z);
    if(!isfinite(n)||n<=0)return false;
    *origin=camera.origin;*direction=(LCVec){d.x/n,d.y/n,d.z/n};
    return true;
}
bool lc_view_sample(const LCWorld *w,float u,float v,float limit,LCHit *out){
    LCVec o,d;
    if(!w)return false;
    LCPose p=lc_body_camera_pose(w);
    return lc_view_ray(&p,u,v,&o,&d)&&lc_cast(w,o,d,limit,out);
}
