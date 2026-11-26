#ifndef LC_ACOUSTICS_H
#define LC_ACOUSTICS_H
#include "world.h"

#define LC_AUDIO_MAX_SOURCES 5u
#define LC_AUDIO_RATE 16000u
#define LC_AUDIO_CHANNELS 2u
#define LC_AUDIO_MAX_FRAMES 1600u

typedef enum {
    LC_SOUND_APPLIANCE_HUM=1, LC_SOUND_CRT_HUM=2, LC_SOUND_DOORWAY_AIR=3
} LCSoundKind;
typedef struct {
    uint32_t source_id; /* simulation provenance, not recognized identity */
    LCSoundKind kind;
    float relative_bearing_rad; /* zero ahead, positive right */
    float distance_m;
    float gain; /* normalized amplitude, not decibels or measured loudness */
    bool obstructed;
} LCSoundCue;
typedef struct {
    uint64_t elapsed_ms;
    size_t count;
    LCSoundCue cues[LC_AUDIO_MAX_SOURCES];
} LCAudioSample;
/* A listener-local snapshot. No gaze constraint, but distance and obstruction
 * attenuate cues. Silence is a successful zero-count observation. */
bool lc_audio_sample(const LCWorld *world,LCAudioSample *out);
/* Render an original synthetic stereo PCM block from a frozen acoustic
 * snapshot. Caller owns 2*frames samples. Absolute frame index preserves
 * oscillator/noise continuity between blocks; no clock is advanced here.
 * A new snapshot is needed after changes in source/listener state. */
bool lc_audio_render(const LCAudioSample *sample,uint64_t first_frame,
                     size_t frames,int16_t *pcm,size_t pcm_samples);
#endif
