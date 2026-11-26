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

typedef struct { float x, y, z; } LCVec;
typedef struct { LCVec min, max; } LCBox;
typedef struct { LCVec position; float yaw, pitch; } LCPose;
typedef struct { uint8_t r, g, b; } LCColor;
typedef struct { LCBox shape; LCColor color; } LCSolid;

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
} LCObject;

typedef struct {
    LCSolid solids[LC_MAX_SOLIDS];
    size_t count;
    LCObject objects[LC_MAX_OBJECTS];
    size_t object_count;
    LCPose player;
} LCWorld;

bool lc_add_wall(LCWorld *world, LCBox shape, LCColor color);
bool lc_add_box(LCWorld *world, LCBox shape); /* compatibility helper */
bool lc_add_object(LCWorld *world, LCObject object);
LCBox lc_object_shape(const LCObject *object);
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
