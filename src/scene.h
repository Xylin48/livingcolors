#ifndef LC_SCENE_H
#define LC_SCENE_H
#include "world.h"
typedef struct {
    LCBox bounds;
    LCColor color;
    uint32_t object_id; /* zero for architecture */
} LCScenePart;
typedef void (*LCSceneVisitor)(const LCScenePart *part,void *context);
/* Read-only geometry shared by rays and both rendering backends. The visitor
 * receives a borrowed part valid only during the call; do not mutate world. */
bool lc_scene_visit(const LCWorld *world,LCSceneVisitor visit,void *context);
LCColor lc_scene_lit_color(const LCWorld *world,const LCScenePart *part);
LCColor lc_scene_sky(const LCWorld *world);
#endif
