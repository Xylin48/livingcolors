/* Standalone manual inspection viewer. The authoritative world state lives in world.c,
 * not in this renderer. No model is required to navigate or test the geometry. */
#include "world.h"
#include "command.h"
#include "perception.h"
#include "body.h"
#include "storage.h"
#include "environment.h"
#include "scene.h"
#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdio.h>


static Color color(LCColor c) {return (Color){c.r,c.g,c.b,255};}
static Vector3 center(LCBox b) {
    return (Vector3){(b.min.x+b.max.x)*.5f,(b.min.y+b.max.y)*.5f,(b.min.z+b.max.z)*.5f};
}
static Vector3 extent(LCBox b) {
    return (Vector3){b.max.x-b.min.x,b.max.y-b.min.y,b.max.z-b.min.z};
}
static void draw_box(LCBox b,Color c) {
    DrawCubeV(center(b),extent(b),c);
    /* Small outlines make individual pieces recognizable at 640x480. */
    DrawCubeWiresV(center(b),extent(b),ColorBrightness(c,-.30f));
}
static void draw_part(const LCScenePart *p,void *ctx){
    draw_box(p->bounds,color(lc_scene_lit_color(ctx,p)));
}
static void draw_scene(const LCWorld *w){
    /* Visitor is read-only; the context points to the same world. */
    (void)lc_scene_visit(w,draw_part,(void *)w);
}

int main(void) {
    LCWorld w;
    lc_house(&w);
    if(lc_load(&w,"livingcolors.save"))puts("Restored previous Living Colors session");
    InitWindow((int)LC_VIEW_WIDTH,(int)LC_VIEW_HEIGHT,"Living Colors - standalone world prototype");
    SetTargetFPS(30);
    DisableCursor();
    double time_remainder_ms=0;
    while(!WindowShouldClose()) {
        float frame=GetFrameTime();
        if(isfinite(frame)&&frame>0){
            time_remainder_ms+=fminf(frame,1.0f)*1000.0;
            uint32_t ms=(uint32_t)time_remainder_ms;
            if(lc_environment_advance(&w,ms))time_remainder_ms-=ms;
            else time_remainder_ms=0;
        }
        float dt=fminf(frame,.05f);
        Vector2 mouse=GetMouseDelta();
        LCCommandReceipt receipt;
        (void)lc_execute(&w,&(LCCommand){.api_version=LC_COMMAND_API_VERSION,
            .kind=LC_COMMAND_LOOK,.input.look={mouse.x*.0025f,-mouse.y*.0025f}},&receipt);
        float forward=(IsKeyDown(KEY_W)?1.0f:0.0f)-(IsKeyDown(KEY_S)?1.0f:0.0f);
        float right=(IsKeyDown(KEY_D)?1.0f:0.0f)-(IsKeyDown(KEY_A)?1.0f:0.0f);
        (void)lc_execute(&w,&(LCCommand){.api_version=LC_COMMAND_API_VERSION,
            .kind=LC_COMMAND_WALK,.input.walk={forward,right,dt}},&receipt);
        if(IsKeyPressed(KEY_E) || IsKeyPressed(KEY_P)) {
            LCHit hit;
            if(lc_focus(&w,1.5f,&hit) && hit.object_id) {
                LCAction action=IsKeyPressed(KEY_E)?LC_ACTION_TOGGLE:LC_ACTION_READ_NEXT;
                if(lc_execute(&w,&(LCCommand){.api_version=LC_COMMAND_API_VERSION,
                    .kind=LC_COMMAND_USE,.input.use={hit.object_id,action}},&receipt)==LC_COMMAND_APPLIED)
                    printf("Interacted with object %u\n",receipt.after_object.id);
            }
        }
        if(IsKeyPressed(KEY_G)){
            LCHit hit;
            uint32_t id=w.held_object_id;
            LCAction action=id?LC_ACTION_PLACE:LC_ACTION_PICK_UP;
            if(!id&&lc_focus(&w,1.5f,&hit))id=hit.object_id;
            if(id&&lc_execute(&w,&(LCCommand){.api_version=LC_COMMAND_API_VERSION,
                .kind=LC_COMMAND_USE,.input.use={id,action}},&receipt)==LC_COMMAND_APPLIED)
                puts(receipt.after_held_id?"Picked up object":"Placed object");
        }
        if(IsKeyPressed(KEY_T)&&w.held_object_id){
            LCHit hit;
            if(lc_focus(&w,1.5f,&hit)){
                LCObject *o=lc_find_object(&w,hit.object_id);
                uint32_t id=o?(o->container_id?o->container_id:o->id):0;
                if(id&&lc_execute(&w,&(LCCommand){.api_version=LC_COMMAND_API_VERSION,
                    .kind=LC_COMMAND_USE,.input.use={id,LC_ACTION_STORE}},&receipt)==LC_COMMAND_APPLIED)
                    printf("Stored object %u in %u\n",receipt.after_object.id,id);
            }
        }
        if(IsKeyPressed(KEY_R)){
            uint32_t id=w.support_id;
            LCAction action=LC_ACTION_STAND;
            if(!id){
                LCHit hit;
                if(lc_focus(&w,1.5f,&hit)){
                    LCPosture p=lc_furniture_posture(lc_find_object(&w,hit.object_id));
                    if(p!=LC_STANDING){id=hit.object_id;action=p==LC_SEATED?LC_ACTION_SIT:LC_ACTION_REST;}
                }
            }
            if(id&&lc_execute(&w,&(LCCommand){.api_version=LC_COMMAND_API_VERSION,
               .kind=LC_COMMAND_USE,.input.use={id,action}},&receipt)==LC_COMMAND_APPLIED)
                printf("Posture: %u\n",(unsigned)receipt.after_posture);
        }
        if(IsKeyPressed(KEY_I)){
            LCCommand sense={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_SENSE,
                             .input.sense={LC_SENSE_ENVIRONMENT,0}};
            if(lc_execute(&w,&sense,&receipt)==LC_COMMAND_OBSERVED){
                LCEnvironmentSample s=receipt.environment_sample;
                printf("Environment: zone=%u, temperature=%.1f C, air=%.2f m/s, light=%.2f\n",
                       (unsigned)s.zone,s.temperature_c,s.air_speed_mps,s.light_level);
            }
        }
        if(IsKeyPressed(KEY_H)){
            LCSenseKind mode=w.held_object_id?LC_SENSE_HELD_CONTACT:
                w.support_id?LC_SENSE_SUPPORT_CONTACT:LC_SENSE_FOCUSED_CONTACT;
            LCCommand sense={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_SENSE,.input.sense={mode,0}};
            if(lc_execute(&w,&sense,&receipt)==LC_COMMAND_OBSERVED){
                LCContactSample s=receipt.contact_sample;
                printf("Contact: object=%u, material=%u, known=%u\n",s.object_id,
                       (unsigned)s.surface.material,(unsigned)s.surface.known);
            }
        }
        if(IsKeyPressed(KEY_J)){
            LCCommand sense={.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_SENSE,
                             .input.sense={LC_SENSE_AUDIO,0}};
            if(lc_execute(&w,&sense,&receipt)==LC_COMMAND_OBSERVED){
                printf("Audible simulated sources: %zu\n",receipt.audio_sample.count);
                for(size_t i=0;i<receipt.audio_sample.count;i++){
                    LCSoundCue c=receipt.audio_sample.cues[i];
                    printf("  source=%u bearing=%.2f distance=%.2f gain=%.3f obstructed=%u\n",
                           c.source_id,c.relative_bearing_rad,c.distance_m,c.gain,(unsigned)c.obstructed);
                }
            }
        }
        if(IsKeyPressed(KEY_F5))printf("Save: %s\n",lc_save(&w,"livingcolors.save")?"OK":"FAILED");
        if(IsKeyPressed(KEY_F9)){
            bool loaded=lc_load(&w,"livingcolors.save");
            if(loaded)time_remainder_ms=0;
            printf("Load: %s\n",loaded?"OK":"FAILED");
        }
        /* Read the current pose AFTER any load or interaction, not a stale yaw. */
        LCView view;
        if(!lc_world_view(&w,&view))continue;
        Camera3D camera={0};
        camera.position=(Vector3){view.origin.x,view.origin.y,view.origin.z};
        camera.target=(Vector3){view.origin.x+view.forward.x,view.origin.y+view.forward.y,
                                view.origin.z+view.forward.z};
        camera.up=(Vector3){view.up.x,view.up.y,view.up.z};
        camera.fovy=view.vertical_fov_degrees;
        camera.projection=CAMERA_PERSPECTIVE;
        BeginDrawing();
        ClearBackground(color(lc_scene_sky(&w)));
        BeginMode3D(camera);
        draw_scene(&w);
        EndMode3D();
        EndDrawing();
    }
    (void)lc_save(&w,"livingcolors.save");
    CloseWindow();
    return 0;
}
