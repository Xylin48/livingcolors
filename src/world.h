#ifndef LC_WORLD_H
#define LC_WORLD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LC_MAX_SOLIDS 256
#define LC_MAX_OBJECTS 96
#define LC_OBJECT_NAME 40
#define LC_WORLD_WIDTH 12.0f
#define LC_WORLD_DEPTH 10.0f
#define LC_UPSTAIRS_Y 3.0f
#define LC_PLAYER_RADIUS .24f
#define LC_PLAYER_HEIGHT 1.52f
#define LC_STAIR_TREADS 17u
#define LC_SOUND_EVENT_CAPACITY 8u

typedef struct { float x, y, z; } LCVec;
typedef struct { LCVec min, max; } LCBox;
typedef struct { LCVec position; float yaw, pitch; } LCPose;
typedef enum { LC_STANDING=0, LC_SEATED=1, LC_RESTING=2 } LCPosture;
typedef struct { uint8_t r, g, b; } LCColor;
typedef struct { LCBox shape; LCColor color; } LCSolid;

typedef enum {
    LC_MATERIAL_UNKNOWN=0, LC_MATERIAL_WOOD=1, LC_MATERIAL_FABRIC=2,
    LC_MATERIAL_METAL=3, LC_MATERIAL_PAPER=4, LC_MATERIAL_CERAMIC=5,
    LC_MATERIAL_PLASTIC=6
} LCMaterial;

typedef enum {
    LC_FURNITURE = 1, LC_DOOR = 2, LC_DEVICE = 3, LC_BOOK = 4,
    LC_CONTAINER = 5, LC_FOOD = 6
} LCObjectKind;

typedef enum {
    LC_OBJ_SOLID = 1u << 0,
    LC_OBJ_MOVABLE = 1u << 1,
    LC_OBJ_OPEN = 1u << 2,
    LC_OBJ_POWERED = 1u << 3,
    LC_OBJ_INTERACTIVE = 1u << 4
} LCObjectFlags;

typedef struct {
    uint32_t id;
    char name[LC_OBJECT_NAME];
    LCObjectKind kind;
    LCBox shape;
    LCColor color;
    uint32_t flags;
    uint32_t progress; /* e.g. page position; never inferred from rendering */
    uint32_t container_id,container_slot; /* zero when not stored; one-based slot */
    LCMaterial material; /* static exterior surface metadata, not a preference */
} LCObject;

typedef enum {
    LC_SOUND_APPLIANCE_HUM=1, LC_SOUND_CRT_HUM=2, LC_SOUND_DOORWAY_AIR=3,
    LC_SOUND_FOOTSTEP=4, LC_SOUND_DOOR_CLICK=5, LC_SOUND_SWITCH_CLICK=6,
    LC_SOUND_CONTAINER_CLICK=7
} LCSoundKind;
typedef struct {
    uint64_t started_ms;
    uint32_t source_id; /* zero for the body's own footsteps */
    LCSoundKind kind;
    LCVec position;
} LCTransientSound;

typedef struct {
    LCSolid solids[LC_MAX_SOLIDS];
    size_t count;
    LCObject objects[LC_MAX_OBJECTS];
    size_t object_count;
    LCPose player;
    uint32_t held_object_id; /* zero or one existing movable object; no duplicate inventory */
    LCPosture posture;
    uint32_t support_id;
    LCVec exit_position; /* previously occupied standing location */
    uint64_t environment_elapsed_ms; /* explicit simulation time, not wall clock */
    LCTransientSound sound_events[LC_SOUND_EVENT_CAPACITY]; /* ephemeral, not saved */
    size_t sound_event_count;
    float footstep_distance_m;
} LCWorld;

bool lc_add_wall(LCWorld *world, LCBox shape, LCColor color);
bool lc_add_box(LCWorld *world, LCBox shape); /* compatibility helper */
bool lc_add_object(LCWorld *world, LCObject object);
LCBox lc_object_shape(const LCObject *object);
/* Walking uses a continuous stair profile; rendering and rays share these tread boxes. */
bool lc_stair_tread(size_t index, LCBox *shape);
/* Validates a proposed box against architecture and other objects, without mutation. */
bool lc_placement_clear(const LCWorld *world, uint32_t id, LCBox shape, bool all_objects);
bool lc_blocked(const LCWorld *world, LCVec feet);
float lc_floor_height(float x, float z, float current_y);
bool lc_move(const LCWorld *world, LCPose *pose, float dx, float dz, float dt);
void lc_house(LCWorld *world);
LCObject *lc_find_object(LCWorld *world, uint32_t id);
bool lc_toggle_object(LCWorld *world, uint32_t id, LCVec from);
bool lc_move_object(LCWorld *world, uint32_t id, LCVec from, LCVec target);
bool lc_save(const LCWorld *world, const char *path);
bool lc_load(LCWorld *world, const char *path);
#endif
