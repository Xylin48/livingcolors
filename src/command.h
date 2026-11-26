#ifndef LC_COMMAND_H
#define LC_COMMAND_H

#include "interaction.h"
#include "environment.h"
#include "contact.h"
#include "acoustics.h"

#define LC_COMMAND_API_VERSION 5u
#define LC_COMMAND_MIN_VERSION 1u

/* A renderer-independent, synchronous world command. Correlation IDs are
 * caller-owned labels, NOT event IDs or replay/deduplication guarantees.
 * One world owner serializes calls. No external network or model required.
 * Recompiled clients may request earlier semantic versions (1–5); this is
 * not a stable binary ABI. Receipts always use the current struct version. */
typedef enum {
    LC_COMMAND_WALK=1, LC_COMMAND_LOOK=2, LC_COMMAND_USE=3, LC_COMMAND_SENSE=4
} LCCommandKind;
typedef enum {
    LC_COMMAND_INVALID=0, LC_COMMAND_REJECTED=1,
    LC_COMMAND_UNCHANGED=2, LC_COMMAND_APPLIED=3, LC_COMMAND_OBSERVED=4
} LCCommandStatus;

typedef enum {
    LC_SENSE_ENVIRONMENT=1, LC_SENSE_FOCUSED_CONTACT=2,
    LC_SENSE_HELD_CONTACT=3, LC_SENSE_SUPPORT_CONTACT=4, LC_SENSE_AUDIO=5
} LCSenseKind;

typedef struct {
    uint32_t api_version;
    uint64_t correlation_id;
    LCCommandKind kind;
    union {
        struct { float forward, right, seconds; } walk;
        struct { float yaw_delta, pitch_delta; } look;
        struct { uint32_t object_id; LCAction action; } use;
        struct { LCSenseKind kind; uint32_t expected_object_id; } sense;
    } input;
} LCCommand;

typedef enum {
    LC_LOCATION_NONE=0, LC_LOCATION_WORLD=1, LC_LOCATION_HELD=2, LC_LOCATION_CONTAINER=3
} LCLocation;

typedef struct {
    uint32_t id;
    LCObjectKind kind;
    LCLocation location;
    uint32_t flags,progress,container_id,container_slot;
    LCBox bounds; /* meaningful in world or container; zero for held */
} LCObjectState;

typedef struct {
    uint32_t api_version;
    uint64_t correlation_id;
    LCCommandKind kind;
    LCCommandStatus status;
    LCAction requested_action; /* input, not evidence of success */
    uint32_t requested_object_id; /* input, not observed target metadata */
    LCPose before_pose,after_pose;
    uint32_t before_held_id,after_held_id;
    LCPosture before_posture,after_posture;
    uint32_t before_support_id,after_support_id;
    LCObjectState before_object,after_object; /* affected item; store request targets its container */
    LCSenseKind requested_sense;
    uint64_t environment_elapsed_ms;
    bool has_environment_sample,has_contact_sample;
    LCEnvironmentSample environment_sample;
    LCContactSample contact_sample;
    bool has_audio_sample;
    LCAudioSample audio_sample;
} LCCommandReceipt;

/* Receipts report observed in-memory state transitions, not durable Life Log
 * events, emotional appraisals, rewards, or proof of learning. A rejected
 * command exposes no target-object data. An output receipt is mandatory.
 * All values are native in-process types, NOT an IPC wire format. */
LCCommandStatus lc_execute(LCWorld *world,const LCCommand *command,LCCommandReceipt *receipt);

#endif
