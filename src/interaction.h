#ifndef LC_INTERACTION_H
#define LC_INTERACTION_H
#include "world.h"
#define LC_INTERACTION_API_VERSION 1u
#define LC_EYE_HEIGHT 1.35f
/* Read-only geometric observation. A zero ID is architecture, not an object.
 * The caller owns the output; no global state or renderer dependency. */
typedef struct { uint32_t object_id; float distance; LCVec point; } LCHit;
typedef enum { LC_ACTION_TOGGLE=1, LC_ACTION_READ_NEXT=2 } LCAction;
LCVec lc_eye(const LCPose *pose);
LCVec lc_view_direction(const LCPose *pose);
bool lc_cast(const LCWorld *world, LCVec origin, LCVec direction, float max_distance, LCHit *hit);
bool lc_focus(const LCWorld *world, float max_distance, LCHit *hit);
bool lc_look(LCWorld *world,float yaw_delta,float pitch_delta);
/* Requires current gaze and reach; never accepts a caller-supplied actor position. */
bool lc_use(LCWorld *world,uint32_t id,LCAction action);
#endif
