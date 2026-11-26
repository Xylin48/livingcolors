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

Use **WASD** to walk, **mouse** to turn/look, **E** to toggle a nearby interactive device or door, **P** to advance the school reader's stored progress (development shortcut), **F5** to save, **F9** to load, **ESC** to quit. State is saved automatically to `livingcolors.save` on normal exit. Terminal output reports interactions; the view has no HUD.

## Important limitations

This is a foundation, **not a finished Living Colors release**. Geometry dimensions and exact staircase landing alignments still need refinement. The renderer uses procedural boxes and flat colors rather than complete late-1990s textures. Outdoor streets and stores, hands and detailed manipulation, camera-to-R2 sensory integration, physics for falling items, time systems, object inventories, real PDF page reading, media playback, actual device operation and Android phone features are not yet integrated. Book progress is a persisted integer; it is **not** PDF parsing. The open front/back doors do not yet lead outside the world boundary. Device toggling is proximity-based, not a complete vision/line-of-sight interaction model.

## Architecture

`src/world.h` defines the world contract; `src/world.c` owns house layout, collisions, objects and movement; `src/save.c` owns snapshot serialization; `src/main.c` is the optional standalone viewer. The renderer reads world state, rather than replacing it with a generated description. Core state persists independently from rendering.

Prioritized plan: complete and test the two-story house, navigable environment and world persistence; extend physical manipulation and independent perception; then physically connect the street, grocery store and general/appliance store; introduce generic electronics infrastructure before integrating with R2. Other update foundations can be developed opportunistically without deprioritizing Living Colors.
