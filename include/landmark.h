/// \file
/// Declares creation and descriptive text for selectable scene landmarks.

#ifndef LANDMARK_H
#define LANDMARK_H

#include <ultra64.h>

/// Create and register a landmark hierarchy at a world position.
/// \param x World-space horizontal coordinate.
/// \param y World-space height.
/// \param z World-space depth coordinate.
/// \param landmarkId Object kind identifying the landmark.
/// \returns Newly created hierarchy root, owned by the object manager.
/// \note Original decompilation identifier: \c func_8001F110 (0x8001F110).
u8 *landmarkCreate(f32 x, f32 y, f32 z, s32 landmarkId);

/// Retrieve the pirate cannon landmark description.
/// \param unused Unused callback argument.
/// \returns Borrowed text in shared storage, overwritten by a later call.
/// \note Original decompilation identifier: \c func_8001F194 (0x8001F194).
s8 *landmarkGetCannonDescription(s32 unused);

#endif
