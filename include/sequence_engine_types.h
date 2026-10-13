/// \file
/// Defines registered sequence channel controls and borrowed event-reader state.

#ifndef SEQUENCE_ENGINE_TYPES_H
#define SEQUENCE_ENGINE_TYPES_H

#include "sequence_driver_types.h"

/// Per-channel controller storage owned by a registered sequence.
typedef struct EngineChannel {
    ALInstrument *instrument; ///< Borrowed current bank instrument.
    s16 pitchBend;            ///< Retained fourteen-bit bend value, centered at 0x2000.
    u16 pitchBendRange;       ///< Pitch bend sensitivity in 1/128-semitone units.
    u16 pan;                  ///< Fourteen-bit stereo pan control.
    u8 unknown0A;             ///< Unresolved controller byte.
    u8 volume;                ///< Seven-bit channel volume.
    u8 fxMix;                 ///< Seven-bit synthesizer effect mix.
    u8 sustain;               ///< Sustain pedal; values at least 64 hold note releases.
    u16 expression;           ///< Fourteen-bit expression level.
    u16 bank;                 ///< Fourteen-bit bank selection.
    u8 rpnHigh, rpnLow;       ///< Registered parameter selector bytes.
    u8 lastKey;               ///< Retained note used by portamento.
    u8 portamentoEnabled;     ///< Nonzero enables a previous-note pitch transition.
    u8 portamentoTime;        ///< Controller scaling the transition duration.
    u8 vibratoDepth;          ///< Pitch-modulation amount.
    u8 tremoloDepth;          ///< Volume-modulation amount.
    u8 unknown019[3];         ///< Unresolved channel tail.
} EngineChannel;

/// Callback and channel state for one registered sequence.
/// Event storage and reader context are borrowed until the sequence is removed.
typedef struct EngineSequence {
    struct EngineSequence *next;           ///< Next registered sequence in the driver list.
    s32 readerContext;                     ///< Borrowed event-reader context represented by its target address.
    s8 (*eventReader)(s32, s32, u8, u8 *); ///< Legacy event callback view used by the engine.
    ALBank *bank;                          ///< Borrowed instrument bank for channel programs.
    SequenceVoice *voices;                 ///< Active voice list drawn from the driver-owned pool.
    EngineChannel channels[16];            ///< Controller state for the sixteen MIDI channels.
    s16 volume;                            ///< Current master sequence volume.
    u8 unknown1D6[2];                      ///< Unresolved alignment storage.
    f64 fadeRemaining;                     ///< Seconds remaining in the current master-volume fade.
    f64 fadeRate;                          ///< Master-volume change per second during the fade.
    f64 fadeLevel;                         ///< Current normalized master-volume fade level.
    s8 eventLength;                        ///< Length of the pending event, or a negative end marker.
    u8 eventCapacity;                      ///< Capacity supplied for the borrowed event buffer.
    u8 unknown1F2[2];
    s32 eventBuffer; ///< Target address of caller-owned event storage.
    f64 eventDelay;  ///< Seconds remaining before the pending event is dispatched.
} EngineSequence;

typedef char EngineChannelPanOffsetCheck[(((u32) & ((EngineChannel *)0)->pan) == 8) ? 1 : -1];
typedef char EngineChannelEffectsOffsetCheck[(((u32) & ((EngineChannel *)0)->fxMix) == 0xC) ? 1 : -1];
typedef char EngineChannelVibratoOffsetCheck[(((u32) & ((EngineChannel *)0)->vibratoDepth) == 0x17) ? 1 : -1];
typedef char EngineChannelSizeCheck[(sizeof(EngineChannel) == 0x1C) ? 1 : -1];
typedef char EngineSequenceSizeCheck[(sizeof(EngineSequence) == 0x200) ? 1 : -1];
typedef char EngineSequenceVoicesOffsetCheck[((u32) & ((EngineSequence *)0)->voices == 0x10) ? 1 : -1];
typedef char EngineSequenceEventOffsetCheck[((u32) & ((EngineSequence *)0)->eventLength == 0x1F0) ? 1 : -1];
typedef char EngineSequenceFadeTimeOffsetCheck[((u32) & ((EngineSequence *)0)->fadeRemaining == 0x1D8) ? 1 : -1];
typedef char EngineSequenceFadeRateOffsetCheck[((u32) & ((EngineSequence *)0)->fadeRate == 0x1E0) ? 1 : -1];
typedef char EngineSequenceFadeLevelOffsetCheck[((u32) & ((EngineSequence *)0)->fadeLevel == 0x1E8) ? 1 : -1];

#endif
