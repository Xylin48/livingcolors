#ifndef LC_STORAGE_H
#define LC_STORAGE_H
#include "world.h"
#define LC_CONTAINER_PANELS 7u
/* Closed containers have a single visual box; open containers expose a back,
 * two sides, top, bottom and two shelves. Body collision retains outer bounds. */
size_t lc_container_panels(const LCObject *container,LCBox out[LC_CONTAINER_PANELS]);
unsigned lc_container_capacity(const LCObject *container);
bool lc_container_slot(const LCObject *container,unsigned slot,LCBox *bounds);
bool lc_storage_visible(const LCWorld *world,const LCObject *item);
bool lc_storage_accessible(const LCWorld *world,const LCObject *container);
bool lc_storage_valid(const LCWorld *world);
bool lc_store_held(LCWorld *world,uint32_t container_id);
#endif
