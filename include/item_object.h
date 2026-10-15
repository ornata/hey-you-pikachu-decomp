/// \file
/// Declares item-object creation, destruction and material selection contracts.

#ifndef ITEM_OBJECT_H
#define ITEM_OBJECT_H

#include <ultra64.h>

/// Remove an item from world participation and request its destruction.
/// \param item Live, registered item to destroy; must not already be pending destruction.
/// \note Object and state storage remain allocated until the deferred hierarchy destruction sweep;
/// the item is removed from registry participation by this call.
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
/// \param kind Retained item kind from one through 164 with valid model and animation resources;
/// catalog endpoints zero and 165 have no model.
/// \returns Registered item managed by the object system.
///
/// \note Requires initialized registry, object resources, and area configuration; ground placement also
/// requires initialized terrain. Sufficient allocation and registry capacity must be available; the caller
/// enforces any item-count limit. Borrowed model and animation resources must remain live while used by the
/// item. Portability: registry keys use 32-bit integer object addresses.
/// \note Original decompilation identifier: \c func_80113944 (0x80113944).
u8 *itemCreate(f32 x, f32 y, f32 z, f32 pitch, f32 yaw, s32 kind);

/// Select the rendering style of an item; a missing item is ignored.
/// \param objectId Item object handle.
/// \param mode Supported item draw mode from zero through six.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114624 (0x80114624).
void itemSetDrawMode(s32 objectId, s32 mode);

/// Set the color tint of a live item; has no effect when the item is unavailable.
/// \param id Item object handle.
/// \param r Red intensity from zero through 255.
/// \param g Green intensity from zero through 255.
/// \param b Blue intensity from zero through 255.
/// \param a Opacity from zero through 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114654 (0x80114654).
void itemSetTint(s32 id, u8 r, u8 g, u8 b, u8 a);

/// Apply a velocity change to a free item that permits motion.
/// \param objectId Handle of a live item.
/// \param velocity Three world-space velocity components; must not overlap the item state.
/// \returns Nothing.
void itemAddVelocity(s32 objectId, f32 *velocity);

/// Start visual direction motion when the item permits it.
/// \param item Initialized item object.
/// \param duration Positive finite duration in seconds.
/// \param pitchRate Pitch angular rate in degrees per second.
/// \param yawRate Yaw angular rate in degrees per second.
/// \returns Nothing.
void itemStartDirectionMotion(u8 *item, f32 duration, f32 pitchRate, f32 yawRate);

#endif
