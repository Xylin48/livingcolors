/* Standalone manual inspection viewer. The authoritative world state lives in world.c,
 * not in this renderer. No model is required to navigate or test the geometry. */
#include "world.h"
#include "command.h"
#include "perception.h"
#include "body.h"
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
static void draw_staircase(void) {
    /* The same treads are used by the virtual sight ray. */
    for(size_t i=0;i<LC_STAIR_TREADS;i++){
        LCBox step;
        if(lc_stair_tread(i,&step))draw_box(step,(Color){119,78,46,255});
    }
}
static void draw_scene(const LCWorld *w) {
    draw_staircase();
    for(size_t i=0;i<w->count;i++) draw_box(w->solids[i].shape,color(w->solids[i].color));
    for(size_t i=0;i<w->object_count;i++) {
        const LCObject *o=&w->objects[i];
        if(o->id==w->held_object_id)continue;
        Color c=color(o->color);
        if(o->kind==LC_DEVICE && (o->flags&LC_OBJ_POWERED))c=(Color){73,132,126,255};
        draw_box(lc_object_shape(o),c);
    }
}

int main(void) {
    LCWorld w;
    lc_house(&w);
    if(lc_load(&w,"livingcolors.save"))puts("Restored previous Living Colors session");
    InitWindow((int)LC_VIEW_WIDTH,(int)LC_VIEW_HEIGHT,"Living Colors - standalone world prototype");
    SetTargetFPS(30);
    DisableCursor();
    while(!WindowShouldClose()) {
        float dt=fminf(GetFrameTime(),.05f);
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
        if(IsKeyPressed(KEY_F5))printf("Save: %s\n",lc_save(&w,"livingcolors.save")?"OK":"FAILED");
        if(IsKeyPressed(KEY_F9))printf("Load: %s\n",lc_load(&w,"livingcolors.save")?"OK":"FAILED");
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
        ClearBackground((Color){137,164,176,255});
        BeginMode3D(camera);
        draw_scene(&w);
        EndMode3D();
        EndDrawing();
    }
    (void)lc_save(&w,"livingcolors.save");
    CloseWindow();
    return 0;
}
