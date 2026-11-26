/* Standalone manual inspection viewer. The authoritative world state lives in world.c,
 * not in this renderer. No model is required to navigate or test the geometry. */
#include "world.h"
#include "command.h"
#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdio.h>

#define LC_PI 3.14159265358979323846f

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
    InitWindow(640,480,"Living Colors - standalone world prototype");
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
        const float yaw=w.player.yaw;
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
        if(IsKeyPressed(KEY_F5))printf("Save: %s\n",lc_save(&w,"livingcolors.save")?"OK":"FAILED");
        if(IsKeyPressed(KEY_F9))printf("Load: %s\n",lc_load(&w,"livingcolors.save")?"OK":"FAILED");
        const float cy=cosf(w.player.pitch),sy=sinf(w.player.pitch);
        LCVec eye=lc_eye(&w.player);
        Vector3 origin={eye.x,eye.y,eye.z};
        Camera3D camera={0};
        camera.position=origin;
        camera.target=(Vector3){origin.x+sinf(yaw)*cy,origin.y+sy,origin.z-cosf(yaw)*cy};
        camera.up=(Vector3){0,1,0};
        /* 125% magnification relative to 60 degree reference field of view. */
        camera.fovy=2.0f*atanf(tanf((60.0f*LC_PI/180.0f)/2.0f)/1.25f)*180.0f/LC_PI;
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
