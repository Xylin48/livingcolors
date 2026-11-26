#include "interaction.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
    LCWorld w;lc_house(&w);LCHit h;
    assert(lc_cast(&w,(LCVec){2,1,5},(LCVec){0,0,-1},10,&h));
    assert(h.object_id==0&&h.distance<1);
    assert(lc_cast(&w,(LCVec){7,1,5},(LCVec){0,1,0},10,&h));
    assert(h.object_id==0&&fabsf(h.distance-1.88f)<.01f);
    assert(!lc_cast(&w,(LCVec){2,1,5},(LCVec){0,0,0},10,&h));
    w.player=(LCPose){{4.8f,3,5.1f},0,0};
    assert(lc_focus(&w,1.5f,&h)&&h.object_id==211);
    assert(!lc_use(&w,204,LC_ACTION_TOGGLE));
    assert(lc_use(&w,211,LC_ACTION_TOGGLE));
    assert(!lc_blocked(&w,(LCVec){4.8f,3,4.35f}));
    assert(lc_blocked(&w,(LCVec){4.17f,3,4.9f}));
    assert(!lc_use(&w,210,LC_ACTION_READ_NEXT));
    w.player=(LCPose){{1.85f,3,3.25f},-1.5707963f,-.55f};
    assert(lc_focus(&w,1.5f,&h)&&h.object_id==210);
    assert(lc_use(&w,210,LC_ACTION_READ_NEXT));
    assert(lc_find_object(&w,210)->progress==1);
    assert(lc_look(&w,0,9));
    assert(w.player.pitch<=1.4f);
    assert(!lc_use(&w,210,LC_ACTION_READ_NEXT));
    puts("Living Colors: authoritative ray and interaction tests passed");
}
