#include "environment.h"
#include "command.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void aim(LCWorld *w,LCVec p){
    LCVec e=lc_eye(&w->player);
    float x=p.x-e.x,y=p.y-e.y,z=p.z-e.z;
    w->player.yaw=atan2f(x,-z);w->player.pitch=atan2f(y,hypotf(x,z));
}
int main(void){
    LCWorld w;lc_house(&w);LCEnvironmentSample a,b;
    assert(lc_environment_sample(&w,&a)&&a.zone==LC_ZONE_LIVING);
    assert(a.time_of_day_ms==28800000&&a.elapsed_ms==0&&a.temperature_c==21);
    assert(!a.artificial_light&&a.light_level>.3f);
    assert(lc_environment_advance(&w,16u*3600000u));
    assert(lc_environment_sample(&w,&b)&&b.time_of_day_ms==0&&b.light_level<a.light_level);
    assert(b.elapsed_ms==16u*3600000u);
    assert(lc_environment_sample(&w,&a)&&a.elapsed_ms==b.elapsed_ms);
    aim(&w,(LCVec){4.97f,1.18f,9.82f});
    w.player.position=(LCVec){5.05f,0,8.9f};
    aim(&w,(LCVec){4.97f,1.18f,9.82f});
    LCCommand cmd={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_USE,.input.use={301,LC_ACTION_TOGGLE}};
    LCCommandReceipt receipt;
    assert(lc_execute(&w,&cmd,&receipt)==LC_COMMAND_APPLIED);
    assert(lc_environment_sample(&w,&a)&&a.artificial_light&&a.light_level>b.light_level);
    assert(lc_environment_light_at(&w,(LCVec){7,4,2})<a.light_level);
    w.player=(LCPose){{6,0,8.8f},0,0};
    assert(lc_environment_sample(&w,&a));
    assert(lc_toggle_object(&w,110,w.player.position));
    assert(lc_environment_sample(&w,&b));
    assert(b.air_speed_mps>a.air_speed_mps&&b.temperature_c<a.temperature_c);
    w.player=(LCPose){{7,3,2},0,0};
    assert(lc_environment_sample(&w,&a)&&a.zone==LC_ZONE_BEDROOM);
    assert(a.air_speed_mps<.05f&&!a.artificial_light);
    const char *path="/tmp/livingcolors_environment_test.save";
    assert(lc_save(&w,path));LCWorld fresh;lc_house(&fresh);
    assert(lc_load(&fresh,path));
    assert(fresh.environment_elapsed_ms==w.environment_elapsed_ms);
    assert(lc_find_object(&fresh,301)->flags&LC_OBJ_POWERED);
    assert(lc_find_object(&fresh,110)->flags&LC_OBJ_OPEN);
    fresh.environment_elapsed_ms=UINT64_MAX-1;
    assert(!lc_environment_advance(&fresh,2)&&fresh.environment_elapsed_ms==UINT64_MAX-1);
    assert(!lc_environment_advance(&fresh,86400001u));
    assert(remove(path)==0);
    puts("Living Colors: explicit time, local lighting, temperature, airflow and persistence tests passed");
    return 0;
}
