/// \file
/// Declares actor behavior flags and operations for enabling or clearing them.

#ifndef ACTOR_BEHAVIOR_H
#define ACTOR_BEHAVIOR_H

#include "ultra64.h"

/// Established actor behavior controls; positive settings and inhibition bits share this word.
typedef enum ActorBehaviorFlag {
    ACTOR_INHIBIT_TARGET_FACING = 1,           ///< Suppress child orientation toward the actor target.
    ACTOR_INHIBIT_CAMERA_FACING = 2,           ///< Suppress child orientation toward the active camera.
    ACTOR_INHIBIT_HORIZONTAL_MOVEMENT = 4,     ///< Clear horizontal displacement during terrain updates.
    ACTOR_INHIBIT_TERRAIN_AND_GRAVITY = 8,     ///< Bypass terrain constraints and gravity updates.
    ACTOR_BEHAVIOR_COOPERATION_ENABLED = 0x10, ///< Permit cooperation requests for actor kind sixteen.
    ACTOR_BEHAVIOR_PRESERVE_APPEARANCE = 0x40  ///< Retain selected appearance variants across action changes.
} ActorBehaviorFlag;

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
