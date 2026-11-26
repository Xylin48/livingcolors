#ifndef LC_PERCEPTION_H
#define LC_PERCEPTION_H
#include "interaction.h"

/* The same fixed first-person projection is used by the viewer and spatial
 * sampling. A sample is geometric evidence, not a rendered image or a claim
 * that the model has recognized a particular object. */
#define LC_VIEW_WIDTH 640u
#define LC_VIEW_HEIGHT 480u
#define LC_VIEW_MAGNIFICATION 1.25f
#define LC_VIEW_REFERENCE_FOV_DEG 60.0f

typedef struct {
    LCVec origin,forward,right,up;
    float vertical_fov_degrees,aspect;
} LCView;
bool lc_view(const LCPose *pose,LCView *out);
/* Normalized view coordinates: left/top 0, right/bottom 1. Center .5,.5.
 * Pixel centers can be mapped by (pixel+.5)/width or height. */
bool lc_view_ray(const LCPose *pose,float u,float v,LCVec *origin,LCVec *direction);
bool lc_view_sample(const LCWorld *world,float u,float v,float max_distance,LCHit *hit);
#endif
