#include "frame.h"
#include "scene.h"
#include "body.h"
#include "environment.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t rgb[LC_FRAME_RGB_BYTES],other[LC_FRAME_RGB_BYTES];
static float depth[LC_FRAME_PIXELS];
static size_t pixel(unsigned x,unsigned y){return (size_t)y*LC_VIEW_WIDTH+x;}
static void compare_depth(const LCWorld *w,unsigned x,unsigned y){
    LCHit h;
    size_t i=pixel(x,y);
    bool hit=lc_view_sample(w,(x+.5f)/LC_VIEW_WIDTH,(y+.5f)/LC_VIEW_HEIGHT,50,&h);
    if(!hit){assert(depth[i]==0);return;}
    LCView v;assert(lc_world_view(w,&v));
    float z=(h.point.x-v.origin.x)*v.forward.x+(h.point.y-v.origin.y)*v.forward.y+
            (h.point.z-v.origin.z)*v.forward.z;
    assert(z>.01f&&depth[i]>0&&fabsf(1/depth[i]-z)<.003f);
}
int main(void){
    LCWorld w;lc_house(&w);
    assert(!lc_frame_render(&w,rgb,sizeof(rgb)-1,depth,LC_FRAME_PIXELS));
    assert(!lc_frame_render(&w,rgb,sizeof(rgb),depth,LC_FRAME_PIXELS-1));
    assert(lc_frame_render(&w,rgb,sizeof(rgb),depth,LC_FRAME_PIXELS));
    for(unsigned y=30;y<450;y+=39)for(unsigned x=31;x<610;x+=47)compare_depth(&w,x,y);
    memcpy(other,rgb,sizeof(rgb));
    assert(lc_frame_render(&w,rgb,sizeof(rgb),depth,LC_FRAME_PIXELS)&&!memcmp(rgb,other,sizeof(rgb)));
    assert(lc_environment_advance(&w,16u*3600000u));
    assert(lc_frame_render(&w,rgb,sizeof(rgb),depth,LC_FRAME_PIXELS)&&memcmp(rgb,other,sizeof(rgb)));
    w.player.yaw=3.14159265f;
    assert(lc_frame_render(&w,rgb,sizeof(rgb),depth,LC_FRAME_PIXELS));
    compare_depth(&w,320,240);
    float closed=depth[pixel(320,240)];
    assert(lc_use(&w,110,LC_ACTION_TOGGLE));
    assert(lc_frame_render(&w,rgb,sizeof(rgb),depth,LC_FRAME_PIXELS));
    compare_depth(&w,320,240);
    assert(closed>0&&depth[pixel(320,240)]==0);
    lc_house(&w);w.player=(LCPose){{4.8f,3,5.1f},0,.25f};
    assert(lc_frame_render(&w,rgb,sizeof(rgb),depth,LC_FRAME_PIXELS));
    for(unsigned y=30;y<450;y+=43)for(unsigned x=31;x<610;x+=53)compare_depth(&w,x,y);
    assert(w.environment_elapsed_ms==0);
    puts("Living Colors: headless RGB rendering, depth agreement, lighting and occlusion tests passed");
    return 0;
}
