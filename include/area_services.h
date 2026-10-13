/// \file
/// Declares current-area loading, spatial queries, terrain effects and item spawning.

#ifndef AREA_SERVICES_H
#define AREA_SERVICES_H

#include <ultra64.h>
#include "item_spawn.h"

/// Look up a sound effect for a terrain material and contact category.
/// \param category Sound category identifier from zero through seven.
/// \param material Terrain material code with a material identifier from zero through seven.
/// \returns The configured sound effect identifier, or minus one when no sound is configured.
///
/// \note Undefined behavior: material identifiers eight through fifteen exceed the selected row's bounds.
/// \note Original decompilation identifier: \c func_8010C1A4 (0x8010C1A4).
s32 terrainGetMaterialSound(s32 category, s32 material);

/// Create the contact effect appropriate to a terrain material when one is configured.
/// \param position World position in X, Y, Z order; supported ground effects may adjust Y to just above the
/// terrain.
/// \param material Terrain material code selecting the contact effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010C1C4 (0x8010C1C4).
void terrainCreateContactEffect(f32 *position, s32 material);

/// Check whether a world position lies outside the current area's permitted bounds.
/// \param x World X coordinate.
/// \param y World Y coordinate.
/// \param z World Z coordinate.
/// \returns One if the position is outside the permitted bounds, or zero otherwise.
///
/// \note Requires an initialized current area.
/// \note Original decompilation identifier: \c func_8010AAF0 (0x8010AAF0).
s32 areaIsPositionOutsideBounds(f32 x, f32 y, f32 z);

/// Look up the display name of an area.
/// \param area Area identifier from zero through 22.
/// \returns The persistent area display-name string.
/// \note Original decompilation identifier: \c func_8010E958 (0x8010E958).
char *areaGetDisplayName(s32 area);

/// Check horizontal clearance at a candidate world position under the requested collision policy.
/// \param type Collision policy from zero through nine; policy five also requires the active primary actor.
/// \param x Candidate world X coordinate.
/// \param y Candidate world Y coordinate.
/// \param z Candidate world Z coordinate.
/// \param width Horizontal clearance width.
/// \param height Vertical span used for clearance checks.
/// \param step Maximum permitted terrain world height.
/// \param dirX X movement sign: minus one, zero, or one.
/// \param dirZ Z movement sign: minus one, zero, or one.
/// \param previous Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Directional obstruction flags, or zero when the tested position is clear.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010C788 (0x8010C788).
u16 terrainCheckHorizontalObstruction(s32 type, f32 x, f32 y, f32 z, f32 width, f32 height, f32 step, f32 dirX,
                                      f32 dirZ, u8 **previous);

/// Load and initialize the selected destination area's overlay.
/// \returns The start of available memory following the loaded overlay, aligned to sixteen bytes.
///
/// \note Requires a selected destination from zero through 22 and available destination memory. Loading
/// replaces the previous scene overlay; its references must no longer be in use.
/// \note Original decompilation identifier: \c func_8010E650 (0x8010E650).
void *areaLoadNextOverlay(void);

/// Dispatch a position-based area event, suppressing repeat events within the same node for one second.
/// \param position Readable world event position in X, Y, Z order; borrowed by the callback during this call.
/// \param eventType Event selector passed to the area's event handler.
/// \returns The event handler's result, zero when a repeat event is suppressed, or minus one when no handler
/// is present.
///
/// \note Requires an initialized current area. The callback receives the position, selected node, and event
/// selector.
/// \note Original decompilation identifier: \c func_8010B054 (0x8010B054).
s32 areaDispatchPositionEvent(f32 *position, s32 eventType);

/// Look up the friction coefficient for a terrain material.
/// \param material Terrain material code whose low four bits identify a material from zero through seven.
/// \returns The configured friction coefficient.
///
/// \note Undefined behavior: identifiers eight through fifteen exceed the friction table bounds.
/// \note Original decompilation identifier: \c func_8010C18C (0x8010C18C).
f32 terrainGetFriction(s32 material);

/// Remove a maze block's area association and request destruction of its object hierarchy.
/// \param object Live block object associated with its current maze region.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010DC14 (0x8010DC14).
void mazeRemoveBlockObject(u8 *object);

/// Spawn listed maze items whose saved availability permits them to appear.
/// \param spawn Readable placement list terminated by item kind zero; preceding kinds must be valid catalog
/// entries.
/// \returns Nothing.
///
/// \note Requires initialized item and progress systems. Portability: the empty debug type check preserves
/// IDO register allocation; its historical comparison constant is unrecoverable.
/// \note Original decompilation identifier: \c func_8010E6C4 (0x8010E6C4).
void mazeSpawnAvailableItems(ItemSpawn *spawn);

#endif
