#include "perception.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static float dot(LCVec a,LCVec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static bool close_to(float a,float b){return fabsf(a-b)<.0001f;}
int main(void){
    LCWorld w;lc_house(&w);
    LCView camera;LCVec o,d;
    assert(lc_view(&w.player,&camera));
    assert(LC_VIEW_WIDTH==640&&LC_VIEW_HEIGHT==480);
    assert(close_to(camera.aspect,4.0f/3.0f));
    assert(camera.vertical_fov_degrees>49&&camera.vertical_fov_degrees<50);
    assert(lc_view_ray(&w.player,.5f,.5f,&o,&d));
    assert(close_to(o.y,LC_EYE_HEIGHT)&&close_to(d.z,-1)&&close_to(d.y,0));
    assert(lc_view_ray(&w.player,1,.5f,&o,&d));
    assert(d.x>0&&close_to(d.x/-d.z,.6158403f));
    assert(lc_view_ray(&w.player,.5f,0,&o,&d)&&d.y>0);
    assert(lc_view_ray(&w.player,.5f,1,&o,&d)&&d.y<0);
    assert(!lc_view_ray(&w.player,1.01f,.5f,&o,&d));
    assert(!lc_view_ray(&w.player,NAN,.5f,&o,&d));
    assert(!lc_view_ray(&w.player,.5f,.5f,&o,&o));
    w.player.yaw=.7f;w.player.pitch=.4f;
    assert(lc_view(&w.player,&camera));
    assert(close_to(dot(camera.forward,camera.right),0));
    assert(close_to(dot(camera.forward,camera.up),0));
    assert(close_to(dot(camera.right,camera.up),0));
    assert(close_to(dot(camera.forward,camera.forward),1));
    assert(lc_view_ray(&w.player,.5f,.5f,&o,&d));
    assert(close_to(dot(d,lc_view_direction(&w.player)),1));
    w.player=(LCPose){{4.8f,3,5.1f},0,0};LCHit h,g;
    assert(lc_view_sample(&w,.5f,.5f,3,&h));
    assert(lc_focus(&w,3,&g));
    assert(h.object_id==211&&h.object_id==g.object_id&&close_to(h.distance,g.distance));
    assert(lc_use(&w,211,LC_ACTION_TOGGLE));
    assert(!lc_view_sample(&w,.5f,.5f,.9f,&h)); /* same ray now passes through doorway */
    w.player=(LCPose){{2,0,5},0,0};
    assert(lc_view_sample(&w,.5f,.5f,10,&h)&&h.object_id==0&&h.distance<1);
    w.player.yaw=NAN;assert(!lc_view(&w.player,&camera));
    puts("Living Colors: calibrated camera and occluded viewport ray tests passed");
    return 0;
}
