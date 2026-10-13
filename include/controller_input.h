/// \file
/// Declares processed controller input, stick-button synthesis and repeat timing.

#ifndef CONTROLLER_INPUT_H
#define CONTROLLER_INPUT_H

#include "controller_types.h"

/// Reset controller button history, press timestamps, and repeat configuration.
/// Disables stick-derived buttons, enables D-pad repeat, restores default repeat
/// timing, and clears the processed state for all four ports.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerResetInputState (0x80004AD0).
void controllerResetInputState(void);

/// Update held, pressed, released, and repeated buttons from cached pad samples.
/// Applies optional stick-derived direction bits and updates per-button timing.
/// Does not poll hardware; ports with invalid samples retain their prior state.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerUpdateInputState (0x80004BC0).
void controllerUpdateInputState(void);

/// Select which held buttons generate auto-repeat events.
/// \param buttonMask Controller button bits eligible for auto-repeat. New
/// presses appear in the repeated-button result regardless of this mask.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerSetRepeatMask (0x80004F18).
void controllerSetRepeatMask(u32 buttonMask);

/// Configure direction-button synthesis from the analog stick.
/// \param enabled Nonzero to synthesize buttons; zero to disable synthesis
/// without changing the saved thresholds.
/// \param upThreshold Positive stick-Y threshold; triggers when exceeded.
/// \param downThreshold Magnitude of the negative stick-Y threshold.
/// \param leftThreshold Magnitude of the negative stick-X threshold.
/// \param rightThreshold Positive stick-X threshold; triggers when exceeded.
/// Negative-direction comparisons are strict, against the negated magnitudes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerConfigureStickButtons (0x80004F24).
void controllerConfigureStickButtons(s32 enabled, s32 upThreshold, s32 downThreshold, s32 leftThreshold,
                                     s32 rightThreshold);

/// Set the initial delay and interval for held-button auto-repeat.
/// \param initialDelaySeconds Nonnegative delay before the first repeat.
/// \param repeatIntervalSeconds Nonnegative delay between subsequent repeats.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerSetRepeatTiming (0x80004F64).
void controllerSetRepeatTiming(f32 initialDelaySeconds, f32 repeatIntervalSeconds);

/// Measure how long the oldest currently held button has been held.
/// \param port Controller port index, 0 through 3; not bounds-checked.
/// \param unused Reserved argument, ignored by this implementation.
/// \returns Hold duration in seconds, or zero when no buttons are held.
/// \note Original decompilation identifier: \c controllerGetLongestHoldSeconds (0x80005018).
f32 controllerGetLongestHoldSeconds(s32 port, s32 unused);

/// Test whether a port has a connected, error-free standard controller.
/// \param port Controller port index, 0 through 3.
/// \returns 1 for a standard controller with valid status, 0 otherwise.
/// \note Original decompilation identifier: \c controllerIsStandardPad (0x8000513C).
s32 controllerIsStandardPad(s32 port);

#endif
