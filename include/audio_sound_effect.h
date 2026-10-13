/// \file
/// Declares game sound-effect playback and per-handle volume and pan controls.

#ifndef AUDIO_SOUND_EFFECT_H
#define AUDIO_SOUND_EFFECT_H

#include <ultra64.h>

/// Start a sound effect using its bank index.
/// \param soundId Valid sound-effect index in the loaded bank.
/// \returns The playback identifier, or zero if no playback slot is available.
/// \note Original decompilation identifier: \c func_8001FE48 (0x8001FE48).
s32 audioPlaySoundEffect(s32 soundId);

/// Adjust a sound effect's volume immediately when newly started, or over ten milliseconds otherwise.
/// \param id Playback identifier; zero selects no playback.
/// \param volume Volume multiplier in units of 1/128 of the original level.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FE78 (0x8001FE78).
void audioSetSoundEffectVolume(s32 id, s32 volume);

/// Set the stereo position of a sound effect.
/// \param id Playback identifier; zero selects no playback.
/// \param pan Stereo position on a zero-to-255 left-to-right scale.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FEAC (0x8001FEAC).
void audioSetSoundEffectPan(s32 id, s32 pan);

#endif
