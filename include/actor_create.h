/// \file
/// Declares actor-kind creation and replacement with retained script ownership.

#ifndef ACTOR_CREATE_H
#define ACTOR_CREATE_H

#include <ultra64.h>

/// Create and register an actor of the requested kind with its resources ready for use.
/// \param kind Valid actor kind to create.
/// \param x Initial world-space X coordinate.
/// \param y Initial world-space Y coordinate.
/// \param z Initial world-space Z coordinate.
/// \param pitch Initial pitch adjustment in degrees.
/// \param yaw Initial yaw adjustment in degrees.
/// \param scale Uniform actor scale.
/// \param script Script configuration whose referenced program and command tables remain valid for the script lifetime, or null to omit script state.
/// \param scriptArgument Argument retained for the actor's script.
/// \returns Newly initialized actor registered with the runtime; its hierarchy is released through runtime destruction.
/// \note Original decompilation identifier: \c func_8001E1C4 (0x8001E1C4).
u8 *actorCreateForKind(s32 kind, f32 x, f32 y, f32 z, f32 pitch, f32 yaw, f32 scale, u8 *script, s32 scriptArgument);

/// Replace an actor with another kind while preserving its position, pitch, yaw, scale, and script ownership.
/// \param actor Actor whose script is transferred and whose hierarchy is scheduled for destruction.
/// \param kind Valid actor kind for the replacement.
/// \returns Runtime-registered replacement actor; the original hierarchy awaits runtime destruction.
/// \note Original decompilation identifier: \c func_8001E568 (0x8001E568).
u8 *actorReplaceKindPreservingScript(u8 *actor, s32 kind);

#endif
