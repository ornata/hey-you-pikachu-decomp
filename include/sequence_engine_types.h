/// \file
/// Defines registered sequence channel controls and borrowed event-reader state.

#ifndef SEQUENCE_ENGINE_TYPES_H
#define SEQUENCE_ENGINE_TYPES_H

#include "sequence_driver_types.h"
#include "midi_sequence_types.h"
#include "midi_encoding.h"

/// Read one event from the registered sequence control state.
/// \param state Borrowed playback and queued-control context to advance.
/// \param event Caller-owned destination meeting the selected reader's storage contract.
/// \param eventLimit Reader-specific data limit; it may exclude a leading status byte.
/// \param delta Receives the event delay in seconds, or -1 when releases have ended.
/// \returns Complete event length from one through 255, or zero when no event remains.
/// \pre Nonterminal delays must be finite and nonnegative; a negative delay denotes completion.
/// Successive zero-delay events must eventually advance time or complete.
/// \note Portability: event lengths above 127 depend on target signed-byte conversion.
typedef s32 (*SequenceEventReader)(SequenceControlState *state, u8 *event, s32 eventLimit, f64 *delta);

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
/// Event storage, callback and reader context remain borrowed while the driver references the sequence.
typedef struct EngineSequence {
    struct EngineSequence *next;                ///< Next registered sequence in the driver list.
    SequenceControlState *readerContext;        ///< Borrowed playback and queued-control context.
    SequenceEventReader eventReader;            ///< Event producer advancing the borrowed context.
    ALBank *bank;                               ///< Borrowed instrument bank for channel programs.
    SequenceVoice *voices;                      ///< Active voice list drawn from the caller-supplied pool.
    EngineChannel channels[MIDI_CHANNEL_COUNT]; ///< Controller state for the sixteen MIDI channels.
    s16 volume;                                 ///< Current master sequence volume.
    u8 unknown1D6[2];                           ///< Unresolved alignment storage.
    f64 fadeRemaining;                          ///< Seconds remaining in the current master-volume fade.
    f64 fadeRate;                               ///< Master-volume change per second during the fade.
    f64 fadeLevel;                              ///< Current normalized master-volume fade level.
    s8 eventLength; ///< Pending event byte length; zero means no event, and completion is carried by eventDelay.
    u8 eventLimit;  ///< Reader-specific event limit; not an allocation-size guarantee.
    u8 unknown1F2[2];
    u8 *eventBuffer; ///< Caller-owned event storage.
    f64 eventDelay;  ///< Seconds remaining before the pending event is dispatched.
} EngineSequence;

typedef char EngineChannelPanOffsetCheck[(((u32) & ((EngineChannel *)0)->pan) == 8) ? 1 : -1];
typedef char EngineChannelBendRangeOffsetCheck[(((u32) & ((EngineChannel *)0)->pitchBendRange) == 6) ? 1 : -1];
typedef char EngineChannelRpnHighOffsetCheck[(((u32) & ((EngineChannel *)0)->rpnHigh) == 0x12) ? 1 : -1];
typedef char EngineChannelRpnLowOffsetCheck[(((u32) & ((EngineChannel *)0)->rpnLow) == 0x13) ? 1 : -1];
typedef char EngineChannelEffectsOffsetCheck[(((u32) & ((EngineChannel *)0)->fxMix) == 0xC) ? 1 : -1];
typedef char EngineChannelVibratoOffsetCheck[(((u32) & ((EngineChannel *)0)->vibratoDepth) == 0x17) ? 1 : -1];
typedef char EngineChannelSizeCheck[(sizeof(EngineChannel) == 0x1C) ? 1 : -1];
typedef char EngineSequenceSizeCheck[(sizeof(EngineSequence) == 0x200) ? 1 : -1];
typedef char EngineSequenceVoicesOffsetCheck[((u32) & ((EngineSequence *)0)->voices == 0x10) ? 1 : -1];
typedef char EngineSequenceEventOffsetCheck[((u32) & ((EngineSequence *)0)->eventLength == 0x1F0) ? 1 : -1];
typedef char EngineSequenceFadeTimeOffsetCheck[((u32) & ((EngineSequence *)0)->fadeRemaining == 0x1D8) ? 1 : -1];
typedef char EngineSequenceFadeRateOffsetCheck[((u32) & ((EngineSequence *)0)->fadeRate == 0x1E0) ? 1 : -1];
typedef char EngineSequenceFadeLevelOffsetCheck[((u32) & ((EngineSequence *)0)->fadeLevel == 0x1E8) ? 1 : -1];

typedef char EngineSequenceEventLimitOffsetCheck[((u32) & ((EngineSequence *)0)->eventLimit == 0x1F1) ? 1 : -1];

typedef char EngineSequenceContextOffsetCheck[((u32) & ((EngineSequence *)0)->readerContext == 4) ? 1 : -1];
typedef char EngineSequenceReaderOffsetCheck[((u32) & ((EngineSequence *)0)->eventReader == 8) ? 1 : -1];
typedef char EngineSequenceBufferOffsetCheck[((u32) & ((EngineSequence *)0)->eventBuffer == 0x1F4) ? 1 : -1];
typedef char EngineSequenceDelayOffsetCheck[((u32) & ((EngineSequence *)0)->eventDelay == 0x1F8) ? 1 : -1];

#endif
