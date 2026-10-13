#include "audio_bank.h"
#include "sequence_registration.h"
#include "sequence_engine_types.h"
#include "sequence_playback.h"
#include "sequence_control.h"
#include "audio_sound_effect.h"
#include "audio_playback_stop.h"
#include "audio_playback_query.h"
#include "common.h"
#include "sound_effect_player.h"
#include "n_synthInternals.h"
#include "sequence_driver.h"
#include "midi_sequence_types.h"
/// One audio sequence slot with embedded MIDI control state and a playback handle.
/// The engine borrows its MIDI control state, instrument bank, and event buffer.
/// Four slots share one contiguous allocation. The slot owns the event buffer borrowed by engine registration.
typedef struct AudioSequenceSlot {
    EngineSequence engine; ///< Embedded registered sequence engine state.
    SequenceControlState player; ///< Embedded MIDI reader and queued playback controls.
    s32 handle; ///< Playback handle issued by audioPlaySequence, or zero when free.
    u8 eventBuffer[0x14]; ///< MIDI event bytes borrowed by the registered engine.
} AudioSequenceSlot;

typedef char AudioSequenceSlotSizeCheck[(sizeof(AudioSequenceSlot) == 0x7F0) ? 1 : -1];
typedef char AudioSequenceSlotPlayerOffsetCheck[((u32) & ((AudioSequenceSlot *)0)->player == 0x200) ? 1 : -1];
typedef char AudioSequenceSlotHandleOffsetCheck[((u32) & ((AudioSequenceSlot *)0)->handle == 0x7D8) ? 1 : -1];
/// Four sequence playback slots retaining control state and playback handles.
/// \note Original identifier: \c D_800EC9F0 (0x800EC9F0).
extern AudioSequenceSlot gAudioSequenceSlots[4];
/// Boundary symbol immediately after the four sequence playback slots.
/// \note Original identifier: \c D_800EE9B0 (0x800EE9B0).
extern AudioSequenceSlot gAudioSequenceSlotsEnd; /* one past D_800EC9F0[3] */

typedef struct ReverbSectionInit {
    s32 inputMs, outputMs;
    s32 feedback, feedforward, gain, chorusRate, chorusDepth, filter;
} ReverbSectionInit;
typedef struct ReverbInit {
    s32 count, lengthMs;
    ReverbSectionInit sections[6];
} ReverbInit;
#define AUDIO_BANK_SIZE 0x11690 /* bank ROM D_84F5F0..0x860C80; samples follow */
/// Custom reverb section settings used when initializing game audio.
/// \note Original identifier: \c D_80063510 (0x80063510).
extern s32 gAudioReverbPreset;
/// Storage backing the game audio heap and its playback resources.
/// \note Original identifier: \c D_800B6E00 (0x800B6E00).
extern u8 gAudioHeapStorage[];
/// Nonzero while game audio resources and worker services are initialized.
/// \note Original identifier: \c D_80088EE4 (0x80088EE4).
extern s32 gAudioInitialized;
/// Game-owned synthesizer instance used by audio initialization and playback services.
/// \note Original identifier: \c D_800EC968 (0x800EC968).
extern N_ALGlobals gGameAudioSynthesizer;
/// Loaded audio bank file borrowed by sequence and sound-effect players.
/// \note Original identifier: \c D_80088EE0 (0x80088EE0).
extern ALBankFile *gAudioBankFile;
/// Owned sound-effect player allocated during game audio initialization.
/// \note Original identifier: \c D_800EC960 (0x800EC960).
extern SoundEffectPlayer *gSoundEffectPlayer;
/// ROM start of the audio bank definitions.
/// \note Original identifier: \c D_84F5F0 (0x84F5F0).
extern u8 gAudioBankRomStart[];
/// ROM start of audio samples following the bank definitions.
/// \note Original identifier: \c D_860C80 (0x860C80).
extern u8 gAudioSamplesRomStart[];
/// Audio RSP microcode text start; also marks the end of the RSP boot code.
/// \note Original identifier: \c D_80060120 (0x80060120).
extern u8 gAudioRspMicrocodeTextStart[];
/// Audio RSP microcode data start supplied to audio task initialization.
/// \note Original identifier: \c D_800A8130 (0x800A8130).
extern u8 gAudioRspMicrocodeDataStart[];
s32 audioDmaInitialize(ALHeap *, s32, s32);
ALDMAproc audioDmaCreateCallback(s32 *);
void n_alInit(N_ALGlobals *, ALSynConfig *);
Acmd *n_alAudioFrame(Acmd *, s32 *, s16 *, s32);
void audioInitializeSequencePlayers(s32, u8 *);
void audioWorkerStart(s32, ALHeap *, OSId, OSPri, s32, s32, s32, s32, s32);

/// Initialize game audio resources, playback services, and the audio worker.
/// \param scheduler Scheduler address used by the audio worker.
/// \param threadId Operating-system identifier for the audio worker thread.
/// \param priority Scheduling priority of the audio worker thread.
/// \param frequency Requested audio output frequency in hertz.
/// \returns Audio heap bytes used, or zero if audio was already initialized.
/// \note Original decompilation identifier: \c func_8001F680 (0x8001F680).
s32 audioInitialize(s32 scheduler, s32 threadId, s32 priority, u32 frequency) {
    ALHeap heap;
    ALSynConfig config;
    s32 i;
    s32 used;
    ReverbInit *preset;
    if (gAudioInitialized) {
        return 0;
    }
    preset = (ReverbInit *)&gAudioReverbPreset;
    alHeapInit(&heap, gAudioHeapStorage, 0x35B60);
    audioDmaInitialize(&heap, 0x40, 10);
    config.params = alHeapAlloc(&heap, preset->count * 8 + 2, sizeof(s32));
    config.params[0] = preset->count;
    config.params[1] = ((s32)(preset->lengthMs * frequency) / 1000) & ~7;
    for (i = 0; i < preset->count; i++) {
        config.params[i * 8 + 2] = ((s32)(preset->sections[i].inputMs * frequency) / 1000) & ~7;
        config.params[i * 8 + 3] = ((s32)(preset->sections[i].outputMs * frequency) / 1000) & ~7;
        config.params[i * 8 + 4] = preset->sections[i].feedback;
        config.params[i * 8 + 5] = preset->sections[i].feedforward;
        config.params[i * 8 + 6] = preset->sections[i].gain;
        config.params[i * 8 + 7] = preset->sections[i].chorusRate;
        config.params[i * 8 + 8] = preset->sections[i].chorusDepth;
        config.params[i * 8 + 9] = preset->sections[i].filter;
    }
    config.maxVVoices = 0;
    config.maxPVoices = 32;
    config.maxUpdates = 128;
    config.fxType = AL_FX_CUSTOM;
    config.heap = &heap;
    config.dmaproc = audioDmaCreateCallback;
    config.outputRate = osAiSetFrequency(frequency);
    n_alInit(&gGameAudioSynthesizer, &config);
    gAudioBankFile = audioBankLoad(
        &heap, (s32)gAudioBankRomStart, (s32)(gAudioBankRomStart + AUDIO_BANK_SIZE), (s32)gAudioSamplesRomStart);
    audioInitializeSequencePlayers((s32)&heap, (u8 *)gAudioBankFile);
    gSoundEffectPlayer = soundEffectPlayerCreate(&gGameAudioSynthesizer, &heap, 32);
    audioWorkerStart(scheduler, &heap, threadId, priority, (s32)n_alAudioFrame, (s32)gAudioRspMicrocodeTextStart,
                     (s32)gAudioRspMicrocodeDataStart, 0xB8, 0x200);
    gAudioInitialized = 1;
    used = heap.cur - heap.base;
    if (used > 0x35B60) {
        for (;;) {
        }
    }
    return used;
}

void audioWorkerStopAndWait(); /* extern */
void n_alClose(N_ALGlobals *);

/// Stop the audio worker and close the game synthesizer.
/// \returns Minus one if audio was shut down, or zero if it was already inactive.
/// \note Original decompilation identifier: \c func_8001F9DC (0x8001F9DC).
s32 audioShutdown(void) {
    if (gAudioInitialized == 0) {
        return 0;
    }
    gAudioInitialized = 0;
    audioWorkerStopAndWait();
    n_alClose(&gGameAudioSynthesizer);
    return -1;
}

/// Selected output mode shared by sequence and sound-effect playback.
/// \note Original identifier: \c D_80088EE8 (0x80088EE8).
extern s32 gAudioOutputMode;
/// Game sequence driver shared by the registered playback slots.
/// \note Original identifier: \c D_800EC9C8 (0x800EC9C8).
extern VoicePlayer gAudioSequenceDriver;

/// Set the output mode for sequence and sound-effect playback.
/// \param mode Two enables stereo positioning; other values use centered mono output.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FA28 (0x8001FA28).
void audioSetOutputMode(s32 mode) {
    gAudioOutputMode = mode;
    sequenceEngineSetOutputMode(&gAudioSequenceDriver, mode);
    soundEffectPlayerSetOutputMode(gSoundEffectPlayer, mode);
}


/// Get the selected game audio output mode.
/// \returns The current output mode.
/// \note Original decompilation identifier: \c func_8001FA6C (0x8001FA6C).
s32 audioGetOutputMode(void) {
    return gAudioOutputMode;
}

/// Voice storage supplied to the game sequence driver.
/// \note Original identifier: \c D_800EEA18 (0x800EEA18).
extern SequenceVoice gAudioSequenceVoices[];


/// Initialize the shared sequence synthesizer and its four playback slots.
/// \param heap Unused audio heap address.
/// \param bankFile Loaded audio bank file supplying sequence instruments.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FA78 (0x8001FA78).
void audioInitializeSequencePlayers(s32 heap, u8 *bankFile) {
    s32 i;
    AudioSequenceSlot *p;

    sequenceEngineInitializeDriver(&gAudioSequenceDriver, n_alGlobals, 2, 0x28, gAudioSequenceVoices);
    for (i = 0; i < 4; i++) {
        p = &gAudioSequenceSlots[i];
        sequenceControlInitialize(&gAudioSequenceSlots[i].player);
        p->handle = 0;
        sequenceEngineRegisterSequence(&p->engine, &gAudioSequenceDriver, *(ALBank **)(bankFile + 4), 0x14,
                                       gAudioSequenceSlots[i].eventBuffer, &gAudioSequenceSlots[i].player,
                                       sequenceControlReadEvent);
    }
}

/// Next shared playback identifier assigned to sequences and sound effects.
/// \note Original identifier: \c D_80088EF4 (0x80088EF4).
extern s32 gAudioNextPlaybackHandle;

/// Start a sequence at full volume in an available playback slot.
/// \param data Sequence data address, valid throughout playback.
/// \returns The playback identifier, or zero if all sequence slots are busy.
/// \note Original decompilation identifier: \c func_8001FB5C (0x8001FB5C).
s32 audioPlaySequence(s32 data) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (sequenceEngineIsStopped(&gAudioSequenceSlots[i].player.playback)) {
            gAudioSequenceSlots[i].handle = gAudioNextPlaybackHandle;
            sequenceControlStart(&gAudioSequenceSlots[i].player, data);
            sequenceControlRequestVolume(&gAudioSequenceSlots[i].player, 0.0, 1.0);
            return gAudioNextPlaybackHandle++;
        }
    }
    return 0;
}


/// Stop selected categories of game audio.
/// \param categories Category mask: bit zero selects sound effects and bit one selects sequences.
/// \param fadeSeconds Nonnegative sequence fade duration in seconds; sound effects stop immediately.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FC10 (0x8001FC10).
void audioStopCategories(s32 categories, f64 fadeSeconds) {
    s32 i;

    if (categories & 2) {
        for (i = 0; i < 4; i++) {
            if (gAudioSequenceSlots[i].handle != 0) {
                audioStopPlayback(gAudioSequenceSlots[i].handle, fadeSeconds);
            }
        }
    }
    if (categories & 1) {
        soundEffectPlayerStopAll(gSoundEffectPlayer);
    }
}

/// Count active playback slots in selected audio categories.
/// \param categories Category mask: bit zero selects sound effects and bit one selects sequences.
/// \returns The combined number of active sequence and sound-effect slots.
/// \note Original decompilation identifier: \c func_8001FCA4 (0x8001FCA4).
s32 audioCountActive(s32 categories) {
    s32 i;
    s32 var_s1 = 0;
    if (categories & 2) {
        for (i = 0; i < 4; i++) {
            var_s1 += sequenceEngineIsStopped(&gAudioSequenceSlots[i].player.playback) == 0;
        }
    }
    if (categories & 1) {
        var_s1 += soundEffectPlayerCountActive(gSoundEffectPlayer);
    }
    return var_s1;
}


/// Count active playback slots associated with an audio identifier.
/// \param id Playback identifier; zero selects no playback.
/// \returns One or zero for a sequence identifier, otherwise the number of matching sound-effect slots.
/// \note Original decompilation identifier: \c func_8001FD2C (0x8001FD2C).
s32 audioCountPlayback(s32 id) {
    s32 i;

    if (id == 0) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (id == gAudioSequenceSlots[i].handle) {
            return (u64)(sequenceEngineIsStopped(&gAudioSequenceSlots[i].player.playback) == 0);
        }
    }
    return soundEffectPlayerCountPlayback(gSoundEffectPlayer, id);
}

#include <PR/libaudio.h>

/// Find a sound-effect definition in the loaded audio bank.
/// \param id Nonnegative sound-effect index.
/// \returns A borrowed sound definition owned by the audio bank, or null if its instrument or sound index is out of range.
/// \note Original decompilation identifier: \c func_8001FD9C (0x8001FD9C).
ALSound *audioGetSoundDefinition(s32 id) {
    ALBank *bank = gAudioBankFile->bankArray[0];
    s32 prog = id / 88 + 124;
    s32 key;

    if (prog < bank->instCount) {
        key = id % 88;
        if (key < bank->instArray[prog]->soundCount) {
            return bank->instArray[prog]->soundArray[key];
        }
    }
    return NULL;
}


/// Start a sound effect using its bank index.
/// \param soundId Valid sound-effect index in the loaded bank.
/// \returns The playback identifier, or zero if no playback slot is available.
/// \note Original decompilation identifier: \c func_8001FE48 (0x8001FE48).
s32 audioPlaySoundEffect(s32 soundId) {
    return soundEffectPlayerPlay(gSoundEffectPlayer, audioGetSoundDefinition(soundId));
}


/// Adjust a sound effect's volume immediately when newly started, or over ten milliseconds otherwise.
/// \param id Playback identifier; zero selects no playback.
/// \param volume Volume multiplier in units of 1/128 of the original level.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FE78 (0x8001FE78).
void audioSetSoundEffectVolume(s32 id, s32 volume) {
    soundEffectPlayerSetVolume(gSoundEffectPlayer, id, volume, -1);
}

/// Set the stereo position of a sound effect.
/// \param id Playback identifier; zero selects no playback.
/// \param pan Stereo position on a zero-to-255 left-to-right scale.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FEAC (0x8001FEAC).
void audioSetSoundEffectPan(s32 id, s32 pan) {
    pan = pan * 127 / 256;
    soundEffectPlayerSetPan(gSoundEffectPlayer, id, pan);
}


/// Transpose a sound effect relative to its original pitch.
/// \param id Playback identifier; zero selects no playback.
/// \param semitones Pitch change in semitones; zero preserves the original pitch.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FEFC (0x8001FEFC).
void audioSetSoundEffectPitch(s32 id, f32 semitones) {
    soundEffectPlayerSetPitch(gSoundEffectPlayer, id, (f64)alCents2Ratio((s32)(semitones * 100.0f)));
}


/// Set the synthesizer effect mix for a sound effect.
/// \param id Playback identifier; zero selects no playback.
/// \param mix Effect mix level from zero to 127.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FF54 (0x8001FF54).
void audioSetSoundEffectMix(s32 id, s32 mix) {
    soundEffectPlayerSetEffectMix(gSoundEffectPlayer, id, mix);
}


/// Change a sequence's playback rate.
/// \param id Playback identifier; zero selects no playback.
/// \param rate Positive playback rate multiplier; one uses the original rate.
/// \returns One if the sequence identifier was found, otherwise zero.
/// \note Original decompilation identifier: \c func_8001FF84 (0x8001FF84).
s32 audioSetSequenceRate(s32 id, f64 rate) {
    s32 i;

    if (id == 0) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (id == gAudioSequenceSlots[i].handle) {
            sequenceEngineSetRate(&gAudioSequenceSlots[i].player.playback, rate);
            return 1;
        }
    }
    return 0;
}


/// Get the playback rate of a sequence.
/// \param id Playback identifier; zero selects no playback.
/// \returns The sequence playback rate, or minus one if the identifier was not found.
/// \note Original decompilation identifier: \c func_8001FFF4 (0x8001FFF4).
f64 audioGetSequenceRate(s32 id) {
    s32 i;

    if (id == 0) {
        return -1.0;
    }
    for (i = 0; i < 4; i++) {
        if (id == gAudioSequenceSlots[i].handle) {
            return sequenceEngineGetRate(&gAudioSequenceSlots[i].player.playback);
        }
    }
    return -1.0;
}


/// Set the master level for selected audio categories.
/// \param categories Category mask: bit zero selects sound effects and bit one selects sequences.
/// \param volume Master level: sequence volume uses a zero-to-65535 scale; sound effects use a zero-to-32767 scale.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002006C (0x8002006C).
void audioSetCategoryVolume(s32 categories, s32 volume) {
    if (categories & 2) {
        sequenceEngineSetMasterVolume(&gAudioSequenceDriver, (f64)volume / 65535.0);
    }
    if (categories & 1) {
        soundEffectPlayerSetMasterVolume(gSoundEffectPlayer, volume);
    }
}

s32 audioPlaySequence(s32);

// clang-format off
/// Start a sequence with a volume transition from thirty percent to full level.
/// \param data Sequence data address, valid throughout playback.
/// \param time Nonnegative fade duration in seconds; zero requests full volume immediately.
/// \returns The playback identifier, or zero if all sequence slots are busy.
///
/// \note NONPORTABLE: the loop setup shares a source line to preserve IDO scheduling.
/// \note Original decompilation identifier: \c func_800200E0 (0x800200E0).
s32 audioPlaySequenceWithFade(s32 data, f64 time) {
    s32 id;
    AudioSequenceSlot *entry;

    id = audioPlaySequence(data);
    entry = gAudioSequenceSlots; do { if (id == entry->handle) {
            sequenceControlRequestVolume(&entry->player, 0.0, 0.3);
            sequenceControlRequestVolume(&entry->player, time, 1.0);
        }
        entry++;
    } while (entry != &gAudioSequenceSlotsEnd);
    return id;
}
// clang-format on

/// Target sequence volume requested during a fade followed by scheduled stop.
/// \note Original identifier: \c D_800A4890 (0x800A4890).
extern f64 gAudioSequenceStopFadeTargetVolume;


// clang-format off
/// Fade a sequence and schedule it to stop.
/// \param id Playback identifier; zero selects no playback.
/// \param fadeSeconds Nonnegative fade duration in seconds; zero stops immediately.
/// \returns Zero.
///
/// \note NONPORTABLE: the initializers and loop head share a source line to preserve IDO scheduling.
/// \note Original decompilation identifier: \c func_800201B8 (0x800201B8).
s32 audioFadeOutSequence(s32 id, f64 fadeSeconds) {
    f64 vol;
    AudioSequenceSlot *e;

    if (id == 0) {
        return 0;
    }
    e = gAudioSequenceSlots; vol = gAudioSequenceStopFadeTargetVolume; do { if (id == e->handle) {
            sequenceControlRequestVolume(&e->player, fadeSeconds, vol);
            sequenceControlScheduleStop(&e->player, fadeSeconds);
        }
        e++;
    } while (e != &gAudioSequenceSlotsEnd);
    return 0;
}
// clang-format on

s32 audioFadeOutSequence(s32, f64);

/// Stop sound effects or fade out a sequence associated with an audio identifier.
/// \param id Playback identifier; zero selects no playback.
/// \param fadeSeconds Nonnegative sequence fade duration in seconds; sound effects stop immediately.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80020270 (0x80020270).
s32 audioStopPlayback(s32 id, f64 fadeSeconds) {
    if (id == 0) {
        return 0;
    }
    soundEffectPlayerStop(gSoundEffectPlayer, id);
    audioFadeOutSequence(id, fadeSeconds);
    return 0;
}

/// Count physical synthesizer voices currently allocated to playback.
/// \returns The number of allocated physical voices.
/// \note Original decompilation identifier: \c func_800202D4 (0x800202D4).
s32 audioCountAllocatedVoices(void) {
    ALLink *node;
    s32 count = 0;
    OSIntMask mask;
    mask = osSetIntMask(OS_IM_NONE);
    node = &gGameAudioSynthesizer.drvr.pAllocList;
    if (gGameAudioSynthesizer.drvr.pAllocList.next != NULL) {
        do {
            node = node->next;
            count++;
        } while (node->next != NULL);
    }
    osSetIntMask(mask);
    return count;
}
