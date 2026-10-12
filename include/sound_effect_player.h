/// \file
/// Declares sound-effect playback, controls and player registration lifetimes.

#ifndef SOUND_EFFECT_PLAYER_H
#define SOUND_EFFECT_PLAYER_H

#include "n_synthInternals.h"

/// Opaque audio-heap-owned sound-effect player.
typedef struct SoundEffectPlayer SoundEffectPlayer;

/// Start a sound effect in an available playback slot.
/// \param player Initialized sound-effect player.
/// \param sound Sound definition and sample data that remain valid throughout playback.
/// \returns The assigned playback identifier, or zero when no slot is available.
/// \note Original decompilation identifier: \c func_800206B0 (0x800206B0).
s32 soundEffectPlayerPlay(SoundEffectPlayer *player, ALSound *sound);

/// Stop all active sound effects owned by the player.
/// \param player Initialized sound-effect player.
/// \returns The number of playback slots stopped.
/// \note Original decompilation identifier: \c func_80020AA0 (0x80020AA0).
s32 soundEffectPlayerStopAll(SoundEffectPlayer *player);

/// Stop sound effects associated with a playback identifier.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \returns The number of matching playback slots stopped.
/// \note Original decompilation identifier: \c func_80020B68 (0x80020B68).
s32 soundEffectPlayerStop(SoundEffectPlayer *player, s32 id);

/// Set the stereo position of a sound effect.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param pan Stereo position from zero at left to 127 at right, with 64 centered; retained while mono mode is active.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020C48 (0x80020C48).
s32 soundEffectPlayerSetPan(SoundEffectPlayer *player, s32 id, s32 pan);

/// Set a sound effect's pitch relative to its original playback pitch.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param pitch Playback pitch multiplier; one preserves the original pitch,
/// and the multiplier also affects the playback lifetime.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020D40 (0x80020D40).
s32 soundEffectPlayerSetPitch(SoundEffectPlayer *player, s32 id, f64 pitch);

/// Adjust a sound effect's volume over a requested transition.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param volume Volume multiplier in units of 1/128 of the original level.
/// \param duration Transition time in microseconds; a negative value chooses immediate adjustment for a new voice or a 10000-microsecond transition otherwise.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020E44 (0x80020E44).
s32 soundEffectPlayerSetVolume(SoundEffectPlayer *player, s32 id, s32 volume, s32 duration);

/// Set the amount of synthesizer effect applied to a sound effect.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param mix Effect mix level from zero to 127.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020FD8 (0x80020FD8).
s32 soundEffectPlayerSetEffectMix(SoundEffectPlayer *player, s32 id, s32 mix);

/// Count all active sound-effect playback slots.
/// \param player Initialized sound-effect player.
/// \returns The number of active playback slots.
/// \note Original decompilation identifier: \c func_800210BC (0x800210BC).
s32 soundEffectPlayerCountActive(SoundEffectPlayer *player);

/// Count active sound-effect slots associated with a playback identifier.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \returns The number of matching slots, or zero for identifier zero.
/// \note Original decompilation identifier: \c func_8002110C (0x8002110C).
s32 soundEffectPlayerCountPlayback(SoundEffectPlayer *player, s32 id);

/// Select stereo positioning or centered mono output for sound effects.
/// \param player Initialized sound-effect player.
/// \param mode Output mode: two uses individual pan settings; other values center all voices.
/// \returns The number of active slots whose pan was refreshed, or zero if the mode was unchanged.
/// \note Original decompilation identifier: \c func_800212A0 (0x800212A0).
s32 soundEffectPlayerSetOutputMode(SoundEffectPlayer *player, s32 mode);

/// Set the master level used for subsequent sound starts and volume adjustments.
/// \param player Initialized sound-effect player.
/// \param volume Master volume from zero for silence to 32767 for full level.
/// \returns One.
/// \note Original decompilation identifier: \c func_80021380 (0x80021380).
s32 soundEffectPlayerSetMasterVolume(SoundEffectPlayer *player, s32 volume);

/// Create and register a stereo sound-effect player with a fixed playback capacity.
/// \param synth Synthesizer context address retained by the player.
/// \param heap Audio heap providing the player and playback storage; must remain
/// valid until the player is unregistered and all of its voices are stopped.
/// \param count Nonnegative number of simultaneous playback slots to allocate.
/// \returns The initialized and registered sound-effect player.
/// \note Original decompilation identifier: \c func_8002138C (0x8002138C).
SoundEffectPlayer *soundEffectPlayerCreate(N_ALGlobals *synth, ALHeap *heap, s32 count);

/// Unregister a sound-effect player from synthesizer updates.
/// Active voices and caller-owned player storage are not released.
/// \param player Address of the registered sound-effect player.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800214BC (0x800214BC).
s32 soundEffectPlayerUnregister(SoundEffectPlayer *player);

#endif
