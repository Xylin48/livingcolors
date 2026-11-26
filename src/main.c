/* Standalone manual inspection viewer. The authoritative world state lives in world.c,
 * not in this renderer. No model is required to navigate or test the geometry. */
#include "world.h"
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
    const int steps=16;
    const float z_start=9.55f,z_end=4.45f;
    for(int i=0;i<steps;i++) {
        const float t=(float)(i+1)/steps;
        const float z0=z_start-(z_start-z_end)*(i+1)/steps;
        const float z1=z_start-(z_start-z_end)*i/steps;
        LCBox step={{10.28f,0,z0},{11.72f,LC_UPSTAIRS_Y*t,z1}};
        draw_box(step,(Color){119,78,46,255});
    }
}
static void draw_scene(const LCWorld *w) {
    DrawPlane((Vector3){6,0,5},(Vector2){12,10},(Color){125,102,78,255});
    /* Upper floor slabs, leaving a usable void over the right-side staircase. */
    draw_box((LCBox){{.13f,2.88f,.13f},{10.20f,3.0f,9.87f}},(Color){163,139,105,255});
    draw_box((LCBox){{10.20f,2.88f,.13f},{11.87f,3.0f,4.30f}},(Color){163,139,105,255});
    draw_staircase();
    for(size_t i=0;i<w->count;i++) draw_box(w->solids[i].shape,color(w->solids[i].color));
    for(size_t i=0;i<w->object_count;i++) {
        const LCObject *o=&w->objects[i];
        if(o->kind==LC_DOOR && (o->flags&LC_OBJ_OPEN)) {
            LCBox b=o->shape;
            float width=b.max.x-b.min.x;
            float depth=b.max.z-b.min.z;
            if (width>=depth) {
                b.max.x=b.min.x+.09f;
                if (b.min.z>5.0f) b.min.z=fmaxf(.13f,b.min.z-width);
                else b.max.z=fminf(9.87f,b.min.z+width);
            } else {
                b.max.z=b.min.z+.09f;
                b.max.x=fminf(11.87f,b.min.x+depth);
            }
            draw_box(b,color(o->color));
        } else {
            Color c=color(o->color);
            if(o->kind==LC_DEVICE && (o->flags&LC_OBJ_POWERED))c=(Color){73,132,126,255};
            draw_box(o->shape,c);
        }
    }
}
static LCObject *nearest(LCWorld *w, LCVec pos) {
    LCObject *best=NULL;
    float smallest=1.55f;
    for(size_t i=0;i<w->object_count;i++) {
        LCObject *o=&w->objects[i];
        if(!(o->flags&LC_OBJ_INTERACTIVE))continue;
        LCVec c={(o->shape.min.x+o->shape.max.x)*.5f,
                 (o->shape.min.y+o->shape.max.y)*.5f,
                 (o->shape.min.z+o->shape.max.z)*.5f};
        float d=hypotf(hypotf(c.x-pos.x,c.y-(pos.y+1.0f)),c.z-pos.z);
        if(d<smallest){smallest=d;best=o;}
    }
    return best;
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
        w.player.yaw+=mouse.x*.0025f;
        w.player.pitch-=mouse.y*.0025f;
        w.player.pitch=Clamp(w.player.pitch,-1.40f,1.40f);
        float forward=(IsKeyDown(KEY_W)?1.0f:0.0f)-(IsKeyDown(KEY_S)?1.0f:0.0f);
        float right=(IsKeyDown(KEY_D)?1.0f:0.0f)-(IsKeyDown(KEY_A)?1.0f:0.0f);
        const float yaw=w.player.yaw;
        const float dx=right*cosf(yaw)+forward*sinf(yaw);
        const float dz=right*sinf(yaw)-forward*cosf(yaw);
        (void)lc_move(&w,&w.player,dx,dz,dt);
        if(IsKeyPressed(KEY_E)) {
            LCObject *o=nearest(&w,w.player.position);
            if(o && lc_toggle_object(&w,o->id,w.player.position))
                printf("Interacted with %s (%u)\n",o->name,o->id);
        }
        if(IsKeyPressed(KEY_P)) {
            LCObject *book=lc_find_object(&w,210);
            if(book) {book->progress++;printf("School reader progress: %u\n",book->progress);}
        }
        if(IsKeyPressed(KEY_F5))printf("Save: %s\n",lc_save(&w,"livingcolors.save")?"OK":"FAILED");
        if(IsKeyPressed(KEY_F9))printf("Load: %s\n",lc_load(&w,"livingcolors.save")?"OK":"FAILED");
        const float cy=cosf(w.player.pitch),sy=sinf(w.player.pitch);
        Vector3 origin={w.player.position.x,w.player.position.y+1.35f,w.player.position.z};
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
