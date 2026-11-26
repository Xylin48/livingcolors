#include "command.h"
#include "body.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void aim(LCWorld *w,LCVec p){
    LCPose camera=lc_body_camera_pose(w);LCVec e=lc_eye(&camera);
    float x=p.x-e.x,y=p.y-e.y,z=p.z-e.z;
    w->player.yaw=atan2f(x,-z);w->player.pitch=atan2f(y,hypotf(x,z));
}
static LCCommand sense(LCSenseKind kind,uint32_t expected){
    return (LCCommand){.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_SENSE,
                       .correlation_id=78,.input.sense={kind,expected}};
}
int main(void){
    LCWorld w;lc_house(&w);LCCommandReceipt r;
    LCCommand cmd=sense(LC_SENSE_ENVIRONMENT,0);
    assert(lc_environment_advance(&w,1234));
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_OBSERVED);
    assert(r.has_environment_sample&&!r.has_contact_sample&&r.correlation_id==78);
    assert(r.environment_elapsed_ms==1234&&r.environment_sample.elapsed_ms==1234);
    assert(!r.before_object.id&&!r.after_object.id&&w.environment_elapsed_ms==1234);
    cmd.api_version=3;assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_INVALID&&!r.has_environment_sample);
    cmd=sense(LC_SENSE_FOCUSED_CONTACT,210);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_REJECTED&&!r.has_contact_sample);
    w.player=(LCPose){{1.85f,3,3.25f},0,0};
    aim(&w,(LCVec){.82f,3.78f,3.25f});
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_OBSERVED);
    assert(r.has_contact_sample&&!r.has_environment_sample);
    assert(r.contact_sample.object_id==210&&r.contact_sample.surface.material==LC_MATERIAL_PAPER);
    assert(r.contact_sample.surface.known&&r.contact_sample.has_world_point);
    assert(r.contact_sample.elapsed_ms==1234);
    assert(!w.held_object_id&&!lc_find_object(&w,210)->progress);
    assert(lc_use(&w,210,LC_ACTION_PICK_UP));
    cmd=sense(LC_SENSE_HELD_CONTACT,210);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_OBSERVED);
    assert(r.contact_sample.object_id==210&&!r.contact_sample.has_world_point);
    assert(w.held_object_id==210&&r.before_held_id==r.after_held_id);
    w.player=(LCPose){{2.9f,3,1.5f},0,0};
    aim(&w,(LCVec){2,3.65f,1.5f});
    assert(lc_body_enter(&w,201,LC_RESTING));
    cmd=sense(LC_SENSE_SUPPORT_CONTACT,201);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_OBSERVED);
    assert(r.contact_sample.surface.material==LC_MATERIAL_FABRIC);
    assert(r.contact_sample.has_world_point&&r.contact_sample.point.y==w.player.position.y);
    assert(r.contact_sample.surface.compliance>lc_material_properties(LC_MATERIAL_METAL).compliance);
    assert(r.before_posture==LC_RESTING&&r.after_posture==LC_RESTING);
    assert(lc_body_stand(&w));
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_REJECTED&&!r.has_contact_sample);
    assert(!lc_material_properties(LC_MATERIAL_UNKNOWN).known);
    assert(w.environment_elapsed_ms==1234);
    puts("Living Colors: explicit local sensory queries, contact provenance and no-mutation tests passed");
    return 0;
}
