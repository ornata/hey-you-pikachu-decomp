/// \file
/// Declares tint and texture-animation controls for the actor's tinted model part.

#ifndef ACTOR_PART_TEXTURE_H
#define ACTOR_PART_TEXTURE_H

#include "ultra64.h"

/// Set the tinted model part's primitive RGB color.
/// \param actor Live actor containing the initialized tinted model part.
/// \param red Red component from zero through 255.
/// \param green Green component from zero through 255.
/// \param blue Blue component from zero through 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013E6C (0x80013E6C).
void actorSetPartTint(u8 *actor, u8 red, u8 green, u8 blue);

/// Restore the tinted model part's default RGB color.
/// \param actor Live actor containing the initialized tinted model part.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013EB4 (0x80013EB4).
void actorResetPartTint(u8 *actor);

/// Select a texture-frame animation and playback rate for one model-part channel.
/// \param actor Live actor containing the initialized tinted model part.
/// \param speed Nonnegative finite animation time multiplier.
/// \param channel Texture channel from zero through two.
/// \param animation Valid index in the shared texture-animation program table.
/// \returns Nothing.
/// \note The animation program is borrowed for subsequent updates.
/// \note Original decompilation identifier: \c func_80014960 (0x80014960).
void actorSetPartTextureAnimation(u8 *actor, f32 speed, s32 channel, s32 animation);

#endif
