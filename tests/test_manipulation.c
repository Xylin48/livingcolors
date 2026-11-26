#include "interaction.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void aim(LCWorld *w,LCVec target){
    LCVec eye=lc_eye(&w->player);
    float x=target.x-eye.x,y=target.y-eye.y,z=target.z-eye.z;
    w->player.yaw=atan2f(x,-z);
    w->player.pitch=atan2f(y,hypotf(x,z));
}
static void v1(const LCWorld *w,const char *path){
    FILE *f=fopen(path,"w");assert(f);
    assert(fprintf(f,"LIVINGCOLORS 1\n%a %a %a %a %a\n%zu\n",
        (double)w->player.position.x,(double)w->player.position.y,(double)w->player.position.z,
        (double)w->player.yaw,(double)w->player.pitch,w->object_count)>0);
    for(size_t i=0;i<w->object_count;i++){
        const LCObject *o=&w->objects[i];
        assert(fprintf(f,"%u %u %u %a %a %a %a %a %a\n",o->id,o->flags,o->progress,
          (double)o->shape.min.x,(double)o->shape.min.y,(double)o->shape.min.z,
          (double)o->shape.max.x,(double)o->shape.max.y,(double)o->shape.max.z)>0);
    }
    assert(fputs("END\n",f)>=0);assert(fclose(f)==0);
}
int main(void){
    LCWorld w;lc_house(&w);
    const char *path="/tmp/livingcolors_manipulation_test.save";
    LCHit h;
    assert(lc_cast(&w,(LCVec){7,1,5},(LCVec){0,-1,0},2,&h)&&h.object_id==0);
    assert(fabsf(h.point.y)<.001f);
    assert(lc_cast(&w,(LCVec){10.8f,3.5f,7},(LCVec){0,-1,0},4,&h)&&h.object_id==0);
    assert(h.point.y>1.3f&&h.point.y<2.0f);
    w.player=(LCPose){{1.85f,3,3.25f},0,0};
    LCObject *book=lc_find_object(&w,210);
    assert(book);book->progress=7;
    aim(&w,(LCVec){.82f,3.78f,3.25f});
    assert(lc_use(&w,210,LC_ACTION_PICK_UP));
    assert(w.held_object_id==210&&book->progress==7);
    assert(!lc_use(&w,210,LC_ACTION_PICK_UP));
    assert(!lc_move_object(&w,210,w.player.position,(LCVec){2,3,3}));
    assert(lc_focus(&w,1.5f,&h)&&h.object_id!=210);
    assert(lc_save(&w,path));
    LCWorld restored;lc_house(&restored);
    assert(lc_load(&restored,path));
    assert(restored.held_object_id==210&&lc_find_object(&restored,210)->progress==7);
    w=restored;book=lc_find_object(&w,210);
    w.player.yaw=0;w.player.pitch=0;
    assert(!lc_place_held(&w)&&w.held_object_id==210);
    aim(&w,(LCVec){1.53f,3.75f,3.4f});
    assert(!lc_place_held(&w)&&w.held_object_id==210); /* overhanging desk edge */
    aim(&w,(LCVec){1.05f,3.75f,3.5f});
    assert(lc_use(&w,210,LC_ACTION_PLACE));
    assert(!w.held_object_id&&book->progress==7);
    assert(fabsf(book->shape.min.y-3.75f)<.001f);
    assert(lc_save(&w,path));
    lc_house(&restored);assert(lc_load(&restored,path));
    assert(!restored.held_object_id&&lc_find_object(&restored,210)->progress==7);
    /* A v1 file remains readable after a new movable object is added. */
    v1(&w,path);
    lc_house(&restored);
    LCObject extra={.id=999,.name="New item",.kind=LC_BOOK,
        .shape={{7,3,.9f},{7.2f,3.1f,1.1f}},.flags=LC_OBJ_MOVABLE};
    assert(lc_add_object(&restored,extra));
    assert(lc_load(&restored,path));
    assert(lc_find_object(&restored,999)&&lc_find_object(&restored,210)->progress==7);
    LCBox valid=book->shape;
    book->shape.max.x+=.1f;v1(&w,path);
    assert(!lc_load(&restored,path));
    book->shape=valid;
    book->shape.min.x-=.5f;book->shape.max.x-=.5f;
    book->shape.min.y-=.2f;book->shape.max.y-=.2f;v1(&w,path);
    assert(!lc_load(&restored,path));
    assert(lc_find_object(&restored,210)->progress==7);
    assert(remove(path)==0);
    puts("Living Colors: shared geometry, pickup, supported placement and backward save tests passed");
    return 0;
}
