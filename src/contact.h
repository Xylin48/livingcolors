#ifndef LC_CONTACT_H
#define LC_CONTACT_H
#include "world.h"

typedef enum {
    LC_CONTACT_FOCUSED=1, LC_CONTACT_HELD=2, LC_CONTACT_SUPPORT=3
} LCContactMode;
typedef struct {
    LCMaterial material;
    bool known;
    float roughness,compliance,thermal_transfer; /* modeled relative [0,1] */
} LCMaterialProperties;
typedef struct {
    uint64_t elapsed_ms;
    LCContactMode mode;
    uint32_t object_id;
    LCMaterialProperties surface;
    float ambient_temperature_c; /* local air, NOT measured surface temperature */
    LCVec point;
    bool has_world_point;
} LCContactSample;
/* Explicit requests only. Focused contact is a short-range virtual surface
 * probe, not a hand animation, force sensor, or physical-world touch reading.
 * Held/support modes derive from existing world contact state, not gaze.
 * Optional expected ID (zero = any); failed queries leave output untouched. */
LCMaterialProperties lc_material_properties(LCMaterial material);
bool lc_contact_sample(const LCWorld *world,LCContactMode mode,uint32_t expected_id,LCContactSample *out);
#endif
