#ifndef LC_COMMAND_H
#define LC_COMMAND_H

#include "interaction.h"

#define LC_COMMAND_API_VERSION 1u

/* A renderer-independent, synchronous world command. Correlation IDs are
 * caller-owned labels, NOT event IDs or replay/deduplication guarantees.
 * One world owner serializes calls. No external network or model required. */
typedef enum {
    LC_COMMAND_WALK=1, LC_COMMAND_LOOK=2, LC_COMMAND_USE=3
} LCCommandKind;
typedef enum {
    LC_COMMAND_INVALID=0, LC_COMMAND_REJECTED=1,
    LC_COMMAND_UNCHANGED=2, LC_COMMAND_APPLIED=3
} LCCommandStatus;

typedef struct {
    uint32_t api_version;
    uint64_t correlation_id;
    LCCommandKind kind;
    union {
        struct { float forward, right, seconds; } walk;
        struct { float yaw_delta, pitch_delta; } look;
        struct { uint32_t object_id; LCAction action; } use;
    } input;
} LCCommand;

typedef enum {
    LC_LOCATION_NONE=0, LC_LOCATION_WORLD=1, LC_LOCATION_HELD=2
} LCLocation;

typedef struct {
    uint32_t id;
    LCObjectKind kind;
    LCLocation location;
    uint32_t flags,progress;
    LCBox bounds; /* meaningful only for LC_LOCATION_WORLD */
} LCObjectState;

typedef struct {
    uint32_t api_version;
    uint64_t correlation_id;
    LCCommandKind kind;
    LCCommandStatus status;
    LCPose before_pose,after_pose;
    uint32_t before_held_id,after_held_id;
    LCObjectState before_object,after_object; /* only on applied object actions */
} LCCommandReceipt;

/* Receipts report observed in-memory state transitions, not durable Life Log
 * events, emotional appraisals, rewards, or proof of learning. A rejected
 * command exposes no target-object data. An output receipt is mandatory.
 * All values are native in-process types, NOT an IPC wire format. */
LCCommandStatus lc_execute(LCWorld *world,const LCCommand *command,LCCommandReceipt *receipt);

#endif
