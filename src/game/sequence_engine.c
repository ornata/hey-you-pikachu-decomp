#include "sequence_registration.h"
#include "sequence_engine_types.h"
#include "sequence_playback.h"
#include "midi_sequence.h"
/* Voice and sequence engine; split from the control API at ROM 6750. */
#include "common.h"
#include "sequence_driver.h"

typedef char EngineDriverVoicesOffsetCheck[((u32) & ((VoicePlayer *)0)->voices == 0x20) ? 1 : -1];

struct N_ALVoice_s;
void n_alSynSetVol(struct N_ALVoice_s *, s16, ALMicroTime);

void sequenceEngineReleaseVoice(VoicePlayer *, EngineSequence *, SequenceVoice *);
void sequenceEngineReadNextEvent(EngineSequence *);
void sequenceEngineDispatchMidiEvent(VoicePlayer *, EngineSequence *);
s32 sequenceEngineRecycleVoice(VoicePlayer *, EngineSequence *, SequenceVoice *);
void n_alSynFreeVoice(struct N_ALVoice_s *);
void n_alSynStopVoice(struct N_ALVoice_s *);

s32 sequenceEngineSetSustain(VoicePlayer *, EngineSequence *, s32, s32);
s32 sequenceEngineGetVoicePan(VoicePlayer *, EngineSequence *, SequenceVoice *);
s32 sequenceEngineResetControllers(VoicePlayer *, EngineSequence *);
void sequenceEngineStopVoice(s32, s32, s32);
f64 sequenceEngineGetVoiceUpdateInterval(N_ALGlobals *);
void sequenceEngineUpdateVoices(VoicePlayer *, EngineSequence *, f64);
void sequenceEngineAdvanceVolumeFade(EngineSequence *, f64);
void sequenceEngineAdvanceEvents(VoicePlayer *, EngineSequence *, f64);


/// Check whether sequence playback and pending release events have ended.
/// \param player Sequence player to inspect.
/// \returns One when playback and pending releases have ended; otherwise zero.
/// \note Original decompilation identifier: \c func_80005B50 (0x80005B50).
s32 sequenceEngineIsDrained(SequenceState *player) {
    return midiSequenceIsDrained(player);
}

/// Check whether sequence playback is stopped.
/// \param player Sequence player to inspect.
/// \returns Nonzero when stopped, even if release events remain; otherwise zero.
/// \note Original decompilation identifier: \c func_80005B70 (0x80005B70).
s32 sequenceEngineIsStopped(SequenceState *player) {
    return midiSequenceIsStopped(player);
}

/// Set the sequence playback speed multiplier.
/// \param player Sequence player to update.
/// \param rate Positive playback multiplier; one preserves the original tempo.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005B90 (0x80005B90).
s32 sequenceEngineSetRate(SequenceState *player, f64 rate) {
    return midiSequenceSetRate(player, rate);
}


/// Get the sequence playback speed multiplier.
/// \param player Sequence player to inspect.
/// \returns The configured playback multiplier.
/// \note Original decompilation identifier: \c func_80005BC0 (0x80005BC0).
f64 sequenceEngineGetRate(SequenceState *player) {
    return midiSequenceGetRate(player);
}

/// Assign a free sequence voice to a sequence.
/// \param driver Sequence driver owning the free voice pool.
/// \param sequence Sequence receiving the voice.
/// \returns A borrowed voice owned by the driver and assigned to the sequence, or null when the free pool is empty.
/// \note Original decompilation identifier: \c func_80005BE0 (0x80005BE0).
SequenceVoice *sequenceEngineAcquireVoice(VoicePlayer *driver, EngineSequence *sequence) {
    SequenceVoice *node;
    SequenceVoice *head;

    node = driver->voices;
    if (node != NULL) {
        driver->voices = node->next;
        node->next = sequence->voices;
        node->previous = NULL;
        head = sequence->voices;
        if (head != NULL) {
            head->previous = node;
        }
        sequence->voices = node;
    }
    return node;
}

/// Return a sequence voice to the driver free pool, ending its assignment to the sequence.
/// \param driver Sequence driver owning the voice pool.
/// \param sequence Sequence currently owning the voice.
/// \param voice Voice belonging to the sequence.
/// \returns One.
/// \note Original decompilation identifier: \c func_80005C1C (0x80005C1C).
s32 sequenceEngineRecycleVoice(VoicePlayer *driver, EngineSequence *sequence, SequenceVoice *voice) {
    SequenceVoice *previousVoice;
    SequenceVoice *nextVoice;

    previousVoice = voice->previous;
    if (previousVoice != NULL) {
        previousVoice->next = voice->next;
    } else {
        sequence->voices = voice->next;
    }
    nextVoice = voice->next;
    if (nextVoice != NULL) {
        nextVoice->previous = voice->previous;
    }
    voice->next = driver->voices;
    driver->voices = voice;
    return 1;
}

/// Stop a sequence voice and release it for reuse.
/// \param driver Sequence driver owning the voice pool.
/// \param sequence Sequence currently owning the voice.
/// \param voiceRecord Voice belonging to the sequence.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80005C64 (0x80005C64).
void sequenceEngineStopVoice(s32 driver, s32 sequence, s32 voiceRecord) {
    struct N_ALVoice_s *voice = (struct N_ALVoice_s *)(voiceRecord + 0x14);
    n_alSynStopVoice(voice);
    n_alSynFreeVoice(voice);
    sequenceEngineRecycleVoice((VoicePlayer *)driver, (EngineSequence *)sequence, (SequenceVoice *)voiceRecord);
}

/// Release a sequence note according to its sound release setting.
/// \param driver Sequence driver owning the voice pool.
/// \param sequence Sequence currently owning the voice.
/// \param voice Voice belonging to the sequence.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80005CAC (0x80005CAC).
void sequenceEngineReleaseVoice(VoicePlayer *driver, EngineSequence *sequence, SequenceVoice *voice) {
    f64 releaseSeconds;
    s32 releaseVolumeRate;

    releaseSeconds = (f64)voice->sound->envelope->releaseTime * 0.000001;
    if (releaseSeconds > 0.0) {
        releaseVolumeRate = (s32)((f64) - voice->envelope / releaseSeconds);
        voice->envelopeRate = releaseVolumeRate;
        if (releaseVolumeRate == 0) {
            voice->envelopeRate = -1;
        }
        n_alSynSetVol(&voice->native, 0, (s32)(releaseSeconds * 1000000.0));
        return;
    }
    sequenceEngineStopVoice((s32)driver, (s32)sequence, (s32)voice);
}

/// Set channel sustain, releasing all sustain-held notes in the sequence when that channel lifts sustain.
/// \param driver Sequence driver owning the voice pool.
/// \param sequence Sequence whose sustain state is changed.
/// \param channelIndex MIDI channel index from zero through fifteen.
/// \param sustain MIDI sustain value from zero through 127; values below 64 release sustain.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005D6C (0x80005D6C).
s32 sequenceEngineSetSustain(VoicePlayer *driver, EngineSequence *sequence, s32 channelIndex, s32 sustain) {
    SequenceVoice *voice;

    if ((sequence->channels[channelIndex].sustain >= 0x40) && (sustain < 0x40)) {
        voice = sequence->voices;
        if (voice != NULL) {
            do {
                if (voice->sustainDeferred != 0) {
                    sequenceEngineReleaseVoice(driver, sequence, voice);
                }
                voice = voice->next;
            } while (voice != NULL);
        }
    }
    sequence->channels[channelIndex].sustain = sustain;
    return 0;
}


/// Determine the effective stereo pan for a sequence voice.
/// \param driver Sequence driver supplying the output mode.
/// \param sequence Sequence supplying channel pan settings.
/// \param voice Sequence voice whose pan is evaluated.
/// \returns The pan value from zero through 127, or 64 in mono mode.
/// \note Original decompilation identifier: \c func_80005E14 (0x80005E14).
s32 sequenceEngineGetVoicePan(VoicePlayer *driver, EngineSequence *sequence, SequenceVoice *voice) {
    s32 pan;

    if (driver->outputMode == 1) {
        return 0x40;
    }
    pan = voice->sound->samplePan + (sequence->channels[voice->channel].pan >> 7) - 0x40;
    if (pan < 0) {
        pan = 0;
    }
    if (pan >= 0x80) {
        pan = 0x7F;
    }
    return pan;
}

/// Apply the effective stereo pan to a sequence voice.
/// \param driver Sequence driver supplying the output mode.
/// \param sequence Sequence supplying channel pan settings.
/// \param voice Sequence voice whose pan is evaluated.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005E80 (0x80005E80).
s32 sequenceEngineUpdateVoicePan(VoicePlayer *driver, EngineSequence *sequence, SequenceVoice *voice) {
    s32 pan;

    pan = sequenceEngineGetVoicePan(driver, sequence, voice);
    if (pan != voice->pan) {
        voice->pan = pan;
        n_alSynSetPan(&voice->native, pan & 0xFF, voice);
    }
    return 0;
}

/// Restore the channel controller defaults and release sustain.
/// \param driver Sequence driver owning the voice pool.
/// \param sequence Sequence whose channel is reset.
/// \param channelIndex MIDI channel index from zero through fifteen.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005EC4 (0x80005EC4).
s32 sequenceEngineResetChannelControllers(VoicePlayer *driver, EngineSequence *sequence, s32 channelIndex) {
    EngineChannel *channel;

    sequenceEngineSetSustain(driver, sequence, channelIndex, 0);
    channel = &sequence->channels[channelIndex];
    channel->pitchBend = 0x2000;
    channel->portamentoEnabled = 0;
    channel->vibratoDepth = 0;
    channel->tremoloDepth = 0;
    channel->rpnHigh = 0x7F;
    channel->rpnLow = 0x7F;
    channel->expression = 0x3F80;
    return 0;
}

struct N_ALVoice_s;
void n_alSynSetFXMix(struct N_ALVoice_s *, u8);
/// Restore sequence and channel playback defaults and refresh active voice pan and effects.
/// \param driver Sequence driver owning the voice pool.
/// \param seq Sequence to reset.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005F30 (0x80005F30).
s32 sequenceEngineResetControllers(VoicePlayer *driver, EngineSequence *seq) {
    s32 i;
    u8 *channel;
    SequenceVoice *voice;
    seq->volume = 0x4000;
    channel = (u8 *)seq;
    i = 0;
    seq->fadeRate = 0.0;
    seq->fadeLevel = 1.0;
    do {
        sequenceEngineResetChannelControllers(driver, seq, i);
        *(s16 *)(channel + 0x24) = 0;
        *(ALInstrument **)(channel + 0x14) = seq->bank->percussion;
        i++;
        channel += 0x1C;
        channel[4] = 0;
        channel[3] = 0x64;
        channel[2] = 5;
        *(s16 *)(channel - 2) = 0x100;
        channel[0xE] = 0;
        channel[0xC] = 0x40;
        *(s16 *)channel = 0x2000;
    } while (i != 16);
    voice = seq->voices;
    if (voice != NULL) {
        do {
            sequenceEngineUpdateVoicePan(driver, seq, voice);
            n_alSynSetFXMix(&voice->native, seq->channels[voice->channel].fxMix);
            voice = voice->next;
        } while (voice != NULL);
    }
    return 0;
}

/// Load the next sequence event and its playback delay.
/// \param sequence Sequence whose event reader is advanced.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80006068 (0x80006068).
void sequenceEngineReadNextEvent(EngineSequence *sequence) {
    sequence->eventLength =
        ((EngineSequence *)sequence)
            ->eventReader(sequence->readerContext, sequence->eventBuffer,
                          sequence->eventCapacity, &sequence->eventDelay);
}

/// Initialize a sequence and register it for playback by the driver.
/// \param sequence Caller-owned state of at least 512 bytes, not already registered, which must remain valid while registered.
/// \param driver Initialized sequence driver receiving the sequence.
/// \param instrumentBank Borrowed instrument bank with a valid default instrument, retained while the sequence and its voices use it.
/// \param eventCapacity Event buffer capacity in bytes, representable as an unsigned byte.
/// \param eventBuffer Address of caller-owned storage of at least eventCapacity bytes, valid while the sequence is registered.
/// \param readerContext Borrowed context passed to the event reader, valid while the sequence is registered.
/// \param eventReader Callback receiving the context, buffer, byte capacity, and output delay in seconds; it must emit complete supported MIDI messages within capacity and return their byte length.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800060A8 (0x800060A8).
void sequenceEngineRegisterSequence(EngineSequence *sequence, VoicePlayer *driver, ALBank *instrumentBank, s32 eventCapacity, u8 *eventBuffer, SequenceControlState *readerContext, SequenceEventReader eventReader) {
    s32 i;

    sequence->next = driver->sequences;
    sequence->bank = instrumentBank;
    sequence->voices = NULL;
    sequence->readerContext = readerContext;
    sequence->eventReader = eventReader;
    sequence->eventCapacity = eventCapacity;
    sequence->eventBuffer = eventBuffer;
    for (i = 0; i < 16; i++) {
        sequence->channels[i].sustain = 0;
    }
    sequenceEngineResetControllers(driver, sequence);
    sequenceEngineReadNextEvent(sequence);
    driver->sequences = sequence;
}


/// Select the channel instrument sound matching a note and velocity.
/// \param seq Sequence with a valid channel instrument whose sound definitions are ordered by note and velocity range.
/// \param channel MIDI channel index from zero through fifteen.
/// \param key MIDI note number from zero through 127.
/// \param velocity MIDI note-on velocity from zero through 127.
/// \returns A borrowed sound definition from the instrument bank, or null when no sound covers the note and velocity.
/// \note Original decompilation identifier: \c func_80006140 (0x80006140).
ALSound *sequenceEngineFindSound(EngineSequence *seq, u8 channel, u8 key, u8 velocity) {
    EngineChannel *channels = seq->channels;
    s32 low = 0;
    s32 high = channels[channel].instrument->soundCount - 1;
    ALSound **sounds = channels[channel].instrument->soundArray;
    s32 middle;
    ALKeyMap *map;

    while (low <= high) {
        middle = (low + high) / 2;
        map = sounds[middle]->keyMap;
        if (key >= map->keyMin && key <= map->keyMax && velocity >= map->velocityMin && velocity <= map->velocityMax) {
            return sounds[middle];
        } else if (key < map->keyMin || (velocity < map->velocityMin && key <= map->keyMax)) {
            high = middle - 1;
        } else {
            low = middle + 1;
        }
    }
    return NULL;
}

/// Sixteen-sample unipolar waveform used for sequence-voice volume modulation.
/// \note Original identifier: \c D_800635E0 (0x800635E0).
extern s32 gSequenceTremoloWaveform[];

/// Determine the effective volume of a sequence voice.
/// \param driver Sequence driver supplying the master volume.
/// \param seq Sequence supplying channel and playback volume settings.
/// \param voice Active sequence voice to evaluate.
/// \returns The effective synthesizer volume, capped at 32767.
/// \note Original decompilation identifier: \c func_80006254 (0x80006254).
s32 sequenceEngineGetVoiceVolume(u8 *driver, u8 *seq, u8 *voice) {
    u8 *channel;
    s32 tremolo;
    u32 volume;

    channel = seq;
    channel += ((SequenceVoice *)voice)->channel * 0x1C;
    if (channel[0x2C] != 0) {
        tremolo = 256 - (s32)((u32)(*(u8 **)(voice + 0xC))[6] *
                              (gSequenceTremoloWaveform[*(u16 *)(voice + 0x3A) >> 12] * channel[0x2C])) /
                            256 / 128;
    } else {
        tremolo = 256;
    }
    volume = (s32)(((SequenceVoice *)voice)->velocity * (32767 * (u32)driver[0x24])) / 128 / 128;
    volume = ((((SequenceVoice *)voice)->sound)->sampleVolume * (channel[0x1F] * volume)) / 128 / 128;
    volume = (tremolo * volume) / 256;
    volume = (*(u16 *)(channel + 0x22) * volume) / 16384;
    volume *= *(f64 *)(seq + 0x1E8);
    volume = (*(u16 *)(seq + 0x1D4) * volume) / 16384;
    volume = ((*(u32 *)(voice + 0x48) & 0xF000) * volume) / 65536;
    if (volume >= 32768) {
        volume = 32767;
    }
    return volume;
}

/// Sixteen-sample bipolar waveform used for sequence-voice pitch modulation.
/// \note Original identifier: \c D_80063620 (0x80063620).
extern s32 gSequenceVibratoWaveform[];

/// Determine the effective playback pitch ratio of a sequence voice.
/// \param seq Sequence supplying pitch and modulation settings.
/// \param voice Active sequence voice to evaluate.
/// \returns The playback pitch multiplier relative to the sound sample.
/// \note Matching: byte views of the voice key and portamento retain the original IDO expression scheduling.
/// \note Original decompilation identifier: \c func_8000645C (0x8000645C).
f64 sequenceEngineGetVoicePitchRatio(u8 *seq, u8 *voice) {
    u8 *channel;
    s32 vibrato;
    s32 depth;
    ALKeyMap *keymap;

    channel = seq;
    channel += voice[0x30] * 0x1C + 0x14;
    depth = channel[0x17];
    if (depth != 0) {
        vibrato =
            (depth * (*(u8 **)(voice + 0xC))[0xA]) * gSequenceVibratoWaveform[*(u16 *)(voice + 0x38) >> 12] / 128 / 128;
    } else {
        vibrato = 0;
    }
    keymap = (*(ALSound **)(voice + 0x10))->keyMap;
    return alCents2Ratio(((*(u16 *)(channel + 4) - 0x2000) * *(u16 *)(channel + 6) / 8192) * 100 / 128 +
                         *(s16 *)(voice + 0x3C) + ((voice[0x31] - keymap->keyBase) * 100 - keymap->detune) + vibrato);
}

/// Get the sequence voice update interval.
/// \param synth Unused synthesizer context retained by the timing interface.
/// \returns The fixed update interval of 0.00575 seconds.
/// \note Original decompilation identifier: \c func_8000657C (0x8000657C).
f64 sequenceEngineGetVoiceUpdateInterval(N_ALGlobals *synth) {
    return 0.00575;
}

ALSound *sequenceEngineFindSound(EngineSequence *, u8, u8, u8);
s32 sequenceEngineGetVoiceVolume(u8 *, u8 *, u8 *);
f64 sequenceEngineGetVoicePitchRatio(u8 *, u8 *);
s32 n_alSynAllocVoice(struct N_ALVoice_s *, ALVoiceConfig *);
void n_alSynStartVoiceParams(struct N_ALVoice_s *, ALWaveTable *, f32, s16, ALPan, u8, ALMicroTime);

/// Start a sequence note when a matching sound and voice are available.
/// \param driver Sequence driver owning the voice pool.
/// \param seq Sequence receiving the note.
/// \param channel MIDI channel index from zero through fifteen.
/// \param key MIDI note number from zero through 127.
/// \param velocity MIDI note-on velocity from zero through 127.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000658C (0x8000658C).
void sequenceEngineNoteOn(VoicePlayer *driver, EngineSequence *seq, s32 channel, s32 key, s32 velocity) {
    ALSound *sound;
    SequenceVoice *voice;
    ALVoiceConfig config;

    sound = sequenceEngineFindSound(seq, channel, key, velocity);
    if (sound != NULL) {
        voice = sequenceEngineAcquireVoice(driver, seq);
        if (voice != NULL) {
            config.priority = 5;
            config.fxBus = 0;
            config.unityPitch = 0;
            voice->sustainDeferred = 0;
            voice->keyDown = 1;
            voice->sound = sound;
            voice->envelope = 0xFFFF;
            voice->envelopeRate =
                ((voice->sound->envelope->decayTime != 0) &&
                 (voice->sound->envelope->decayVolume == 0))
                    ? (f64) - voice->envelope / (0.000001 * voice->sound->envelope->decayTime)
                    : 0.0;
            voice->instrument = seq->channels[channel].instrument;
            voice->key = key;
            voice->channel = channel;
            voice->velocity = velocity;
            voice->vibratoPhase = 0;
            voice->tremoloPhase = 0;
            voice->volume = sequenceEngineGetVoiceVolume((u8 *)driver, (u8 *)seq, (u8 *)voice);
            if (seq->channels[channel].portamentoEnabled) {
                voice->portamento = (seq->channels[channel].lastKey - voice->key) * 100;
            } else {
                voice->portamento = 0;
            }
            seq->channels[channel].lastKey = key;
            voice->pitch = sequenceEngineGetVoicePitchRatio((u8 *)seq, (u8 *)voice);
            voice->pan = sequenceEngineGetVoicePan(driver, seq, voice);
            n_alSynAllocVoice(&voice->native, &config);
            n_alSynStartVoiceParams(&voice->native, voice->sound->wavetable,
                                    voice->pitch, voice->volume, voice->pan,
                                    seq->channels[voice->channel].fxMix, 0);
            if (voice->envelopeRate != 0) {
                n_alSynSetVol(&voice->native, 0, voice->sound->envelope->decayTime);
            }
        }
    }
}

/// Provide the sequence-engine callback that requires no action.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800067B4 (0x800067B4).
void sequenceEngineNoOp(void) {
}

/// Release matching sequence notes, honoring channel sustain.
/// \param driver Sequence driver owning the voice pool.
/// \param sequence Sequence receiving the note-off.
/// \param channel MIDI channel index from zero through fifteen.
/// \param key MIDI note number to release.
/// \param velocity Unused note-release velocity.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800067BC (0x800067BC).
void sequenceEngineNoteOff(VoicePlayer *driver, EngineSequence *sequence, s32 channel, s32 key, s32 velocity) {
    SequenceVoice *voice;
    voice = sequence->voices;
    if (voice != NULL) {
        do {
            if ((key == voice->key) && (channel == voice->channel) && (voice->keyDown != 0)) {
                voice->keyDown = 0;
                if (sequence->channels[channel].sustain >= 0x40) {
                    voice->sustainDeferred = 1;
                } else {
                    sequenceEngineReleaseVoice(driver, sequence, voice);
                }
            }
            voice = voice->next;
        } while (voice != NULL);
    }
}

/// Select the instrument program for a sequence channel.
/// \param sequence Sequence containing the instrument bank.
/// \param channel MIDI channel index from zero through fifteen.
/// \param program Nonnegative instrument program index; values beyond the bank are ignored.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80006890 (0x80006890).
void sequenceEngineSetProgram(EngineSequence *sequence, s32 channel, s32 program) {
    ALBank *bank;

    bank = sequence->bank;
    if (program < bank->instCount) {
        sequence->channels[channel].instrument = bank->instArray[program];
    }
}

/// Set the pitch bend for a sequence channel.
/// \param sequence Sequence receiving the pitch bend.
/// \param channel MIDI channel index from zero through fifteen.
/// \param lowBits Low seven bits of the MIDI pitch-bend value.
/// \param highBits High seven bits of the MIDI pitch-bend value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800068CC (0x800068CC).
void sequenceEngineSetPitchBend(EngineSequence *sequence, s32 channel, s32 lowBits, s32 highBits) {
    sequence->channels[channel].pitchBend = (s16)((highBits << 7) | lowBits);
}

/// Apply MIDI data entry to the selected supported channel parameter.
/// \param channel Channel controller state receiving the data entry.
/// \param isLowPart Nonzero for the fine value; zero for the coarse value.
/// \param value Seven-bit MIDI data-entry value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800068EC (0x800068EC).
void sequenceEngineApplyDataEntry(u8 *channel, s32 isLowPart, s32 value) {
    if ((channel[0x13] | (channel[0x12] << 7)) == 0) {
        if (isLowPart != 0) {
            *(u16 *)(channel + 6) = (*(u16 *)(channel + 6) & 0x3F80) | value;
            return;
        }
        *(u16 *)(channel + 6) = value << 7;
    }
}

/// Apply a MIDI controller change to a sequence channel.
/// \param driver Sequence driver owning the active voices.
/// \param seq Sequence receiving the controller change.
/// \param channel MIDI channel index from zero through fifteen.
/// \param controller MIDI controller number.
/// \param value Seven-bit controller value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000692C (0x8000692C).
void sequenceEngineControlChange(VoicePlayer *driver, EngineSequence *seq, s32 channel, s32 controller, s32 value) {
    SequenceVoice *voice;
    SequenceVoice *current;
    switch (controller) {
    case 0:
        seq->channels[channel].bank = value << 7;
        break;
    case 32:
        seq->channels[channel].bank =
            (seq->channels[channel].bank & 0x3F80) | value;
        break;
    case 1:
        seq->channels[channel].vibratoDepth = value;
        seq->channels[channel].tremoloDepth = value;
        break;
    case 5:
        seq->channels[channel].portamentoTime = value;
        break;
    case 6:
        sequenceEngineApplyDataEntry((u8 *)&seq->channels[channel], 0, value);
        break;
    case 38:
        sequenceEngineApplyDataEntry((u8 *)&seq->channels[channel], 1, value);
        break;
    case 7:
        seq->channels[channel].volume = value;
        break;
    case 10:
        seq->channels[channel].pan = value << 7;
        for (voice = seq->voices; voice != NULL; voice = voice->next) {
            if (channel == voice->channel) {
                sequenceEngineUpdateVoicePan(driver, seq, voice);
            }
        }
        break;
    case 42:
        seq->channels[channel].pan =
            (seq->channels[channel].pan & 0x3F80) | value;
        for (voice = seq->voices; voice != NULL; voice = voice->next) {
            if (channel == voice->channel) {
                sequenceEngineUpdateVoicePan(driver, seq, voice);
            }
        }
        break;
    case 11:
        seq->channels[channel].expression = value << 7;
        break;
    case 64:
        sequenceEngineSetSustain(driver, seq, channel, value);
        break;
    case 65:
        seq->channels[channel].portamentoEnabled = value;
        break;
    case 84:
        seq->channels[channel].lastKey = value;
        break;
    case 91:
        seq->channels[channel].fxMix = value;
        for (voice = seq->voices; voice != NULL; voice = voice->next) {
            if (channel == voice->channel) {
                n_alSynSetFXMix(&voice->native,
                                seq->channels[voice->channel].fxMix);
            }
        }
        break;
    case 100:
        seq->channels[channel].rpnLow = value;
        break;
    case 101:
        seq->channels[channel].rpnHigh = value;
        break;
    case 120:
        for (current = seq->voices; current != NULL; current = voice) {
            voice = current->next;
            if (channel == current->channel) {
                sequenceEngineStopVoice((s32)driver, (s32)seq, (s32)current);
            }
        }
        break;
    case 121:
        sequenceEngineResetChannelControllers(driver, seq, channel);
        break;
    case 123:
        for (current = seq->voices; current != NULL; current = voice) {
            voice = current->next;
            if (channel == current->channel) {
                sequenceEngineReleaseVoice(driver, seq, current);
            }
        }
        break;
    }
}

/// Apply supported sequence volume, fade, and reset system events.
/// \param driver Sequence driver supplying controller defaults.
/// \param seq Sequence receiving the system event.
/// \param event Complete MIDI system-event message with seven-bit data bytes; supported system-exclusive messages require at least seven bytes.
/// \param length Event length supplied by the caller; unused.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80006CC0 (0x80006CC0).
void sequenceEngineHandleSystemEvent(VoicePlayer *driver, EngineSequence *seq, u8 *event, s32 length) {
    u8 type;
    f64 time;
    f64 level;
    type = event[0];
    switch (type) {
    case 0xF0:
        if (event[1] == 0x7F && event[3] == 4 && event[4] == 1) {
            seq->volume = (event[6] << 7) | event[5];
            return;
        }
        if (event[1] == 0x7F && (type = event[3]) == 4 && event[4] == 3) {
            time = (u32)event[5] * 0.0625;
            level = (u32)event[6] / 127.0;
            if (time > 0.0) {
                seq->fadeRemaining = time;
                seq->fadeRate = (level - seq->fadeLevel) / time;
                return;
            }
            seq->fadeLevel = level;
        }
        return;
    case 0xFF:
        sequenceEngineResetControllers(driver, seq);
        break;
    }
}

void sequenceEngineNoteOn(VoicePlayer *, EngineSequence *, s32, s32, s32);
/// Apply the current MIDI event to a sequence.
/// \param driver Sequence driver owning the active voices.
/// \param seq Sequence whose current event is a complete supported MIDI message with seven-bit data bytes, or an empty event.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80006E08 (0x80006E08).
void sequenceEngineDispatchMidiEvent(VoicePlayer *driver, EngineSequence *seq) {
    u8 *event;
    s32 status;
    u8 length;
    u8 velocity;
    length = (u8)seq->eventLength;
    if (length) {
        event = seq->eventBuffer;
        status = event[0];
        switch (status & 0xF0) {
        case 0x90:
            velocity = event[2];
            if (velocity != 0) {
                sequenceEngineNoteOn(driver, seq, status & 0xF, event[1], velocity);
                return;
            }
        case 0x80:
            sequenceEngineNoteOff(driver, seq, status & 0xF, event[1], event[2]);
            return;
        case 0xB0:
            sequenceEngineControlChange(driver, seq, status & 0xF, event[1], event[2]);
            return;
        case 0xC0:
            sequenceEngineSetProgram(seq, status & 0xF, event[1]);
            return;
        case 0xE0:
            sequenceEngineSetPitchBend(seq, status & 0xF, event[1], event[2]);
            return;
        case 0xF0:
            sequenceEngineHandleSystemEvent(driver, seq, event, length);
            break;
        }
    }
}

/// Advance the sequence volume fade.
/// \param sequence Sequence whose volume fade is advanced.
/// \param elapsed Nonnegative elapsed time in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80006F2C (0x80006F2C).
void sequenceEngineAdvanceVolumeFade(EngineSequence *sequence, f64 elapsed) {
    f64 rate;
    f64 left;
    f64 level;

    rate = sequence->fadeRate;
    if (sequence->fadeRate != 0.0) {
        left = sequence->fadeRemaining;
        if (left < elapsed) {
            sequence->fadeRate = 0.0;
            sequence->fadeLevel = sequence->fadeLevel + (left * rate);
        } else {
            sequence->fadeRemaining = left - elapsed;
            sequence->fadeLevel = sequence->fadeLevel + (elapsed * rate);
        }
        level = sequence->fadeLevel;
        if (level < 0.0) {
            sequence->fadeLevel = 0.0;
            level = 0.0;
        }
        if (level > 1.0) {
            sequence->fadeLevel = 1.0;
        }
    }
}

/// Advance sequence playback and process events that are due.
/// \param driver Sequence driver receiving the events.
/// \param sequence Sequence whose playback is advanced.
/// \param elapsed Nonnegative elapsed playback time in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80006FD4 (0x80006FD4).
void sequenceEngineAdvanceEvents(VoicePlayer *driver, EngineSequence *sequence, f64 elapsed) {
    f64 var_f0;
    f64 var_f20;

    var_f20 = elapsed;
    var_f0 = sequence->eventDelay;
    if (var_f0 <= var_f20) {
    loop_2:
        if (var_f0 >= 0.0) {
            var_f20 -= var_f0;
        }
        sequenceEngineDispatchMidiEvent(driver, sequence);
        sequenceEngineReadNextEvent(sequence);
        var_f0 = sequence->eventDelay;
        if (!(var_f0 < 0.0) && (var_f0 <= var_f20)) {
            goto loop_2;
        }
    }
    if (var_f0 >= 0.0) {
        sequence->eventDelay = var_f0 - var_f20;
    }
}

#include "n_synthInternals.h"

void n_alSynSetPitch(struct N_ALVoice_s *, f32);

/// Advance sequence voice envelopes and modulation and refresh their playback settings.
/// \param driver Sequence driver owning the active voices.
/// \param seq Sequence whose voices are updated.
/// \param elapsed Nonnegative elapsed time in seconds.
/// \returns Nothing.
///
/// \note NONPORTABLE: empty body of a compiled-out check; the ROM keeps its read of the rate, which
/// the envelope update below then reuses.
///
/// \note UB: rate * step overflows s32 for short decays; IDO wraps (UB.md).
/// \note Original decompilation identifier: \c func_800070A4 (0x800070A4).
void sequenceEngineUpdateVoices(VoicePlayer *driver, EngineSequence *seq, f64 elapsed) {
    SequenceVoice *voice;
    s32 step;
    s32 vibRate;
    s32 tremRate;
    s32 volume;
    f64 pitch;

    step = elapsed * 65536.0;
    voice = seq->voices;
    while (voice != NULL) {


        if (voice->envelopeRate != 0) {
        }

        voice->envelope += voice->envelopeRate * step / 65536;
        if (voice->native.active == 0 || voice->envelope <= 0x2000) {
            sequenceEngineStopVoice((s32)driver, (s32)seq, (s32)voice);
            return;
        }
        if (seq->channels[voice->channel].vibratoDepth != 0) {
            vibRate = voice->instrument->vibRate;
            voice->vibratoPhase += step * vibRate / 16;
        }
        if (seq->channels[voice->channel].tremoloDepth != 0) {
            tremRate = voice->instrument->tremRate;
            voice->tremoloPhase += step * tremRate / 16;
        }
        if (voice->portamento > 0) {
            voice->portamento -= 768 / (seq->channels[voice->channel].portamentoTime + 1);
            if (voice->portamento < 0) {
                voice->portamento = 0;
            }
        }
        if (voice->portamento < 0) {
            voice->portamento += 768 / (seq->channels[voice->channel].portamentoTime + 1);
            if (voice->portamento > 0) {
                voice->portamento = 0;
            }
        }
        if (voice->envelopeRate == 0) {
            volume = sequenceEngineGetVoiceVolume((u8 *)driver, (u8 *)seq, (u8 *)voice);
            if (volume != (u16)voice->volume) {
                voice->volume = volume;
                n_alSynSetVol(&voice->native, voice->volume, (s32)(elapsed * 1000000.0));
            }
        }
        pitch = sequenceEngineGetVoicePitchRatio((u8 *)seq, (u8 *)voice);
        if (pitch != voice->pitch) {
            voice->pitch = pitch;
            n_alSynSetPitch((struct N_ALVoice_s *)(&voice->native), pitch);
        }
        voice = voice->next;
    }
}

/// Advance every registered sequence for one player update.
/// \param context Sequence driver registered with the synthesizer.
/// \returns Always -1, the player callback result.
/// \note Original decompilation identifier: \c func_80007390 (0x80007390).
s32 sequenceEnginePlayerCallback(void *context) {
    VoicePlayer *driver = context;
    EngineSequence *sequence;
    f64 updateInterval;

    updateInterval = sequenceEngineGetVoiceUpdateInterval(driver->synth);
    sequence = driver->sequences;
    if (sequence != NULL) {
        do {
            sequenceEngineAdvanceEvents(driver, sequence, updateInterval);
            sequenceEngineAdvanceVolumeFade(sequence, updateInterval);
            sequenceEngineUpdateVoices(driver, sequence, updateInterval);
            sequence = sequence->next;
        } while (sequence != NULL);
    }
    return -1;
}

void n_alSynAddPlayer(ALPlayer *);

/// Initialize and register a sequence driver with its voice pool.
/// \param p Caller-owned driver state that must remain valid while registered with the synthesizer.
/// \param synth Synthesizer context that must remain valid while the driver is registered.
/// \param outputMode Output mode; one selects mono panning.
/// \param maxVoices Number of records in the supplied voice pool, from one through 255.
/// \param voices Caller-owned pool with at least maxVoices records, valid while the driver is registered.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007420 (0x80007420).
void sequenceEngineInitializeDriver(VoicePlayer *p, N_ALGlobals *synth, s32 outputMode, s32 maxVoices, SequenceVoice *voices) {
    s32 i;

    p->synth = synth;
    p->sequences = NULL;
    p->outputMode = outputMode;
    p->masterVolume = 0x80;
    p->maxVoices = maxVoices;
    p->voices = voices;
    for (i = 0; i < maxVoices - 1; i++) {
        p->voices[i].next = &p->voices[i + 1];
    }
    p->voices[i].next = NULL;
    p->node.next = NULL;
    p->node.handler = sequenceEnginePlayerCallback;
    p->node.clientData = p;
    n_alSynAddPlayer(&p->node);
}

/// Set the master volume for the sequence driver.
/// \param driver Sequence driver to update.
/// \param volume Normalized master volume from zero through one.
/// \returns Always one.
/// \note Original decompilation identifier: \c func_80007538 (0x80007538).
s32 sequenceEngineSetMasterVolume(VoicePlayer *driver, f64 volume) {
    driver->masterVolume = (u32)(volume * 255.0);
    return 1;
}

/// Set the sequence driver output mode.
/// \param driver Sequence driver to update.
/// \param outputMode Output mode; one selects mono panning.
/// \returns Always one.
/// \note Original decompilation identifier: \c func_800075E0 (0x800075E0).
s32 sequenceEngineSetOutputMode(VoicePlayer *driver, s32 outputMode) {
    driver->outputMode = outputMode;
    return 1;
}

/// Provide the sequence-engine entry point that returns zero.
/// \returns Always zero.
/// \note Original decompilation identifier: \c func_800075EC (0x800075EC).
s32 sequenceEngineReturnZero(void) {
    return 0;
}
