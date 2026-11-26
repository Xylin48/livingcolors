/* Standalone manual inspection viewer. The authoritative world state lives in world.c,
 * not in this renderer. No model is required to navigate or test the geometry. */
#include "world.h"
#include "command.h"
#include "perception.h"
#include "body.h"
#include "storage.h"
#include "environment.h"
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
static Color lit(const LCWorld *w,LCBox b,Color c){
    Vector3 p=center(b);
    float level=lc_environment_light_at(w,(LCVec){p.x,p.y,p.z});
    return (Color){(unsigned char)(c.r*level),(unsigned char)(c.g*level),(unsigned char)(c.b*level),c.a};
}
static void draw_staircase(const LCWorld *w) {
    /* The same treads are used by the virtual sight ray. */
    for(size_t i=0;i<LC_STAIR_TREADS;i++){
        LCBox step;
        if(lc_stair_tread(i,&step))draw_box(step,lit(w,step,(Color){119,78,46,255}));
    }
}
static void draw_scene(const LCWorld *w) {
    draw_staircase(w);
    for(size_t i=0;i<w->count;i++) draw_box(w->solids[i].shape,lit(w,w->solids[i].shape,color(w->solids[i].color)));
    for(size_t i=0;i<w->object_count;i++) {
        const LCObject *o=&w->objects[i];
        if(!lc_storage_visible(w,o))continue;
        Color c=color(o->color);
        if(o->kind==LC_DEVICE && (o->flags&LC_OBJ_POWERED))c=(Color){73,132,126,255};
        LCBox panels[LC_CONTAINER_PANELS];
        size_t n=lc_container_panels(o,panels);
        if(!n){panels[0]=lc_object_shape(o);n=1;}
        for(size_t j=0;j<n;j++)draw_box(panels[j],lit(w,panels[j],c));
    }
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
        float sky=.10f+.90f*lc_environment_daylight(&w);
        ClearBackground((Color){(unsigned char)(137*sky),(unsigned char)(164*sky),(unsigned char)(176*sky),255});
        BeginMode3D(camera);
        draw_scene(&w);
        EndMode3D();
        EndDrawing();
    }
    (void)lc_save(&w,"livingcolors.save");
    CloseWindow();
    return 0;
}
