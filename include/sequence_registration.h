/// \file
/// Declares registration of caller-owned sequence state and borrowed event storage.

#ifndef SEQUENCE_REGISTRATION_H
#define SEQUENCE_REGISTRATION_H

#include "sequence_engine_types.h"

/// Initialize a sequence and register it for playback by the driver.
/// \param sequence Caller-owned state of at least 512 bytes, not already registered, which must remain valid while registered.
/// \param driver Initialized sequence driver receiving the sequence.
/// \param instrumentBank Borrowed instrument bank with a valid default instrument, retained while the sequence and its voices use it.
/// \param eventLimit Reader-specific event limit, representable as an unsigned byte.
/// \param eventBuffer Caller-owned storage meeting the selected reader's contract, valid while registered.
/// \param readerContext Borrowed context passed to the event reader, valid while the sequence is registered.
/// \param eventReader Retained callback receiving context, destination, reader-specific event limit
/// and output delay. It must honor its destination-storage contract and produce complete supported events.
/// \pre The callback must be valid at registration and while the sequence is registered;
/// registration can request the first event immediately. Zero-delay event runs must be finite.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800060A8 (0x800060A8).
void sequenceEngineRegisterSequence(EngineSequence *sequence, VoicePlayer *driver, ALBank *instrumentBank,
                                    s32 eventLimit, u8 *eventBuffer, SequenceControlState *readerContext,
                                    SequenceEventReader eventReader);

#endif
