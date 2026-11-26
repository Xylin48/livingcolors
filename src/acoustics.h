#ifndef LC_ACOUSTICS_H
#define LC_ACOUSTICS_H
#include "world.h"

#define LC_AUDIO_MAX_SOURCES (5u+LC_SOUND_EVENT_CAPACITY)
#define LC_AUDIO_RATE 16000u
#define LC_AUDIO_CHANNELS 2u
#define LC_AUDIO_MAX_FRAMES 1600u

typedef struct {
    uint32_t source_id; /* simulation provenance, not recognized identity */
    LCSoundKind kind;
    float relative_bearing_rad; /* zero ahead, positive right */
    float distance_m;
    float gain; /* normalized amplitude, not decibels or measured loudness */
    bool obstructed;
    uint64_t start_frame; /* world-clock frame, only for transient sounds */
    uint32_t duration_frames; /* zero for continuous sounds */
} LCSoundCue;
typedef struct {
    uint64_t elapsed_ms;
    size_t count;
    LCSoundCue cues[LC_AUDIO_MAX_SOURCES];
} LCAudioSample;
/* Trusted world-owner hooks, invoked only after actual successful actions.
 * They never advance time or mutate the source's persisted device state. */
void lc_audio_note_walk(LCWorld *world,LCVec before);
void lc_audio_note_toggle(LCWorld *world,const LCObject *object);
void lc_audio_reset_transients(LCWorld *world);
bool lc_audio_frame_at(uint64_t elapsed_ms,uint64_t *frame);
/* A listener-local snapshot. No gaze constraint, but distance and obstruction
 * attenuate cues. Silence is a successful zero-count observation. */
bool lc_audio_sample(const LCWorld *world,LCAudioSample *out);
/* Render an original synthetic stereo PCM block from a frozen acoustic
 * snapshot. Caller owns 2*frames samples. Absolute frame index preserves
 * oscillator/noise continuity between blocks; no clock is advanced here.
 * Use lc_audio_frame_at(sample.elapsed_ms, ...) for a world-clock-aligned
 * block; transient envelopes use that clock. A new snapshot is needed
 * after changes in source/listener state. */
bool lc_audio_render(const LCAudioSample *sample,uint64_t first_frame,
                     size_t frames,int16_t *pcm,size_t pcm_samples);
#endif
