#include "audio_output.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "n_synthInternals.h"
#include <PR/libaudio.h>
#include "sound_effect_player.h"

/// Private playback slot retaining a borrowed sound and its synthesizer voice.
typedef struct SoundEffectVoice {
    s32 id;                     ///< Playback handle, zero when unused.
    ALSound *sound;             ///< Borrowed definition and samples, valid throughout playback.
    N_ALVoice voice;            ///< Synthesizer voice allocated while this slot is active.
    s64 elapsed;                ///< Playback lifetime in scaled microseconds.
    s32 baseVolume, volume;     ///< Definition level and playback multiplier.
    f32 basePitch, pitch, rate; ///< Definition pitch, playback multiplier and variation.
    s32 mix;                    ///< Effect mix level.
    s32 pan;                    ///< Retained stereo position.
    s32 unknown04C;             ///< Unresolved trailing word; no meaning established.
} SoundEffectVoice;

/// Audio-heap-owned player and slot table; unregistering does not free either.
struct SoundEffectPlayer {
    N_ALGlobals *synth;       ///< Borrowed synthesizer context retained at creation.
    ALPlayer player;          ///< Registered lifetime-update client.
    s32 count, mode, volume;  ///< Slot capacity, output mode and master level.
    SoundEffectVoice *voices; ///< Owned contiguous playback slots.
};

typedef char SoundEffectVoiceSizeCheck[(sizeof(SoundEffectVoice) == 0x50) ? 1 : -1];
typedef char SoundEffectPlayerSizeCheck[(sizeof(SoundEffectPlayer) == 0x28) ? 1 : -1];
typedef char SoundEffectVoiceElapsedCheck[(((u32) & ((SoundEffectVoice *)0)->elapsed) == 0x28) ? 1 : -1];
typedef char SoundEffectPlayerSlotsCheck[(((u32) & ((SoundEffectPlayer *)0)->voices) == 0x24) ? 1 : -1];

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
s32 soundEffectPlayerPlay(SoundEffectPlayer *player, ALSound *sound) {
    s32 variation;
    SoundEffectPlayer *q = player;
    s32 id = 0;
    OSIntMask mask;
    ALVoiceConfig config;
    s32 i;
    s64 level;
    mask = osSetIntMask(OS_IM_NONE);
    for (i = 0; i < q->count; i++) {
        if (player->voices[i].id == 0) {
            player->voices[i].sound = sound;
            config.priority = 10;
            config.fxBus = 0;
            config.unityPitch = 0;
            player->voices[i].elapsed = 0;
            if (((u8 *)sound)[15] >= 2) {
                variation = (osGetCount() % (((u8 *)sound)[15] * 2 + 1)) - ((u8 *)sound)[15];
            } else {
                variation = 0;
            }
            level = (u32)variation + sound->sampleVolume;
            if (level < 0) {
                level = 0;
            }
            if (level > 255) {
                level = 255;
            }
            player->voices[i].baseVolume = level;
            player->voices[i].basePitch =
                alCents2Ratio((sound->keyMap->keyBase * -100) - sound->keyMap->detune + (sound->keyMap->keyMin * 100));
            if (sound->flags * 1200 / 255 >= 2) {
                variation = (osGetCount() % ((sound->flags * 1200 / 255) * 2 + 1)) - (sound->flags * 1200 / 255);
            } else {
                variation = 0;
            }
            player->voices[i].rate = alCents2Ratio(variation);
            player->voices[i].volume = 128;
            player->voices[i].pitch = 1.0F;
            player->voices[i].pan = sound->samplePan;
            player->voices[i].mix = 0;
            variation = gAudioNextPlaybackHandle;
            player->voices[i].id = variation;
            id = variation;
            gAudioNextPlaybackHandle++;
            player->voices[i].elapsed = 0;
            n_alSynAllocVoice(&player->voices[i].voice, &config);
            level = (s64)player->voices[i].baseVolume * player->voices[i].volume;
            level = (level * player->volume) >> 15;
            if (level > 32767) {
                level = 32767;
            }
            n_alSynStartVoiceParams(&player->voices[i].voice, sound->wavetable,
                                    player->voices[i].basePitch * player->voices[i].rate, level,
                                    player->mode == AUDIO_OUTPUT_STEREO ? player->voices[i].pan : AL_PAN_CENTER, player->voices[i].mix, 0);
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
s32 soundEffectPlayerStopAll(SoundEffectPlayer *player) {
    s32 n = 0;
    SoundEffectPlayer *q = player;
    s32 pad; /* unused, sizes the frame */
    OSIntMask mask;
    s32 i;

    mask = osSetIntMask(OS_IM_NONE);
    for (i = 0; i < q->count; i++) {
        if (player->voices[i].id != 0) {
            player->voices[i].id = 0;
            n_alSynStopVoice((u8 *)&player->voices[i].voice);
            n_alSynFreeVoice((u8 *)&player->voices[i].voice);
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
s32 soundEffectPlayerStop(SoundEffectPlayer *player, s32 id) {
    s32 n = 0;
    SoundEffectPlayer *q;
    s32 pad; /* unused, sizes the frame */
    OSIntMask mask;
    s32 i;

    if (id == 0) {
        return 0;
    }
    q = player;
    mask = osSetIntMask(OS_IM_NONE);
    for (i = 0; i < q->count; i++) {
        if (id == player->voices[i].id) {
            player->voices[i].id = 0;
            n_alSynStopVoice((u8 *)&player->voices[i].voice);
            n_alSynFreeVoice((u8 *)&player->voices[i].voice);
            n++;
        }
    }
    osSetIntMask(mask);
    return n;
}

void n_alSynSetPan(N_ALVoice *, u8);
/// Set the stereo position of a sound effect.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param pan Stereo position from zero at left to 127 at right, with 64 centered; retained while mono mode is active.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020C48 (0x80020C48).
s32 soundEffectPlayerSetPan(SoundEffectPlayer *player, s32 id, s32 pan) {
    s32 n = 0;
    SoundEffectPlayer *q;
    u8 effectivePan;
    OSIntMask mask;
    s32 i;
    if (id == 0) {
        return 0;
    }
    q = player;
    mask = osSetIntMask(OS_IM_NONE);
    for (i = 0; i < q->count; i++) {
        if (id == q->voices[i].id) {
            q->voices[i].pan = pan;
            if (q->mode == AUDIO_OUTPUT_STEREO) {
                n_alSynSetPan(&q->voices[i].voice, effectivePan = q->voices[i].pan);
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
s32 soundEffectPlayerSetPitch(SoundEffectPlayer *player, s32 id, f64 pitch) {
    s32 n = 0;
    SoundEffectPlayer *q;
    f32 effectivePitch;
    OSIntMask mask;
    s32 i;
    if (id == 0) {
        return 0;
    }
    q = player;
    mask = osSetIntMask(OS_IM_NONE);
    for (i = 0; i < q->count; i++) {
        if (id == q->voices[i].id) {
            effectivePitch = pitch;
            q->voices[i].pitch = effectivePitch;
            n_alSynSetPitch(&q->voices[i].voice, (q->voices[i].basePitch * q->voices[i].rate) * q->voices[i].pitch);
            n++;
        }
    }
    osSetIntMask(mask);
    return n;
}

void n_alSynSetVol(N_ALVoice *, s16, ALMicroTime);
/// Adjust a sound effect's volume over a requested transition.
/// \param player Initialized sound-effect player.
/// \param id Playback identifier; zero selects no playback.
/// \param volume Volume multiplier in units of 1/128 of the original level.
/// \param duration Transition time in microseconds; a negative value chooses immediate adjustment for a new voice or a 10000-microsecond transition otherwise.
/// \returns The number of matching playback slots updated.
/// \note Original decompilation identifier: \c func_80020E44 (0x80020E44).
s32 soundEffectPlayerSetVolume(SoundEffectPlayer *player, s32 id, s32 volume, s32 duration) {
    s32 n = 0;
    SoundEffectPlayer *q;
    s32 offset;
    OSIntMask mask;
    s64 level;
    s32 i;
    if (id == 0) {
        return 0;
    }
    q = player;
    mask = osSetIntMask(OS_IM_NONE);
    for (i = 0, offset = 0; i < q->count; i++, offset += sizeof(SoundEffectVoice)) {
        if (id == ((SoundEffectVoice *)((u8 *)q->voices + offset))->id) {
            if (duration < 0) {
                duration = ((SoundEffectVoice *)((u8 *)q->voices + offset))->elapsed == 0 ? 0 : 10000;
            }
            ((SoundEffectVoice *)((u8 *)q->voices + offset))->volume = volume;
            level = ((s64)((SoundEffectVoice *)((u8 *)q->voices + offset))->baseVolume *
                     ((SoundEffectVoice *)((u8 *)q->voices + offset))->volume * q->volume) >>
                    15;
            if (level > 32767) {
                level = 32767;
            }
            n_alSynSetVol(&((SoundEffectVoice *)((u8 *)q->voices + offset))->voice, level, duration);
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
s32 soundEffectPlayerSetEffectMix(SoundEffectPlayer *player, s32 id, s32 mix) {
    s32 n = 0;
    SoundEffectPlayer *q;
    u8 effectiveMix;
    OSIntMask mask;
    s32 i;
    if (id == 0) {
        return 0;
    }
    q = player;
    mask = osSetIntMask(OS_IM_NONE);
    for (i = 0; i < q->count; i++) {
        if (id == q->voices[i].id) {
            q->voices[i].mix = mix;
            n_alSynSetFXMix(&q->voices[i].voice, effectiveMix = q->voices[i].mix);
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
s32 soundEffectPlayerCountActive(SoundEffectPlayer *player) {
    s32 n = 0;
    SoundEffectPlayer *q;
    s32 i;

    q = player;
    for (i = 0; i < q->count; i++) {
        if (q->voices[i].id != 0) {
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
s32 soundEffectPlayerCountPlayback(SoundEffectPlayer *player, s32 id) {
    s32 n = 0;
    SoundEffectPlayer *q;
    s32 i;

    if (id == 0) {
        return 0;
    }
    q = player;
    for (i = 0; i < q->count; i++) {
        if (q->voices[i].id == id) {
            n++;
        }
    }
    return n;
}

/// Advance sound-effect lifetimes and stop effects whose configured duration has expired.
/// \param player Registered audio player whose client data is the sound-effect player.
/// \returns The interval until the next lifetime update, 5750 microseconds.
/// \note Original decompilation identifier: \c func_8002116C (0x8002116C).
ALMicroTime soundEffectPlayerUpdateLifetimes(void *player) {
    SoundEffectPlayer *state;
    s32 i;
    s32 duration;
    state = ((ALPlayer *)player)->clientData;
    for (i = 0; i < state->count; i++) {
        if (state->voices[i].id != 0) {
            duration = state->voices[i].sound->envelope->decayTime;
            if (duration >= 0) {
                if (state->voices[i].elapsed > duration + 10000) {
                    state->voices[i].id = 0;
                    n_alSynStopVoice((u8 *)&state->voices[i].voice);
                    n_alSynFreeVoice((u8 *)&state->voices[i].voice);
                } else {
                    state->voices[i].elapsed += state->voices[i].rate * state->voices[i].pitch * 5750.0F;
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
s32 soundEffectPlayerSetOutputMode(SoundEffectPlayer *player, s32 mode) {
    s32 changed = 0;
    SoundEffectPlayer *q = player;
    s32 i;
    OSIntMask mask;
    s32 pan;
    if (q->mode != mode) {
        q->mode = mode;
        mask = osSetIntMask(OS_IM_NONE);
        for (i = 0; i < q->count; i++) {
            if (q->voices[i].id != 0) {
                if (q->mode == AUDIO_OUTPUT_STEREO) {
                    pan = q->voices[i].pan;
                } else {
                    pan = AL_PAN_CENTER;
                }
                n_alSynSetPan(&q->voices[i].voice, pan);
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
s32 soundEffectPlayerSetMasterVolume(SoundEffectPlayer *player, s32 volume) {
    (player)->volume = volume;
    return 1;
}

void n_alSynAddPlayer(ALPlayer *);
/// Create and register a stereo sound-effect player with a fixed playback capacity.
/// \param synth Synthesizer context address retained by the player.
/// \param heap Audio heap providing the player and playback storage; must remain
/// valid until the player is unregistered and all of its voices are stopped.
/// \param count Nonnegative number of simultaneous playback slots to allocate.
/// \returns The initialized and registered sound-effect player.
/// \note Original decompilation identifier: \c func_8002138C (0x8002138C).
SoundEffectPlayer *soundEffectPlayerCreate(N_ALGlobals *synth, ALHeap *heap, s32 count) {
    SoundEffectPlayer *p;
    s32 i;
    p = alHeapDBAlloc(NULL, 0, heap, 1, sizeof(SoundEffectPlayer));
    p->synth = synth;
    p->count = count;
    p->volume = 0x7FFF;
    p->mode = AUDIO_OUTPUT_STEREO;
    p->voices = alHeapDBAlloc(NULL, 0, heap, count, sizeof(SoundEffectVoice));
    for (i = 0; i < count; i++) {
        p->voices[i].id = 0;
    }
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
s32 soundEffectPlayerUnregister(SoundEffectPlayer *player) {
    n_alSynRemovePlayer(&player->player);
    return 0;
}
