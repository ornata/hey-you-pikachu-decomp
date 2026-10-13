/// \file
/// Declares destination-door creation, passage access and shared description text.
#ifndef DOOR_H
#define DOOR_H
#include <ultra64.h>
#include "passage_types.h"

/// Begin a new door-registry lifetime for area initialization.
/// \returns Nothing; existing objects remain owned by the object system.
/// \note Original decompilation identifier: \c func_8001C110 (0x8001C110).
void doorRegistryReset(void);

/// Create and register a door or destination sign.
/// \param position Three-component world position.
/// \param kind Description category from zero through three.
/// \param destination Area or exit category selected by kind.
/// \param entrance Entrance identifier in the destination.
/// \param accessState Passage state: zero allowed, one locked, two unavailable, three requiring Pikachu, four blocked by Squirtle.
/// \returns Registered object owned by the object system; the ten-entry door registry must have room.
u8 *doorCreate(f32 *position, s32 kind, s32 destination, s32 entrance, s32 accessState);

/// Change a registered door's passage state.
/// \param doorIndex Valid created-door registry index.
/// \param accessState Passage state from zero through four, using doorCreate's categories.
/// \returns Nothing.
void doorSetAccessState(s32 doorIndex, s32 accessState);

/// Format a destination description.
/// \param object Initialized door with a supported description category and destination.
/// \returns Borrowed shared text, overwritten by the next destination-description request.
s8 *doorGetDescription(u8 *object);

/// Format a passage-access description.
/// \param object Initialized door with passage state.
/// \returns Borrowed shared text, overwritten by the next access-description request.
u8 *doorGetAccessDescription(u8 *object);
#endif
