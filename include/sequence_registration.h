/// \file
/// Declares registration of caller-owned sequence state and borrowed event storage.

#ifndef SEQUENCE_REGISTRATION_H
#define SEQUENCE_REGISTRATION_H

#include "sequence_engine_types.h"

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
void sequenceEngineRegisterSequence(EngineSequence *sequence, VoicePlayer *driver, ALBank *instrumentBank,
                                    s32 eventCapacity, u8 *eventBuffer, SequenceControlState *readerContext,
                                    SequenceEventReader eventReader);

#endif
