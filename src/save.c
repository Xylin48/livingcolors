#include "world.h"
#include "body.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Format is intentionally human-readable and uses stable object IDs, not array offsets.
 * Only mutable fields are stored. Geometry comes from the current world definition. */
bool lc_save(const LCWorld *world, const char *path) {
    if (!world || !path || !*path || strlen(path)>4000 ||
        world->object_count>LC_MAX_OBJECTS||world->count>LC_MAX_SOLIDS||
        !lc_body_valid(world)) return false;
    if(world->held_object_id){
        bool valid=false;
        for(size_t i=0;i<world->object_count;i++)
            if(world->objects[i].id==world->held_object_id&&(world->objects[i].flags&LC_OBJ_MOVABLE))valid=true;
        if(!valid)return false;
    }
    size_t n=strlen(path);
    char *tmp=malloc(n+5);
    if(!tmp)return false;
    memcpy(tmp,path,n);
    memcpy(tmp+n,".tmp",5);
    FILE *f=fopen(tmp,"w");
    if(!f){free(tmp);return false;}
    int good=fprintf(f,"LIVINGCOLORS 3\n%a %a %a %a %a\n%u\n%u %u %a %a %a\n%zu\n",
       (double)world->player.position.x,(double)world->player.position.y,
       (double)world->player.position.z,(double)world->player.yaw,
       (double)world->player.pitch,world->held_object_id,(unsigned)world->posture,world->support_id,
       (double)world->exit_position.x,(double)world->exit_position.y,(double)world->exit_position.z,world->object_count)>0;
    for(size_t i=0;i<world->object_count && good;i++) {
        const LCObject *o=&world->objects[i];
        good=fprintf(f,"%u %u %u %a %a %a %a %a %a\n",
          o->id,o->flags,o->progress,
          (double)o->shape.min.x,(double)o->shape.min.y,(double)o->shape.min.z,
          (double)o->shape.max.x,(double)o->shape.max.y,(double)o->shape.max.z)>0;
    }
    if(good)good=fputs("END\n",f)>=0;
    if(fflush(f)!=0)good=0;
    if(fclose(f)!=0)good=0;
    if(good && rename(tmp,path)!=0)good=0;
    if(!good)(void)remove(tmp);
    free(tmp);
    return good!=0;
}

bool lc_load(LCWorld *world, const char *path) {
    if(!world || !path || world->object_count>LC_MAX_OBJECTS || world->count>LC_MAX_SOLIDS)return false;
    FILE *f=fopen(path,"r");if(!f)return false;
    LCWorld staged=*world;
    unsigned format;
    char magic[24]={0};
    if(fscanf(f,"%23s %u",magic,&format)!=2 || strcmp(magic,"LIVINGCOLORS")!=0 || (format!=1 && format!=2 && format!=3))
        goto invalid;
    if(fscanf(f,"%f %f %f %f %f",&staged.player.position.x,
        &staged.player.position.y,&staged.player.position.z,
        &staged.player.yaw,&staged.player.pitch)!=5)goto invalid;
    if(!isfinite(staged.player.yaw)||!isfinite(staged.player.pitch)||
       fabsf(staged.player.pitch)>1.57f)
        goto invalid;
    staged.held_object_id=0;
    if(format>=2 && fscanf(f,"%u",&staged.held_object_id)!=1)goto invalid;
    staged.posture=LC_STANDING;staged.support_id=0;staged.exit_position=(LCVec){0};
    if(format>=3){
        unsigned posture;
        if(fscanf(f,"%u %u %f %f %f",&posture,&staged.support_id,&staged.exit_position.x,
                  &staged.exit_position.y,&staged.exit_position.z)!=5||posture>LC_RESTING||
           !isfinite(staged.exit_position.x)||!isfinite(staged.exit_position.y)||
           !isfinite(staged.exit_position.z))goto invalid;
        staged.posture=(LCPosture)posture;
    }
    size_t count=0;
    /* Older snapshots may predate additive object definitions. Retain caller-supplied state for
     * new IDs (fresh definitions if the caller initialized a fresh world); unknown and duplicate IDs still fail. */
    if(fscanf(f,"%zu",&count)!=1 || count>staged.object_count)goto invalid;
    bool seen[LC_MAX_OBJECTS]={0};
    for(size_t i=0;i<count;i++) {
        unsigned id,flags,progress;
        LCBox b;
        if(fscanf(f,"%u %u %u %f %f %f %f %f %f",
           &id,&flags,&progress,
           &b.min.x,&b.min.y,&b.min.z,&b.max.x,&b.max.y,&b.max.z)!=9)
            goto invalid;
        size_t ix=0;
        for(;ix<staged.object_count;ix++)if(staged.objects[ix].id==id)break;
        if(ix==staged.object_count || seen[ix] ||
           !isfinite(b.min.x)||!isfinite(b.min.y)||!isfinite(b.min.z)||
           !isfinite(b.max.x)||!isfinite(b.max.y)||!isfinite(b.max.z)||
           b.min.x<0||b.min.y<0||b.min.z<0||
           b.max.x>LC_WORLD_WIDTH||b.max.y>LC_UPSTAIRS_Y+3 ||b.max.z>LC_WORLD_DEPTH ||
           b.min.x>=b.max.x||b.min.y>=b.max.y||b.min.z>=b.max.z)
            goto invalid;
        LCObject *o=&staged.objects[ix];
        const unsigned dynamic=LC_OBJ_OPEN|LC_OBJ_POWERED;
        if((flags & ~dynamic)!=(o->flags & ~dynamic) ||
           ((o->kind!=LC_DOOR && o->kind!=LC_CONTAINER) && (flags&LC_OBJ_OPEN)) ||
           (o->kind!=LC_DEVICE && (flags&LC_OBJ_POWERED)))goto invalid;
        if(!(o->flags&LC_OBJ_MOVABLE) && memcmp(&b,&o->shape,sizeof(b))!=0)
            goto invalid;
        if(o->flags&LC_OBJ_MOVABLE){
            if(fabsf((b.max.x-b.min.x)-(o->shape.max.x-o->shape.min.x))>.0001f||
               fabsf((b.max.y-b.min.y)-(o->shape.max.y-o->shape.min.y))>.0001f||
               fabsf((b.max.z-b.min.z)-(o->shape.max.z-o->shape.min.z))>.0001f)goto invalid;
        }
        o->shape=b; o->flags=flags; o->progress=progress;
        seen[ix]=true;
    }
    char ending[8]={0},excess[2]={0};
    if(fscanf(f,"%7s",ending)!=1||strcmp(ending,"END")!=0||
       fscanf(f,"%1s",excess)==1)goto invalid;
    if(staged.held_object_id){
        LCObject *o=lc_find_object(&staged,staged.held_object_id);
        if(!o||!(o->flags&LC_OBJ_MOVABLE)||!seen[(size_t)(o-staged.objects)])goto invalid;
    }
    for(size_t i=0;i<staged.object_count;i++){
        const LCObject *o=&staged.objects[i];
        if(o->id!=staged.held_object_id&&(o->flags&LC_OBJ_MOVABLE)&&
           !lc_placement_clear(&staged,o->id,o->shape,false))goto invalid;
    }
    /* Validate player against restored dynamic colliders, not stale states. */
    if(!lc_body_valid(&staged))goto invalid;
    fclose(f);
    *world=staged;
    return true;
invalid:
    fclose(f);return false;
}
