/// \file
/// Declares selective actor behavior inhibition and the established flag meanings.

#ifndef ACTOR_BEHAVIOR_H
#define ACTOR_BEHAVIOR_H

#include "ultra64.h"

/// Independently suppressible actor behaviors; other bits retain unresolved roles.
typedef enum ActorBehaviorInhibitFlag {
    ACTOR_INHIBIT_TARGET_FACING = 1,       ///< Suppress child orientation toward the actor target.
    ACTOR_INHIBIT_CAMERA_FACING = 2,       ///< Suppress child orientation toward the active camera.
    ACTOR_INHIBIT_HORIZONTAL_MOVEMENT = 4, ///< Clear horizontal displacement during terrain updates.
    ACTOR_INHIBIT_TERRAIN_AND_GRAVITY = 8  ///< Bypass terrain constraints and gravity updates.
} ActorBehaviorInhibitFlag;

/// Enable actor behavior inhibition flags.
/// \param actor Actor whose behavior inhibition flags are changed.
/// \param flags Behavior inhibition bits to change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80017190 (0x80017190).
void actorSetBehaviorInhibitFlags(u8 *actor, u16 flags);

/// Disable actor behavior inhibition flags.
/// \param actor Actor whose behavior inhibition flags are changed.
/// \param flags Behavior inhibition bits to change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800171A8 (0x800171A8).
void actorClearBehaviorInhibitFlags(u8 *actor, u16 flags);

#endif
