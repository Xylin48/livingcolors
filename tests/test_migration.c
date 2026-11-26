#include "world.h"
#include "body.h"
#include "interaction.h"
#include "storage.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
/* Historical file layouts intentionally reproduced here, independently of the
 * current writer. v1/v2 snapshots predate the chair, v3 includes it. */
static void write_legacy(const LCWorld *w,const char *path,unsigned format){
    FILE *f=fopen(path,"w");assert(f);
    size_t count=w->object_count;
    if(format<3)count--;
    assert(fprintf(f,"LIVINGCOLORS %u\n%a %a %a %a %a\n",format,
        (double)w->player.position.x,(double)w->player.position.y,
        (double)w->player.position.z,(double)w->player.yaw,(double)w->player.pitch)>0);
    if(format>=2)assert(fprintf(f,"%u\n",w->held_object_id)>0);
    if(format>=3)assert(fprintf(f,"%u %u %a %a %a\n",(unsigned)w->posture,w->support_id,
        (double)w->exit_position.x,(double)w->exit_position.y,(double)w->exit_position.z)>0);
    assert(fprintf(f,"%zu\n",count)>0);
    for(size_t i=0;i<w->object_count;i++){
        const LCObject *o=&w->objects[i];
        if(format<3&&o->id==215)continue;
        assert(fprintf(f,"%u %u %u %a %a %a %a %a %a\n",o->id,o->flags,o->progress,
          (double)o->shape.min.x,(double)o->shape.min.y,(double)o->shape.min.z,
          (double)o->shape.max.x,(double)o->shape.max.y,(double)o->shape.max.z)>0);
    }
    assert(fputs("END\n",f)>=0);assert(fclose(f)==0);
}
int main(void){
    const char *path="/tmp/livingcolors_migration_test.save";
    LCWorld w,fresh;lc_house(&w);
    LCObject *book=lc_find_object(&w,210);
    assert(book);book->progress=21;
    lc_find_object(&w,103)->flags|=LC_OBJ_POWERED;
    lc_find_object(&w,107)->flags|=LC_OBJ_OPEN;
    write_legacy(&w,path,1);
    lc_house(&fresh);assert(lc_load(&fresh,path));
    assert(lc_find_object(&fresh,215)&&lc_find_object(&fresh,210)->progress==21);
    assert(fresh.posture==LC_STANDING&&!fresh.held_object_id);
    assert(lc_find_object(&fresh,103)->flags&LC_OBJ_POWERED);
    assert(lc_find_object(&fresh,107)->flags&LC_OBJ_OPEN);
    w.held_object_id=210;
    write_legacy(&w,path,2);
    lc_house(&fresh);assert(lc_load(&fresh,path));
    assert(fresh.held_object_id==210&&fresh.posture==LC_STANDING&&lc_storage_valid(&fresh));
    assert(lc_find_object(&fresh,210)->progress==21);
    assert(!lc_find_object(&fresh,210)->container_id);
    w.player=(LCPose){{1.9f,0,6.5f},-1.5707963f,-.65f};
    assert(lc_body_enter(&w,101,LC_SEATED));
    write_legacy(&w,path,3);
    lc_house(&fresh);assert(lc_load(&fresh,path));
    assert(fresh.held_object_id==210&&fresh.posture==LC_SEATED&&fresh.support_id==101);
    assert(lc_body_valid(&fresh)&&lc_body_stand(&fresh)&&lc_storage_valid(&fresh));
    assert(fresh.held_object_id==210&&lc_find_object(&fresh,210)->progress==21);
    assert(lc_save(&fresh,path));lc_house(&w);assert(lc_load(&w,path));
    assert(w.held_object_id==210&&w.posture==LC_STANDING&&lc_find_object(&w,210)->progress==21);
    assert(remove(path)==0);
    puts("Living Colors: independent v1/v2/v3 snapshot migration and v4 round trip tests passed");
    return 0;
}
