/// \file
/// Declares ambient audio emitters and their borrowed spatial sources.

#ifndef AMBIENT_AUDIO_H
#define AMBIENT_AUDIO_H

#include "vector3_types.h"

/// Playback routing selected by an ambient emitter.
typedef enum AmbientAudioPlaybackKind {
    AMBIENT_AUDIO_SOUND_EFFECT = 0, ///< Start an individual sound effect.
    AMBIENT_AUDIO_PLAY_CUE = 1,     ///< Start a cue.
    AMBIENT_AUDIO_CHANGE_CUE = 2    ///< Replace the current cue.
} AmbientAudioPlaybackKind;

/// Create an ambient audio emitter with the requested playback and spatial settings.
/// \param position Three-component world-space emitter position.
/// \param kind Playback category: 0 for sound effects, 1 to play a cue, or 2 to
/// change the current cue.
/// \param delay Minimum delay between ambient sounds, in seconds.
/// \param delayRange Additional randomized delay range, in seconds.
/// \param firstSound First candidate sound identifier.
/// \param secondSound Second candidate sound identifier.
/// \param firstProbability Probability of choosing the first candidate sound.
/// \param sourceCount Number of source positions; a negative value together with
/// a null sourcePositions retains the emitter position as the sole source.
/// \param sourcePositions Borrowed source-position array, which must remain
/// valid while the emitter uses it. Null with a negative sourceCount selects the
/// default source; null with a nonnegative count disables spatial mixing.
/// \returns The created ambient emitter object.
/// \note Original decompilation identifier: \c func_80034094 (0x80034094).
u8 *ambientAudioCreate(f32 *position, s32 kind, f32 delay, f32 delayRange, s32 firstSound, s32 secondSound,
                       f32 firstProbability, s32 sourceCount, Vec3f *sourcePositions);

#endif
