#ifndef LC_WORLD_H
#define LC_WORLD_H
#include <stdbool.h>
#include <stddef.h>
typedef struct { float x,y,z; } LCVec;
typedef struct { LCVec min,max; } LCBox;
typedef struct { LCVec position; float yaw,pitch; } LCPose;
typedef struct { LCBox solids[128]; size_t count; } LCWorld;
bool lc_add_box(LCWorld *w, LCBox box);
bool lc_blocked(const LCWorld *w, LCVec feet);
bool lc_move(const LCWorld *w,LCPose *p,float dx,float dz,float dt);
void lc_house(LCWorld *w);
#endif
