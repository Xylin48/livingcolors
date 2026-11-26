#ifndef LC_SESSION_H
#define LC_SESSION_H
#include "command.h"
#include "frame.h"

typedef struct LCSession LCSession;
typedef struct {
    uint64_t session_id; /* host-supplied identity, unique within the host */
    uint64_t generation; /* increments after a successful load */
    uint64_t revision;   /* increments after applied commands, nonzero ticks or loads */
    uint64_t elapsed_ms;
} LCSessionStamp;
typedef struct {
    LCSessionStamp before,after;
    LCCommandReceipt command;
} LCSessionReceipt;

enum {
    LC_CAPTURE_RGB=1u,LC_CAPTURE_AUDIO=2u,LC_CAPTURE_ENVIRONMENT=4u
};
typedef struct {
    uint32_t flags;
    uint8_t *rgb;
    size_t rgb_bytes;
    float *depth;
    size_t depth_count;
    int16_t *pcm;
    size_t pcm_samples,audio_frames;
} LCCaptureRequest;
typedef struct {
    LCSessionStamp stamp;
    uint32_t flags;
    unsigned width,height,audio_rate,audio_channels;
    size_t audio_frames;
    LCEnvironmentSample environment;
    LCAudioSample acoustic;
} LCCaptureResult;
/* One serialized owner. No internal thread, wall clock, network, model or
 * window. Session IDs/revisions are NOT durable event IDs or replay tokens.
 * The owner supplies distinct nonzero IDs for simultaneously active sessions. */
LCSession *lc_session_create(uint64_t session_id);
void lc_session_destroy(LCSession *session);
bool lc_session_stamp(const LCSession *session,LCSessionStamp *out);
bool lc_session_execute(LCSession *session,const LCCommand *command,LCSessionReceipt *out);
bool lc_session_advance(LCSession *session,uint32_t ms);
bool lc_session_save(const LCSession *session,const char *path);
bool lc_session_load(LCSession *session,const char *path);
/* Capture all requested modalities from the same revision and world time.
 * Caller owns disjoint buffers. Audio is a prospective frozen-state block,
 * not a recording of past time. A successful capture never changes state.
 * Result is unchanged on failure; buffers are only valid on success. */
bool lc_session_capture(const LCSession *session,const LCCaptureRequest *request,LCCaptureResult *out);
#endif
