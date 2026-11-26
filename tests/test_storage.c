#include "storage.h"
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
static LCCommand use(uint32_t id,LCAction action){
    return (LCCommand){.api_version=LC_COMMAND_API_VERSION,.kind=LC_COMMAND_USE,.input.use={id,action}};
}
static LCVec center(LCBox b){return (LCVec){(b.min.x+b.max.x)*.5f,(b.min.y+b.max.y)*.5f,(b.min.z+b.max.z)*.5f};}
int main(void){
    LCWorld w;lc_house(&w);LCCommandReceipt r;LCHit h;
    LCObject *book=lc_find_object(&w,210),*shelf=lc_find_object(&w,205);
    assert(book&&shelf&&lc_container_capacity(shelf)==9&&lc_storage_valid(&w));
    LCBox panels[LC_CONTAINER_PANELS];
    assert(lc_container_panels(shelf,panels)==1);
    book->progress=17;
    w.player=(LCPose){{1.85f,3,3.25f},0,0};
    aim(&w,center(book->shape));
    assert(lc_use(&w,210,LC_ACTION_PICK_UP));
    w.player=(LCPose){{7.1f,3,1.15f},0,0};
    aim(&w,(LCVec){7.1f,4.35f,.85f});
    LCCommand cmd=use(205,LC_ACTION_STORE);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_REJECTED&&w.held_object_id==210);
    assert(lc_use(&w,205,LC_ACTION_TOGGLE));
    assert(lc_container_panels(shelf,panels)==7);
    assert(lc_execute(&w,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(r.requested_object_id==205&&r.before_object.id==210&&r.after_object.id==210);
    assert(r.before_object.location==LC_LOCATION_HELD&&r.after_object.location==LC_LOCATION_CONTAINER);
    assert(book->container_id==205&&book->container_slot==1&&book->progress==17&&!w.held_object_id);
    assert(lc_storage_valid(&w));
    assert(!lc_move_object(&w,210,w.player.position,(LCVec){7,3,1}));
    aim(&w,center(book->shape));
    assert(lc_focus(&w,1.5f,&h)&&h.object_id==210);
    assert(lc_toggle_object(&w,205,w.player.position));
    assert(!lc_storage_visible(&w,book));
    assert(lc_focus(&w,1.5f,&h)&&h.object_id==205);
    assert(!lc_use(&w,210,LC_ACTION_PICK_UP));
    const char *path="/tmp/livingcolors_storage_test.save";
    assert(lc_save(&w,path));
    LCWorld fresh;lc_house(&fresh);assert(lc_load(&fresh,path));
    book=lc_find_object(&fresh,210);shelf=lc_find_object(&fresh,205);
    assert(book->container_id==205&&book->progress==17&&lc_storage_valid(&fresh));
    assert(!(shelf->flags&LC_OBJ_OPEN)&&!lc_storage_visible(&fresh,book));
    assert(lc_use(&fresh,205,LC_ACTION_TOGGLE));
    assert(lc_focus(&fresh,1.5f,&h)&&h.object_id==210);
    cmd=use(210,LC_ACTION_PICK_UP);
    assert(lc_execute(&fresh,&cmd,&r)==LC_COMMAND_APPLIED);
    assert(r.before_object.location==LC_LOCATION_CONTAINER&&r.after_object.location==LC_LOCATION_HELD);
    assert(r.before_object.container_id==205&&fresh.held_object_id==210&&!book->container_id);
    /* No access from behind; failed store preserves the same held item. */
    fresh.player=(LCPose){{7.1f,0,1.15f},0,0};
    assert(!lc_store_held(&fresh,205)&&fresh.held_object_id==210);
    fresh.player=(LCPose){{7.1f,3,1.15f},0,0};
    aim(&fresh,(LCVec){7.1f,4.35f,.85f});
    assert(lc_store_held(&fresh,205));
    book->container_slot=99;
    assert(!lc_storage_valid(&fresh)&&!lc_save(&fresh,path));
    book->container_slot=1;
    assert(lc_storage_valid(&fresh));
    /* A second item must occupy a distinct accessible slot, not a remote free one. */
    fresh.player=(LCPose){{8.95f,3,1.15f},0,0};
    LCObject other={.id=999,.name="Second reader",.kind=LC_BOOK,
        .shape={{8.8f,3,1.5f},{9.15f,3.04f,1.8f}},.flags=LC_OBJ_MOVABLE};
    assert(lc_add_object(&fresh,other));
    fresh.held_object_id=999; /* unit-test fixture: already holding a second object */
    aim(&fresh,(LCVec){8.95f,4.35f,.85f});
    assert(lc_store_held(&fresh,205));
    LCObject *stored=lc_find_object(&fresh,999);
    assert(stored&&stored->container_id==205&&stored->container_slot!=1&&lc_storage_valid(&fresh));
    unsigned slot=stored->container_slot;
    stored->container_slot=1;
    assert(!lc_storage_valid(&fresh));
    stored->container_slot=slot;
    assert(lc_storage_valid(&fresh));
    assert(remove(path)==0);
    puts("Living Colors: physical storage, occlusion, receipts and persistence tests passed");
    return 0;
}
