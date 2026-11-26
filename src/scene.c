#include "scene.h"
#include "storage.h"
#include "environment.h"

bool lc_scene_visit(const LCWorld *w,LCSceneVisitor visit,void *ctx){
    if(!w||!visit||w->count>LC_MAX_SOLIDS||w->object_count>LC_MAX_OBJECTS)return false;
    for(size_t i=0;i<w->count;i++){
        LCScenePart p={w->solids[i].shape,w->solids[i].color,0};
        visit(&p,ctx);
    }
    for(size_t i=0;i<LC_STAIR_TREADS;i++){
        LCScenePart p={.color={119,78,46}};
        if(lc_stair_tread(i,&p.bounds))visit(&p,ctx);
    }
    for(size_t i=0;i<w->object_count;i++){
        const LCObject *o=&w->objects[i];
        if(!lc_storage_visible(w,o))continue;
        LCColor c=o->color;
        if(o->kind==LC_DEVICE&&(o->flags&LC_OBJ_POWERED))c=(LCColor){73,132,126};
        LCBox panels[LC_CONTAINER_PANELS];
        size_t n=lc_container_panels(o,panels);
        if(!n){panels[0]=lc_object_shape(o);n=1;}
        for(size_t j=0;j<n;j++){
            LCScenePart p={panels[j],c,o->id};visit(&p,ctx);
        }
    }
    return true;
}
LCColor lc_scene_lit_color(const LCWorld *w,const LCScenePart *p){
    if(!w||!p)return (LCColor){0};
    LCBox b=p->bounds;
    LCVec center={(b.min.x+b.max.x)*.5f,(b.min.y+b.max.y)*.5f,(b.min.z+b.max.z)*.5f};
    float light=lc_environment_light_at(w,center);
    return (LCColor){(uint8_t)(p->color.r*light),(uint8_t)(p->color.g*light),(uint8_t)(p->color.b*light)};
}
LCColor lc_scene_sky(const LCWorld *w){
    float sky=.10f+.90f*lc_environment_daylight(w);
    return (LCColor){(uint8_t)(137*sky),(uint8_t)(164*sky),(uint8_t)(176*sky)};
}
