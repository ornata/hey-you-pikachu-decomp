/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "n_synthInternals.h"
#include <PR/libaudio.h>

typedef struct {
    s32 first;
    u8 pad[0x4C];
} Ent210BC;
typedef struct {
    s32 synth;
    ALPlayer player;
    s32 count;
    s32 mode;
    s32 volume;
    Ent210BC *arr;
} S210BC;

typedef struct {
    s32 id;
    ALSound *sound;
    N_ALVoice voice;
    s64 elapsed;
    s32 baseVolume, volume;
    f32 basePitch, pitch, rate;
    s32 mix, pan, flags;
} InitFxEntry;
typedef struct {
    s32 synth;
    ALPlayer player;
    s32 count, mode, volume;
    InitFxEntry *arr;
} InitFxPlayer;
/// Next shared playback identifier assigned to sequences and sound effects.
/// \note Original identifier: \c D_80088EF4 (0x80088EF4).
extern s32 gAudioNextPlaybackHandle;
s32 n_alSynAllocVoice(N_ALVoice *, ALVoiceConfig *);
void n_alSynStartVoiceParams(N_ALVoice *, ALWaveTable *, f32, s16, ALPan, u8, ALMicroTime);
/// Start a sound effect in an available playback slot.
/// \param player Initialized sound-effect player.
/// \param sound Sound definition and sample data that remain valid throughout playback.
/// \returns The assigned playback identifier, or zero when no slot is available.
/// \note Original decompilation identifier: \c func_800206B0 (0x800206B0).
s32 soundEffectPlayerPlay(InitFxPlayer *player, ALSound *sound) {
    s32 variation;
    InitFxPlayer *q = player;
    s32 id = 0;
    OSIntMask mask;
    ALVoiceConfig config;
    s32 i;
    s64 level;
    mask = osSetIntMask(1);
    for (i = 0; i < q->count; i++) {
        if (player->arr[i].id == 0) {
            player->arr[i].sound = sound;
            config.priority = 10;
            config.fxBus = 0;
            config.unityPitch = 0;
            player->arr[i].elapsed = 0;
            if (((u8 *)sound)[15] >= 2) {
                variation = osGetCount() % (((u8 *)sound)[15] * 2 + 1) - ((u8 *)sound)[15];
            } else {
                variation = 0;
            }
            level = (u32)variation + sound->sampleVolume;
            if (level < 0)
                level = 0;
            if (level > 255)
                level = 255;
            player->arr[i].baseVolume = level;
            player->arr[i].basePitch =
                alCents2Ratio(sound->keyMap->keyBase * -100 - sound->keyMap->detune + sound->keyMap->keyMin * 100);
            if (sound->flags * 1200 / 255 >= 2) {
                variation = osGetCount() % ((sound->flags * 1200 / 255) * 2 + 1) - (sound->flags * 1200 / 255);
            } else {
                variation = 0;
            }
            player->arr[i].rate = alCents2Ratio(variation);
            player->arr[i].volume = 128;
            player->arr[i].pitch = 1.0f;
            player->arr[i].pan = sound->samplePan;
            player->arr[i].mix = 0;
            variation = gAudioNextPlaybackHandle;
            player->arr[i].id = variation;
            id = variation;
            gAudioNextPlaybackHandle++;
            player->arr[i].elapsed = 0;
            n_alSynAllocVoice(&player->arr[i].voice, &config);
            level = (s64)player->arr[i].baseVolume * player->arr[i].volume;
            level = (level * player->volume) >> 15;
            if (level > 32767)
                level = 32767;
            n_alSynStartVoiceParams(&player->arr[i].voice, sound->wavetable,
                                    player->arr[i].basePitch * player->arr[i].rate, level,
                                    player->mode == 2 ? player->arr[i].pan : 64, player->arr[i].mix, 0);
            break;
        }
    }
    osSetIntMask(mask);
    return id;
}

void n_alSynStopVoice(u8 *);
void n_alSynFreeVoice(u8 *);

/// Stop all active sound effects owned by the player.
/// \param player Initialized sound-effect player.
/// \returns The number of playback slots stopped.
/// \note Original decompilation identifier: \c func_80020AA0 (0x80020AA0).
s32 soundEffectPlayerStopAll(S210BC *player) {
    s32 n = 0;
    S210BC *q = player;
    s32 pad; /* unused, sizes the frame */
    OSIntMask mask;
    s32 i;

    mask = osSetIntMask(1);
    for (i = 0; i < q->count; i++) {
        if (player->arr[i].first != 0) {
            player->arr[i].first = 0;
            n_alSynStopVoice(player->arr[i].pad + 4);
            n_alSynFreeVoice(player->arr[i].pad + 4);
            n++;
        }
    }
    osSetIntMask(mask);
    return n;
}

/// Stop sound effects associated with a playback identifier.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \returns The number of matching playback slots stopped.
/// \note Original decompilation identifier: \c func_80020B68 (0x80020B68).
s32 soundEffectPlayerStop(S210BC *player, s32 id) {
    s32 n = 0;
    S210BC *q;
    s32 pad; /* unused, sizes the frame */
    OSIntMask mask;
    s32 i;

    if (id == 0) {
        return 0;
    }
    q = player;
    mask = osSetIntMask(1);
    for (i = 0; i < q->count; i++) {
        if (id == player->arr[i].first) {
            player->arr[i].first = 0;
            n_alSynStopVoice(player->arr[i].pad + 4);
            n_alSynFreeVoice(player->arr[i].pad + 4);
            n++;
        }
    }
    osSetIntMask(mask);
    return n;
}

typedef struct {
    s32 id;
    ALSound *sound;
    N_ALVoice voice;
    s64 elapsed;
    s32 baseVolume, volume;
    f32 basePitch, pitch, rate;
    s32 mix, pan, flags;
} FxEntry;
typedef struct {
    s32 synth;
    ALPlayer player;
    s32 count, mode, volume;
    FxEntry *arr;
} FxPlayer;
void n_alSynSetPan(N_ALVoice *, u8);
/// Set the stereo position of a sound effect.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param pan Stereo position from zero at left to 127 at right, with 64 centered; retained while mono mode is active.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020C48 (0x80020C48).
s32 soundEffectPlayerSetPan(FxPlayer *player, s32 id, s32 pan) {
    s32 n = 0;
    FxPlayer *q;
    u8 effectivePan;
    OSIntMask mask;
    s32 i;
    if (id == 0)
        return 0;
    q = player;
    mask = osSetIntMask(1);
    for (i = 0; i < q->count; i++) {
        if (id == q->arr[i].id) {
            q->arr[i].pan = pan;
            if (q->mode == 2) {
                n_alSynSetPan(&q->arr[i].voice, effectivePan = q->arr[i].pan);
            }
            n++;
        }
    }
    osSetIntMask(mask);
    return n;
}

void n_alSynSetPitch(N_ALVoice *, f32);
/// Set a sound effect's pitch relative to its original playback pitch.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param pitch Playback pitch multiplier; one preserves the original pitch,
/// and the multiplier also affects the playback lifetime.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020D40 (0x80020D40).
s32 soundEffectPlayerSetPitch(FxPlayer *player, s32 id, f64 pitch) {
    s32 n = 0;
    FxPlayer *q;
    f32 effectivePitch;
    OSIntMask mask;
    s32 i;
    if (id == 0) {
        return 0;
    }
    q = player;
    mask = osSetIntMask(1);
    for (i = 0; i < q->count; i++) {
        if (id == q->arr[i].id) {
            effectivePitch = pitch;
            q->arr[i].pitch = effectivePitch;
            n_alSynSetPitch(&q->arr[i].voice, (q->arr[i].basePitch * q->arr[i].rate) * q->arr[i].pitch);
            n++;
        }
    }
    osSetIntMask(mask);
    return n;
}

typedef struct {
    s32 id;
    ALSound *sound;
    N_ALVoice voice;
    s64 elapsed;
    s32 baseVolume;
    s32 volume;
    f32 basePitch;
    f32 pitch;
    f32 rate;
    u8 unknown44[0xC];
} VolumeVoice;
typedef struct {
    s32 synth;
    ALPlayer player;
    s32 count;
    s32 mode;
    s32 volume;
    VolumeVoice *voices;
} VolumePlayer;
void n_alSynSetVol(N_ALVoice *, s16, ALMicroTime);
/// Adjust a sound effect's volume over a requested transition.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param volume Volume multiplier in units of 1/128 of the original level.
/// \param duration Transition time in microseconds; a negative value chooses immediate adjustment for a new voice or a 10000-microsecond transition otherwise.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020E44 (0x80020E44).
s32 soundEffectPlayerSetVolume(VolumePlayer *player, s32 id, s32 volume, s32 duration) {
    s32 n = 0;
    VolumePlayer *q;
    s32 offset;
    OSIntMask mask;
    s64 level;
    s32 i;
    if (id == 0) {
        return 0;
    }
    q = player;
    mask = osSetIntMask(1);
    for (i = 0, offset = 0; i < q->count; i++, offset += sizeof(VolumeVoice)) {
        if (id == ((VolumeVoice *)((u8 *)q->voices + offset))->id) {
            if (duration < 0) {
                duration = ((VolumeVoice *)((u8 *)q->voices + offset))->elapsed == 0 ? 0 : 10000;
            }
            ((VolumeVoice *)((u8 *)q->voices + offset))->volume = volume;
            level = ((s64)((VolumeVoice *)((u8 *)q->voices + offset))->baseVolume *
                     ((VolumeVoice *)((u8 *)q->voices + offset))->volume * q->volume) >>
                    15;
            if (level > 32767) {
                level = 32767;
            }
            n_alSynSetVol(&((VolumeVoice *)((u8 *)q->voices + offset))->voice, level, duration);
            n++;
        }
    }
    osSetIntMask(mask);
    return n;
}

void n_alSynSetFXMix(N_ALVoice *, u8);
/// Set the amount of synthesizer effect applied to a sound effect.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param mix Effect mix level from zero to 127.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020FD8 (0x80020FD8).
s32 soundEffectPlayerSetEffectMix(FxPlayer *player, s32 id, s32 mix) {
    s32 n = 0;
    FxPlayer *q;
    u8 effectiveMix;
    OSIntMask mask;
    s32 i;
    if (id == 0)
        return 0;
    q = player;
    mask = osSetIntMask(1);
    for (i = 0; i < q->count; i++) {
        if (id == q->arr[i].id) {
            q->arr[i].mix = mix;
            n_alSynSetFXMix(&q->arr[i].voice, effectiveMix = q->arr[i].mix);
            n++;
        }
    }
    osSetIntMask(mask);
    return n;
}

/// Count all active sound-effect playback slots.
/// \param player Initialized sound-effect player.
/// \returns The number of active playback slots.
/// \note Original decompilation identifier: \c func_800210BC (0x800210BC).
s32 soundEffectPlayerCountActive(S210BC *player) {
    s32 n = 0;
    S210BC *q;
    s32 i;

    q = player;
    for (i = 0; i < q->count; i++) {
        if (q->arr[i].first != 0) {
            n++;
        }
    }
    return n;
}

/// Count active sound-effect slots associated with a playback identifier.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \returns The number of matching slots, or zero for identifier zero.
/// \note Original decompilation identifier: \c func_8002110C (0x8002110C).
s32 soundEffectPlayerCountPlayback(S210BC *player, s32 id) {
    s32 n = 0;
    S210BC *q;
    s32 i;

    if (id == 0) {
        return 0;
    }
    q = player;
    for (i = 0; i < q->count; i++) {
        if (q->arr[i].first == id) {
            n++;
        }
    }
    return n;
}

typedef struct {
    s32 id;
    ALSound *sound;
    u8 voice[0x20];
    s64 elapsed;
    u8 unknown30[0xC];
    f32 pitch;
    f32 rate;
    u8 unknown44[0xC];
} TimedVoice;
typedef struct {
    s32 synth;
    ALPlayer player;
    s32 count;
    s32 mode;
    s32 volume;
    TimedVoice *voices;
} TimedPlayer;
/// Advance sound-effect lifetimes and stop effects whose configured duration has expired.
/// \param player Registered audio player whose client data is the sound-effect player.
/// \returns The interval until the next lifetime update, 5750 microseconds.
/// \note Original decompilation identifier: \c func_8002116C (0x8002116C).
s32 soundEffectPlayerUpdateLifetimes(ALPlayer *player) {
    TimedPlayer *state;
    s32 i;
    s32 duration;
    state = player->clientData;
    for (i = 0; i < state->count; i++) {
        if (state->voices[i].id != 0) {
            duration = state->voices[i].sound->envelope->decayTime;
            if (duration >= 0) {
                if (state->voices[i].elapsed > duration + 10000) {
                    state->voices[i].id = 0;
                    n_alSynStopVoice(state->voices[i].voice);
                    n_alSynFreeVoice(state->voices[i].voice);
                } else {
                    state->voices[i].elapsed += state->voices[i].rate * state->voices[i].pitch * 5750.0f;
                }
            }
        }
    }
    return 5750;
}

/// Select stereo positioning or centered mono output for sound effects.
/// \param player Initialized sound-effect player.
/// \param mode Output mode: two uses individual pan settings; other values center all voices.
/// \returns The number of active slots whose pan was refreshed, or zero if the mode was unchanged.
/// \note Original decompilation identifier: \c func_800212A0 (0x800212A0).
s32 soundEffectPlayerSetOutputMode(FxPlayer *player, s32 mode) {
    s32 changed = 0;
    FxPlayer *q = player;
    s32 i;
    OSIntMask mask;
    s32 pan;
    if (q->mode != mode) {
        q->mode = mode;
        mask = osSetIntMask(1);
        for (i = 0; i < q->count; i++) {
            if (q->arr[i].id != 0) {
                if (q->mode == 2) {
                    pan = q->arr[i].pan;
                } else {
                    pan = 0x40;
                }
                n_alSynSetPan(&q->arr[i].voice, pan);
                changed++;
            }
        }
        osSetIntMask(mask);
    }
    return changed;
}

/// Set the master level used for subsequent sound starts and volume adjustments.
/// \param player Initialized sound-effect player.
/// \param volume Master volume from zero for silence to 32767 for full level.
/// \returns One.
/// \note Original decompilation identifier: \c func_80021380 (0x80021380).
s32 soundEffectPlayerSetMasterVolume(u8 *player, s32 volume) {
    ((InitFxPlayer *)player)->volume = volume;
    return 1;
}

s32 soundEffectPlayerUpdateLifetimes(ALPlayer *);
void n_alSynAddPlayer(ALPlayer *);
/// Create and register a stereo sound-effect player with a fixed playback capacity.
/// \param synth Synthesizer context address retained by the player.
/// \param heap Audio heap providing the player and playback storage; must remain
/// valid until the player is unregistered and all of its voices are stopped.
/// \param count Nonnegative number of simultaneous playback slots to allocate.
/// \returns The initialized and registered sound-effect player.
/// \note Original decompilation identifier: \c func_8002138C (0x8002138C).
S210BC *soundEffectPlayerCreate(s32 synth, ALHeap *heap, s32 count) {
    S210BC *p;
    s32 i;
    p = alHeapDBAlloc(NULL, 0, heap, 1, sizeof(S210BC));
    p->synth = synth;
    p->count = count;
    p->volume = 0x7FFF;
    p->mode = 2;
    p->arr = alHeapDBAlloc(NULL, 0, heap, count, sizeof(Ent210BC));
    for (i = 0; i < count; i++)
        p->arr[i].first = 0;
    p->player.clientData = p;
    p->player.handler = soundEffectPlayerUpdateLifetimes;
    n_alSynAddPlayer(&p->player);
    return p;
}

void n_alSynRemovePlayer(ALPlayer *); /* extern */

/// Unregister a sound-effect player from synthesizer updates.
/// Active voices and caller-owned player storage are not released.
/// \param player Address of the registered sound-effect player.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800214BC (0x800214BC).
s32 soundEffectPlayerUnregister(s32 player) {
    n_alSynRemovePlayer((ALPlayer *)(player + 4));
    return 0;
}
