/// \file
/// Declares actor-kind creation and replacement with retained script ownership.

#ifndef ACTOR_CREATE_H
#define ACTOR_CREATE_H

#include <ultra64.h>
#include "actor_kind.h"

/// Create and register an actor of the requested kind.
/// \param kind ActorKind configuration to create; the search endpoint alias is not a valid creation kind.
/// \param x Initial world-space X coordinate.
/// \param y Initial world-space Y coordinate.
/// \param z Initial world-space Z coordinate.
/// \param pitch Initial pitch adjustment in degrees.
/// \param yaw Initial yaw adjustment in degrees.
/// \param scale Uniform actor scale.
/// \param script Script configuration whose referenced program and command tables remain valid for the script lifetime, or null to omit script state.
/// \param scriptArgument Argument retained for the actor's script.
/// \returns Newly initialized actor registered with the runtime; its hierarchy is released through runtime destruction.
/// \note Texture data requires the renderer's normal transfer synchronization before use.
/// \note Original decompilation identifier: \c func_8001E1C4 (0x8001E1C4).
u8 *actorCreateForKind(s32 kind, f32 x, f32 y, f32 z, f32 pitch, f32 yaw, f32 scale, u8 *script, s32 scriptArgument);

/// Replace an actor with another kind while preserving its position, pitch, yaw, scale, and script ownership.
/// \param actor Live initialized actor whose script ownership transfers to the replacement; the original hierarchy awaits runtime destruction.
/// \param kind Valid actor kind for the replacement.
/// \returns Runtime-registered replacement actor owning the original script context, if present.
/// \note Existing borrowed references to the original actor are not retargeted. Callers must use the returned replacement.
/// \note Original decompilation identifier: \c func_8001E568 (0x8001E568).
u8 *actorReplaceKindPreservingScript(u8 *actor, s32 kind);

#endif
