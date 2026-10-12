/// \file
/// Declares item-object creation, destruction and material selection contracts.

#ifndef ITEM_OBJECT_H
#define ITEM_OBJECT_H

#include <ultra64.h>

/// Remove an item from world participation and request its destruction.
/// \param item Live, registered item to destroy; must not already be pending destruction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80113B78 (0x80113B78).
void itemDestroy(u8 *item);

/// Start an item material animation from its beginning.
/// \param item Initialized item object receiving the animation.
/// \param animation Valid integer animation-track identifier.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801145D0 (0x801145D0).
void itemStartMaterialAnimation(u8 *item, s32 animation);

/// Select a fixed item material frame and stop material animation.
/// \param item Initialized item object receiving the material selection.
/// \param frame Valid material texture frame identifier representable as a signed halfword.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114614 (0x80114614).
void itemSetMaterialFrame(u8 *item, s32 frame);

/// Create and register an item at the requested world position and orientation.
/// \param x World X coordinate.
/// \param y World Y coordinate; a negative value requests ground placement using its magnitude as the terrain
/// query height.
/// \param z World Z coordinate.
/// \param pitch Initial rotation about the X axis in degrees.
/// \param yaw Initial rotation about the Y axis in degrees.
/// \param kind Item kind identifier from zero through 165 with a valid model and animation configuration.
/// \returns Registered item managed by the object system.
///
/// \note Requires initialized registry, object resources, and area configuration; ground placement also
/// requires initialized terrain. The caller must enforce any item-count limit. Portability: registry keys use
/// 32-bit integer object addresses.
/// \note Original decompilation identifier: \c func_80113944 (0x80113944).
u8 *itemCreate(f32 x, f32 y, f32 z, f32 pitch, f32 yaw, s32 kind);

#endif
