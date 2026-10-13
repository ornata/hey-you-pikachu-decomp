/// \file
/// Declares spawning of the scripted Togepi actor.

#ifndef TOGEPI_H
#define TOGEPI_H

#include <ultra64.h>

/// Create and register Togepi with scale adjusted for the current scene and progress.
/// \param x Initial world-space X coordinate.
/// \param y Initial world-space Y coordinate.
/// \param z Initial world-space Z coordinate.
/// \param pitch Initial pitch adjustment in degrees.
/// \param yaw Initial yaw adjustment in degrees.
/// \param script Script configuration, or null to omit script state.
/// \param scriptArgument Argument retained for Togepi's script.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001E76C (0x8001E76C).
void togepiCreate(f32 x, f32 y, f32 z, f32 pitch, f32 yaw, u8 *script, s32 scriptArgument);

#endif
