#include "controller_state.h"
#include "controller_input.h"
#include "controller_pad.h"
#include "common.h"
#include "controller_types.h"

/// Seconds-to-microseconds scale for the initial button-repeat delay.
/// \note Original identifier: \c D_800A01FC (0x800A01FC).
extern f32 gControllerInitialDelayMicrosecondsPerSecond;
/// Seconds-to-microseconds scale for the button-repeat interval.
/// \note Original identifier: \c D_800A0200 (0x800A0200).
extern f32 gControllerRepeatIntervalMicrosecondsPerSecond;

/* Controller history and repeat/stick configuration, bss 0x800AD9A0..0x800AD9E0;
 * declared in address order, which is IDO's layout order */
/// \note Original identifier: \c D_800AD9A0 (0x800AD9A0).
u32 gControllerPreviousButtons[4]; /* previous frame button mask per port */
/// \note Original identifier: \c __additional_scanline_2 (0x800AD9B0).
u32 gControllerRepeatMask;
/// Retained unused word preceding aligned controller repeat timing; original purpose remains unresolved.
/// \note Original identifier: \c D_800AD9B4 (0x800AD9B4).
u32 gControllerRepeatTimingReservedWord;
/// \note Original identifier: \c D_800AD9B8 (0x800AD9B8).
u64 gControllerInitialRepeatTicks;
/// \note Original identifier: \c D_800AD9C0 (0x800AD9C0).
u64 gControllerRepeatIntervalTicks;
/// \note Original identifier: \c D_800AD9C8 (0x800AD9C8).
s32 gControllerStickButtonsEnabled;
/// \note Original identifier: \c D_800AD9CC (0x800AD9CC).
s32 gControllerStickUpThreshold;
/// \note Original identifier: \c D_800AD9D0 (0x800AD9D0).
s32 gControllerStickDownThreshold;
/// \note Original identifier: \c D_800AD9D4 (0x800AD9D4).
s32 gControllerStickLeftThreshold;
/// \note Original identifier: \c D_800AD9D8 (0x800AD9D8).
s32 gControllerStickRightThreshold;
/// Retained unused word at the end of controller configuration; original purpose remains unresolved.
/// \note Original identifier: \c D_800AD9DC (0x800AD9DC).
s32 gControllerConfigurationReservedWord;

/// Last press time in OS clock ticks for each port and button bit.
/// \note Original identifier: \c D_800AD5A0 (0x800AD5A0).
extern OSTime gControllerButtonPressTicks[4][16];
/// Next auto-repeat deadline in OS clock ticks for each port and button bit.
/// \note Original identifier: \c D_800AD7A0 (0x800AD7A0).
extern OSTime gControllerNextRepeatTicks[4][16]; /* next auto-repeat time per port and button */

/// \note Original identifier: \c D_800A01F4 (0x800A01F4).
extern f32 gControllerDefaultRepeatDelaySeconds;
/// \note Original identifier: \c D_800A01F8 (0x800A01F8).
extern f32 gControllerDefaultRepeatIntervalSeconds;

/// Reset controller button history, press timestamps, and repeat configuration.
/// Disables stick-derived buttons, enables D-pad repeat, restores default repeat
/// timing, and clears the processed state for all four ports.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerResetInputState (0x80004AD0).
void controllerResetInputState(void) {
    s32 port;
    s32 button;
    OSTime now;

    for (port = 0; port < 4; port++) {
        now = osGetTime();
        for (button = 0; button < 16; button++) {
            gControllerButtonPressTicks[port][button] = now;
        }
    }
    _bzero(gControllerNextRepeatTicks, sizeof(gControllerNextRepeatTicks));
    _bzero(gControllerPreviousButtons, sizeof(gControllerPreviousButtons));
    controllerConfigureStickButtons(0, 30, 30, 30, 30);
    controllerSetRepeatMask(0xF00);
    controllerSetRepeatTiming(gControllerDefaultRepeatDelaySeconds, gControllerDefaultRepeatIntervalSeconds);
    _bzero(gControllerInputState, sizeof(gControllerInputState));
}

/* Debug trace: compiled out in this build, as a statement-safe macro. The
 * formats live in this unit's rodata blob (boot_rodata_A0DC0), so they are
 * referenced by name; the dead blocks still shape uopt's flow graph. */
#define TRACE(args)                                                                                                    \
    do {                                                                                                               \
        if (0) {                                                                                                       \
            osSyncPrintf args;                                                                                         \
        }                                                                                                              \
    } while (0)
/// Diagnostic format string for processed controller input.
/// \note Original identifier: \c D_800A01C0 (0x800A01C0).
extern char gControllerInputTraceFormat[]; /* "send(%d, %d, %.3f, %.3f, %.3f)\n" */
/// Diagnostic format string for a controller port with no valid sample.
/// \note Original identifier: \c D_800A01E0 (0x800A01E0).
extern char gControllerInvalidPortTraceFormat[]; /* "pad-%d is invalid\n" */

/// Update held, pressed, released, and repeated buttons from cached pad samples.
/// Applies optional stick-derived direction bits and updates per-button timing.
/// Does not poll hardware; ports with invalid samples retain their prior state.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerUpdateInputState (0x80004BC0).
void controllerUpdateInputState(void) {
    OSTime now;
    ControllerState *state;
    u32 *previous;
    OSContPad pad;
    s32 port;
    u32 changed;
    s32 button;
    s32 mask;
    u32 repeated;

    now = osGetTime();
    for (port = 0; port < 4; port++) {
        if (controllerReadCachedPad(&pad, port)) {
            state = &gControllerInputState[port];
            if (gControllerStickButtonsEnabled) {
                if (pad.stick_x < gControllerStickLeftThreshold) {
                    pad.button |= 0x240;
                } else if (pad.stick_x > gControllerStickRightThreshold) {
                    pad.button |= 0x140;
                }
                if (pad.stick_y < gControllerStickDownThreshold) {
                    pad.button |= 0x480;
                } else if (pad.stick_y > gControllerStickUpThreshold) {
                    pad.button |= 0x880;
                }
            }
            TRACE((gControllerInputTraceFormat));
            changed = gControllerPreviousButtons[port] ^ pad.button;
            state->held = pad.button;
            state->pressed = pad.button & changed;
            state->released = gControllerPreviousButtons[port] & changed;
            state->stickX = pad.stick_x;
            state->stickY = pad.stick_y;
            previous = &gControllerPreviousButtons[port];
            *previous = pad.button;
            repeated = state->pressed;
            for (button = 0, mask = 1; button < 16; button++, mask <<= 1) {
                if (state->pressed & mask) {
                    gControllerButtonPressTicks[port][button] = now;
                    gControllerNextRepeatTicks[port][button] = now + gControllerInitialRepeatTicks;
                }
                if (pad.button & mask & gControllerRepeatMask) {
                    if (now >= gControllerNextRepeatTicks[port][button]) {
                        repeated |= mask;
                        gControllerNextRepeatTicks[port][button] = now + gControllerRepeatIntervalTicks;
                    }
                }
            }
            state->repeated = repeated;
        } else {
            TRACE((gControllerInvalidPortTraceFormat, port));
        }
    }
}

/// Select which held buttons generate auto-repeat events.
/// \param buttonMask Controller button bits eligible for auto-repeat. New
/// presses appear in the repeated-button result regardless of this mask.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerSetRepeatMask (0x80004F18).
void controllerSetRepeatMask(u32 buttonMask) {
    gControllerRepeatMask = buttonMask;
}

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
                                     s32 rightThreshold) {
    gControllerStickButtonsEnabled = enabled;
    if (enabled != 0) {
        gControllerStickUpThreshold = upThreshold;
        gControllerStickDownThreshold = -downThreshold;
        gControllerStickLeftThreshold = -leftThreshold;
        gControllerStickRightThreshold = rightThreshold;
    }
}

/// Set the initial delay and interval for held-button auto-repeat.
/// \param initialDelaySeconds Nonnegative delay before the first repeat.
/// \param repeatIntervalSeconds Nonnegative delay between subsequent repeats.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerSetRepeatTiming (0x80004F64).
void controllerSetRepeatTiming(f32 initialDelaySeconds, f32 repeatIntervalSeconds) {
    gControllerInitialRepeatTicks =
        (u64)(initialDelaySeconds * gControllerInitialDelayMicrosecondsPerSecond) * 3000 / 64;
    gControllerRepeatIntervalTicks =
        (u64)(repeatIntervalSeconds * gControllerRepeatIntervalMicrosecondsPerSecond) * 3000 / 64;
}

/// Microseconds-per-second divisor used to report held-button duration.
/// \note Original identifier: \c D_800A0204 (0x800A0204).
extern f32 gControllerHoldMicrosecondsPerSecond;

/// Measure how long the oldest currently held button has been held.
/// \param port Controller port index, 0 through 3; not bounds-checked.
/// \param unused Reserved argument, ignored by this implementation.
/// \returns Hold duration in seconds, or zero when no buttons are held.
/// \note Original decompilation identifier: \c controllerGetLongestHoldSeconds (0x80005018).
f32 controllerGetLongestHoldSeconds(s32 port, s32 unused) {
    f32 duration;
    OSTime earliest;
    OSTime now;
    OSTime elapsed;
    OSTime scaled;
    s32 i;
    s32 mask;
    OSTime pressed;

    earliest = now = osGetTime();
    for (i = 0, mask = 1; i < 16; i++, mask <<= 1) {
        if (gControllerInputState[port].held & mask) {
            pressed = gControllerButtonPressTicks[port][i];
            if (pressed < earliest) {
                earliest = pressed;
            }
        }
    }
    elapsed = now - earliest;
    scaled = elapsed * 64;
    duration = (f32)(scaled / 3000);
    return duration / gControllerHoldMicrosecondsPerSecond;
}

/// Test whether a port has a connected, error-free standard controller.
/// \param port Controller port index, 0 through 3.
/// \returns 1 for a standard controller with valid status, 0 otherwise.
/// \note Original decompilation identifier: \c controllerIsStandardPad (0x8000513C).
s32 controllerIsStandardPad(s32 port) {
    OSContStatus *status;

    status = controllerGetPortStatus(port);
    if ((controllerIsPortConnected(port) != 0) && (status->errno == 0) && ((status->type & 0x1F07) == 5)) {
        return 1;
    }
    return 0;
}
