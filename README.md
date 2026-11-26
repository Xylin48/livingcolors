# Living Colors

An independent, native 3D world foundation for R2-3PO. This repository is developed separately from R2 until tested and ready for integration.

## Current implementation

- Two-story *provisional* house layout with separate upstairs rooms and continuous staircase height model.
- 640 × 480 native 4:3 desktop viewer (requires raylib) with first-person navigation, independent mouse look and a fixed **125% reference magnification**. No in-world HUD, fake screenshots, scripted tour, or imported game assets.
- Static walls, furnishings and IDs for the bed, school desk, two CRTs, bookshelf, fridge, doors, bathtub, sink, toilet and a movable school reader.
- Capsule-approximate player collision (horizontal radius + vertical interval), incremental movement, axis sliding and limited stair traversal.
- Physical object interactions (door/container open state, TV power), limited-range pickup/reposition API, reader progress and durable snapshot state keyed to stable object IDs. Save writes to a temporary file and renames on successful completion.
- Native core tests and CI, usable without a display server or raylib.

## Build

On Linux, `make test` requires only a C11 compiler and the C math library.

To run the viewer, install the raylib development package with pkg-config metadata and run:

```sh
make viewer
./build/livingcolors
```

Use **WASD** to walk, **mouse** to turn/look, **E** to toggle the focused, reachable interactive device or door, **G** to pick up the focused movable item or place the held item on a reachable support surface, **P** to advance the focused, reachable school reader's stored progress (development shortcut), **F5** to save, **F9** to load, **ESC** to quit. State is saved automatically to `livingcolors.save` on normal exit. Terminal output reports interactions; the view has no HUD.

## Important limitations

This is a foundation, **not a finished Living Colors release**. Geometry dimensions and exact staircase landing alignments still need refinement. The renderer uses procedural boxes and flat colors rather than complete late-1990s textures. Outdoor streets and stores, hands and detailed manipulation, camera-to-R2 sensory integration, physics for falling items, time systems, object inventories, real PDF page reading, media playback, actual device operation and Android phone features are not yet integrated. Book progress is a persisted integer; it is **not** PDF parsing. The open front/back doors do not yet lead outside the world boundary. Viewer toggling checks line of sight and reach. Legacy low-level functions are retained, but subsystem clients should use the interaction API for gaze-checked actions. Stair treads are shared visual/ray geometry above a continuous movement surface. The held item is tracked by ID and omitted from its former scene position; a hand mesh and container contents remain future work.

## Architecture

`src/world.h` defines the world contract; `src/world.c` owns house layout, collisions, objects and movement; `src/save.c` owns snapshot serialization; `src/main.c` is the optional standalone viewer. The renderer reads world state, rather than replacing it with a generated description. Core state persists independently from rendering.

Prioritized plan: complete and test the two-story house, navigable environment and world persistence; extend physical manipulation and independent perception; then physically connect the street, grocery store and general/appliance store; introduce generic electronics infrastructure before integrating with R2. Other update foundations can be developed opportunistically without deprioritizing Living Colors.

## Subsystem-facing interaction API
`src/interaction.h` v2 provides renderer-independent gaze, ray intersection and reach-checked interaction against the current world. The manual viewer uses the same commands; it is not a dependency for R2. No R2 repository changes or IPC transport yet. Legacy movement, save format, stable object IDs and low-level functions remain available. Open door panels and floor slabs now have shared physical geometry. Reading advances progress only when the reader is focused and in reach; actual document page rendering remains future work. Integration must preserve physical sensory paths, and Memory should consume confirmed events rather than treating all geometry as observed. House dimensions remain provisional.

## Physical manipulation and persistence
The G command uses a single held-object slot in the authoritative world. Picking up removes the same object from scene rays and collision, not from the object table. Placement requires a visible, reachable top surface with a complete supporting footprint and no interpenetration. Failed placements leave the held object unchanged. The legacy low-level reposition API remains available for existing callers, but new clients should use the gaze-checked API. Snapshot format 2 adds the held ID; the new loader accepts format 1 and retains defaults for new object IDs, but old binaries cannot read format 2. Unknown/duplicate IDs, resized movable objects, colliding restored items and invalid held IDs are rejected. No new ownership or inventory database is introduced. Stair treads participate in sight and rendering while movement uses a continuous incline; placing items on the stairs is not implemented.
