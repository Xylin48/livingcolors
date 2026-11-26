# Living Colors

Standalone procedural 3D world for R2-3PO. Develop and test independently before integration. Local operation must remain offline-capable.

## Design baseline
- Two-story house: open front entry/living room, right staircase, rear combined kitchen/dining, back door beside refrigerator. Upstairs bedroom across far end, hallway, left linen closet with hallway and bathroom doors, central bathroom with tub on stair-side wall and sink/toilet on bottom wall. Bedroom bed and wall-facing school desk along left wall, CRT and shelf along top wall, no PC.
- Physically connected street, grocery store and separate general/appliance store across from house.
- Actual first-person rendered 640x480 4:3 view, 125% projection magnification, no HUD, independent head orientation, separate from physical camera.
- Physical object properties, time-based movement and collision, limited perception, real containers, interruptible actions, functional furniture, authoritative world state and persistent object states across restarts.
- Preserve R2 systems and autonomous choices; no fabricated observations or forced routines.
- Generic electronic infrastructure only; actual phones and two-way calls/texts/voicemail belong to Store Items Overhaul. Food expansion belongs to Food Update.

## Engineering
Native C and raylib are proposed; exact dimensions, FOV, IPC protocol and performance budgets require validation. Procedural geometry and textures; independent manual testing; no integration until verified. Initial implementation is not yet complete.
