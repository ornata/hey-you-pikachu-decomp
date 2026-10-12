/// \file
/// Declares sequence-driver registration, caller-owned voice pools and output controls.

#ifndef SEQUENCE_DRIVER_H
#define SEQUENCE_DRIVER_H

#include "sequence_driver_types.h"

/// Initialize and register a sequence driver with its voice pool.
/// \param p Caller-owned driver state that must remain valid while registered with the synthesizer.
/// \param synth Synthesizer context that must remain valid while the driver is registered.
/// \param outputMode Output mode; one selects mono panning.
/// \param maxVoices Number of records in the supplied voice pool, from one through 255.
/// \param voices Caller-owned pool with at least maxVoices records, valid while the driver is registered.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007420 (0x80007420).
void sequenceEngineInitializeDriver(VoicePlayer *p, N_ALGlobals *synth, s32 outputMode, s32 maxVoices,
                                    SequenceVoice *voices);

/// Set the master volume for the sequence driver.
/// \param driver Sequence driver to update.
/// \param volume Normalized master volume from zero through one.
/// \returns Always one.
/// \note Original decompilation identifier: \c func_80007538 (0x80007538).
s32 sequenceEngineSetMasterVolume(VoicePlayer *driver, f64 volume);

/// Set the sequence driver output mode.
/// \param driver Sequence driver to update.
/// \param outputMode Output mode; one selects mono panning.
/// \returns Always one.
/// \note Original decompilation identifier: \c func_800075E0 (0x800075E0).
s32 sequenceEngineSetOutputMode(VoicePlayer *driver, s32 outputMode);

#endif
