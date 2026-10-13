/// \file
/// Defines application of saved audio levels and temporary gameplay volume scaling.

#ifndef AUDIO_SETTINGS_H
#define AUDIO_SETTINGS_H

#include <ultra64.h>

/// Apply the saved music level to initialized audio services.
/// \param multiplier Finite temporary scale; the scaled level must fit s32 before clamping.
/// \returns Nothing.
/// \note Saved settings require a positive maximum and integer products representable as s32.
/// \note Original decompilation identifier: \c func_801129E8 (0x801129E8).
void audioApplySavedMusicVolume(f32 multiplier);

/// Apply the saved sound-effect level to initialized audio services.
/// \param multiplier Finite temporary scale; the scaled level must fit s32 before clamping.
/// \returns Nothing.
/// \note Saved settings require a positive maximum and integer products representable as s32.
/// \note Original decompilation identifier: \c func_80112A98 (0x80112A98).
void audioApplySavedSoundEffectVolume(f32 multiplier);

#endif
