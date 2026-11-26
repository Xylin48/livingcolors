#include "contact.h"
#include "environment.h"
#include "interaction.h"
#include "body.h"
#include "storage.h"
#include <math.h>

LCMaterialProperties lc_material_properties(LCMaterial m){
    /* Illustrative normalized coefficients, deliberately not SI measurements
     * or calibrated psychophysics. No pleasantness/valence field. */
    switch(m){
    case LC_MATERIAL_WOOD:return (LCMaterialProperties){m,true,.32f,.08f,.20f};
    case LC_MATERIAL_FABRIC:return (LCMaterialProperties){m,true,.55f,.78f,.08f};
    case LC_MATERIAL_METAL:return (LCMaterialProperties){m,true,.10f,.02f,.90f};
    case LC_MATERIAL_PAPER:return (LCMaterialProperties){m,true,.40f,.38f,.12f};
    case LC_MATERIAL_CERAMIC:return (LCMaterialProperties){m,true,.08f,.01f,.60f};
    case LC_MATERIAL_PLASTIC:return (LCMaterialProperties){m,true,.16f,.12f,.16f};
    default:return (LCMaterialProperties){LC_MATERIAL_UNKNOWN,false,0,0,0};
    }
}
static const LCObject *find(const LCWorld *w,uint32_t id){
    if(!w||!id||w->object_count>LC_MAX_OBJECTS)return NULL;
    for(size_t i=0;i<w->object_count;i++)if(w->objects[i].id==id)return &w->objects[i];
    return NULL;
}
bool lc_contact_sample(const LCWorld *w,LCContactMode mode,uint32_t expected,LCContactSample *out){
    LCEnvironmentSample env;
    if(!out||!lc_environment_sample(w,&env))return false;
    LCContactSample s={.elapsed_ms=env.elapsed_ms,.mode=mode,.ambient_temperature_c=env.temperature_c};
    const LCObject *o=NULL;
    switch(mode){
    case LC_CONTACT_FOCUSED:{
        LCHit hit;
        if(!lc_focus(w,1.25f,&hit)||!hit.object_id)return false;
        o=find(w,hit.object_id);
        if(o&&o->container_id&&!lc_storage_accessible(w,find(w,o->container_id)))return false;
        s.point=hit.point;s.has_world_point=true;
        break;
    }
    case LC_CONTACT_HELD:
        o=find(w,w->held_object_id);
        if(o&&(!(o->flags&LC_OBJ_MOVABLE)||o->container_id))return false;
        break;
    case LC_CONTACT_SUPPORT:
        if(w->posture==LC_STANDING)return false;
        o=find(w,w->support_id);
        s.point=w->player.position;s.has_world_point=true;
        break;
    default:return false;
    }
    if(!o||(expected&&o->id!=expected))return false;
    s.object_id=o->id;s.surface=lc_material_properties(o->material);
    *out=s;
    return true;
}
