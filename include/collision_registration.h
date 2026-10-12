/// \file
/// Declares runtime-object collision-candidate registration, removal and overlap queries.

#ifndef COLLISION_REGISTRATION_H
#define COLLISION_REGISTRATION_H

#include "ultra64.h"

/// Register an eligible object at its current collision-grid position.
/// \param object Live object with initialized radius, flags and world position.
/// \returns Nonzero registration key, or zero when ineligible or registration fails.
/// \note The caller retains the key and removes the registration before replacing it or destroying the object.
/// \note Original decompilation identifier: \c func_8002CBA8 (0x8002CBA8).
s32 collisionGridRegisterObject(u8 *object);

/// Remove an object's collision registration and clear the stored key.
/// \param object Object associated with the stored registration key.
/// \param registrationKey Writable stored key, cleared on removal; zero denotes no registration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CCB4 (0x8002CCB4).
void collisionGridUnregisterObject(u8 *object, s32 *registrationKey);

/// Find an overlapping registered candidate and request applicable collision responses.
/// \param object Live object with initialized collision radius and world position.
/// \returns Borrowed last overlapping candidate visited, or null when no eligible overlap is found.
/// \pre The collision registry is initialized and its objects remain live during the query.
/// \note The query may stop early when response capacity is exhausted.
/// \note Queries share registry iteration state and must not nest.
/// \note Original decompilation identifier: \c func_8002CCF8 (0x8002CCF8).
u8 *collisionGridFindOverlap(u8 *object);

/// Initialize storage and alignment for the collision candidate registry.
/// \returns Nothing.
/// \pre The game heap is initialized and no live registry allocation is being replaced.
/// \note Original decompilation identifier: \c func_8002CFA8 (0x8002CFA8).
void collisionGridInitialize(void);

/// Release the collision candidate registry's storage.
/// \returns Nothing.
/// \pre Object registrations have been retired and the registry will not be queried until reinitialized.
/// \note Original decompilation identifier: \c func_8002CFE0 (0x8002CFE0).
void collisionGridDestroy(void);

/// Randomize the spatial-hash alignment for subsequent registrations and queries.
/// \returns Nothing.
/// \note Existing registration keys remain unchanged; object updates refresh registrations for the new alignment.
/// \note Original decompilation identifier: \c func_8002CED0 (0x8002CED0).
void collisionGridRandomizeAlignment(void);

#endif
