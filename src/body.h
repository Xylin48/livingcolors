#ifndef LC_BODY_H
#define LC_BODY_H
#include "world.h"

/* Standing position is feet; seated/resting position is contact with the
 * furniture top. The same authoritative pose drives perception and collision.
 * This is physical posture, not a claim about sleep or internal needs. */
bool lc_body_valid(const LCWorld *world);
bool lc_body_intersects(const LCWorld *world,LCBox shape);
bool lc_body_enter(LCWorld *world,uint32_t id,LCPosture posture);
bool lc_body_stand(LCWorld *world);
LCPose lc_body_camera_pose(const LCWorld *world);
LCPosture lc_furniture_posture(const LCObject *object);
#endif
