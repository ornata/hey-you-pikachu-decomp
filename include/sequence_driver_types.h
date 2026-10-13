/// \file
/// Defines sequence-driver registration and the caller-supplied voice pool.

#ifndef SEQUENCE_DRIVER_TYPES_H
#define SEQUENCE_DRIVER_TYPES_H

#include "n_synthInternals.h"

/// Driver-owned playback record linked through active and free voice lists.
/// Instrument and sound data are borrowed from the bank for the voice lifetime.
typedef struct SequenceVoice {
    struct SequenceVoice *next;     ///< Next active or free voice.
    struct SequenceVoice *previous; ///< Previous active voice, or null at the head.
    s32 keyDown;                    ///< Nonzero until the matching note-off is received.
    ALInstrument *instrument;       ///< Borrowed instrument supplying modulation settings.
    ALSound *sound;                 ///< Borrowed sound definition and sample data.
    N_ALVoice native;               ///< Synthesizer voice owned while active.
    u8 channel;                     ///< MIDI channel index.
    u8 key;                         ///< MIDI note number.
    u8 velocity;                    ///< Note-on velocity.
    u8 unknown033;                  ///< Unresolved byte.
    s16 volume;                     ///< Last synthesizer volume submitted for the voice.
    u8 pan;                         ///< Last stereo pan submitted for the voice.
    u8 sustainDeferred;             ///< Nonzero when a note release waits for sustain to end.
    u16 vibratoPhase, tremoloPhase; ///< Modulation waveform phases.
    s16 portamento;                 ///< Retained pitch transition offset in cents.
    u8 unknown03E[2];               ///< Unresolved padding or storage.
    f64 pitch;                      ///< Last playback pitch ratio submitted for the voice.
    s32 envelope;                   ///< Fixed-point amplitude envelope.
    s32 envelopeRate;               ///< Signed fixed-point envelope change per second.
} SequenceVoice;

typedef char SequenceVoiceKeyDownOffsetCheck[(((u32) & ((SequenceVoice *)0)->keyDown) == 8) ? 1 : -1];
typedef char SequenceVoiceNativeOffsetCheck[(((u32) & ((SequenceVoice *)0)->native) == 0x14) ? 1 : -1];
typedef char SequenceVoiceChannelOffsetCheck[(((u32) & ((SequenceVoice *)0)->channel) == 0x30) ? 1 : -1];
typedef char SequenceVoicePitchOffsetCheck[(((u32) & ((SequenceVoice *)0)->pitch) == 0x40) ? 1 : -1];

struct EngineSequence;

/// Synthesizer registration and shared sequence playback controls.
typedef struct VoicePlayer {
    ALPlayer node;                    ///< Synthesizer callback registration; valid while registered.
    N_ALGlobals *synth;               ///< Borrowed synthesizer context while registered.
    struct EngineSequence *sequences; ///< Borrowed registered-sequence list, or null.
    u8 outputMode;                    ///< Output mode; one selects mono panning.
    u8 maxVoices;                     ///< Number of records supplied in the voice pool.
    SequenceVoice *voices;            ///< First available voice in caller-supplied storage.
    u8 masterVolume;                  ///< Master sequence volume on a zero-to-255 scale.
} VoicePlayer;

typedef char SequenceVoiceSizeCheck[(sizeof(SequenceVoice) == 0x50) ? 1 : -1];
typedef char VoicePlayerSizeCheck[(sizeof(VoicePlayer) == 0x28) ? 1 : -1];
typedef char VoicePlayerSequencesOffsetCheck[((u32) & ((VoicePlayer *)0)->sequences == 0x18) ? 1 : -1];
typedef char VoicePlayerPoolOffsetCheck[((u32) & ((VoicePlayer *)0)->voices == 0x20) ? 1 : -1];

#endif
