#ifndef LC_ENVIRONMENT_H
#define LC_ENVIRONMENT_H
#include "world.h"

#define LC_DAY_MS UINT64_C(86400000)
#define LC_START_MS UINT64_C(28800000) /* standalone scenario starts at 08:00 */
typedef enum {
    LC_ZONE_UNKNOWN=0, LC_ZONE_LIVING=1, LC_ZONE_KITCHEN=2,
    LC_ZONE_BEDROOM=3, LC_ZONE_HALL=4, LC_ZONE_BATH=5, LC_ZONE_CLOSET=6,
    LC_ZONE_STAIRS=7
} LCZone;
typedef struct {
    uint64_t elapsed_ms;
    uint32_t time_of_day_ms;
    LCZone zone;
    float temperature_c;
    float air_speed_mps;
    float light_level; /* relative [0,1], not calibrated lux */
    bool artificial_light;
} LCEnvironmentSample;
/* Host owns time advancement. No time passes as a side effect of perception,
 * rendering, command execution or loading. No wall-clock inference. */
bool lc_environment_advance(LCWorld *world,uint32_t elapsed_ms);
uint32_t lc_environment_time_of_day(const LCWorld *world);
LCZone lc_environment_zone(LCVec point);
uint32_t lc_environment_switch(LCZone zone);
float lc_environment_daylight(const LCWorld *world);
float lc_environment_light_at(const LCWorld *world,LCVec point);
bool lc_environment_sample(const LCWorld *world,LCEnvironmentSample *sample);
#endif
