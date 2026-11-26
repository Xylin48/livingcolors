#ifndef LC_FRAME_H
#define LC_FRAME_H
#include "perception.h"
#define LC_FRAME_PIXELS (LC_VIEW_WIDTH*LC_VIEW_HEIGHT)
#define LC_FRAME_RGB_BYTES (LC_FRAME_PIXELS*3u)
/* Headless, native RGB24 capture. Row-major top-left origin, no HUD. Depth
 * workspace is caller-owned and contains reciprocal forward depth (zero
 * for sky). Buffers must be disjoint. No allocation, display or clock tick. */
bool lc_frame_render(const LCWorld *world,uint8_t *rgb,size_t rgb_bytes,
                     float *depth,size_t depth_count);
#endif
