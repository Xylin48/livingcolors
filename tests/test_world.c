#include "world.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool close_to(float a,float b) { return fabsf(a-b)<.03f; }
int main(void) {
    LCWorld w;
    lc_house(&w);
    assert(w.count>12 && w.object_count>=20);
    assert(!lc_blocked(&w,w.player.position));
    assert(!lc_add_box(&w,(LCBox){{2,2,2},{1,3,3}}));
    assert(!lc_add_object(&w,w.objects[0]));
    assert(lc_blocked(&w,(LCVec){-.1f,0,1}));
    assert(lc_blocked(&w,(LCVec){0.5f,0,5.6f})); /* sofa */
    assert(lc_blocked(&w,(LCVec){2.0f,0,4.25f})); /* kitchen wall */
    assert(!lc_blocked(&w,(LCVec){9.0f,0,4.25f})); /* kitchen opening */
    assert(lc_blocked(&w,(LCVec){4.8f,3,4.35f})); /* closed bedroom door */
    assert(lc_toggle_object(&w,211,(LCVec){4.8f,3,5.1f}));
    assert(!lc_blocked(&w,(LCVec){4.8f,3,4.35f})); /* open bedroom door */
    assert(lc_blocked(&w,(LCVec){2.0f,3,4.35f}));
    assert(lc_find_object(&w,212) && lc_find_object(&w,213) && lc_find_object(&w,214));
    assert(close_to(lc_floor_height(10.6f,9.55f,0),0));
    assert(close_to(lc_floor_height(10.6f,4.45f,3),3));

    LCPose p={{7.5f,0,8.5f},0,0};
    assert(lc_move(&w,&p,0,-1,1));
    assert(close_to(p.position.z,6.5f));
    /* Solid wall blocks high-speed progression, even at low frame rate. */
    p=(LCPose){{2,0,5},0,0};
    assert(lc_move(&w,&p,0,-1,1));
    assert(p.position.z>4.5f);
    assert(!lc_move(&w,&p,0,1,-.1f));

    /* Stair path, entry near the front, exit at upper hallway. */
    p=(LCPose){{10.8f,0,9.55f},0,0};
    for(int i=0;i<200;i++)assert(lc_move(&w,&p,0,-1,.03f));
    assert(p.position.y>2.8f);
    assert(p.position.z>=4.45f);
    assert(lc_move(&w,&p,-1,0,.6f));
    assert(p.position.x<10.2f);
    assert(close_to(p.position.y,3.0f));

    LCObject *tv=lc_find_object(&w,103);
    assert(tv && !(tv->flags&LC_OBJ_POWERED));
    assert(!lc_toggle_object(&w,103,(LCVec){0,0,0}));
    assert(lc_toggle_object(&w,103,(LCVec){5.9f,0,6.5f}));
    assert(tv->flags&LC_OBJ_POWERED);
    LCObject *book=lc_find_object(&w,210);
    assert(book && lc_move_object(&w,210,(LCVec){1.6f,3,3.4f},(LCVec){2.1f,3.8f,3.2f}));
    book->progress=23;
    w.player=(LCPose){{6,0,8.8f},.25f,-.1f};
    /* Closed containers remain physically solid even when open. */
    assert(lc_toggle_object(&w,107,(LCVec){5.2f,0,1.3f}));
    assert(lc_find_object(&w,107)->flags&LC_OBJ_OPEN);
    assert(lc_blocked(&w,(LCVec){5.5f,0,.8f}));
    /* A valid save must restore even when player occupies an open doorway. */
    assert(lc_toggle_object(&w,110,(LCVec){6.0f,0,8.85f}));
    w.player.position=(LCVec){6.0f,0,9.7f};
    assert(!lc_blocked(&w,w.player.position));
    const char *save="/tmp/livingcolors_unit_save.dat";
    assert(lc_save(&w,save));
    LCWorld fresh;lc_house(&fresh);
    assert(lc_load(&fresh,save));
    assert(lc_find_object(&fresh,103)->flags&LC_OBJ_POWERED);
    assert(lc_find_object(&fresh,210)->progress==23);
    assert(close_to(lc_find_object(&fresh,210)->shape.min.x,2.1f));
    assert(close_to(fresh.player.yaw,.25f));
    assert(lc_find_object(&fresh,110)->flags&LC_OBJ_OPEN);
    assert(close_to(fresh.player.position.z,9.7f));
    FILE *f=fopen(save,"w");assert(f);
    assert(fputs("broken file\n",f)>=0);assert(fclose(f)==0);
    LCPose before=fresh.player;
    assert(!lc_load(&fresh,save));
    assert(memcmp(&before,&fresh.player,sizeof(before))==0);
    assert(remove(save)==0);
    puts("Living Colors: geometry, collision, stairs, interaction, save/load tests passed");
    return 0;
}
