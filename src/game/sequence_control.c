/* Sequence control API; its read-only constants end at ROM A0E30. */
#include "common.h"
#include "midi_sequence_types.h"

s32 midiSequenceInitialize(u8 *);
s32 midiSequenceStop(u8 *);
u8 midiSequenceIsStopped(u8 *);

/// Initialize sequence playback with no pending control requests.
/// \param state Sequence control state to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800057D0 (0x800057D0).
void sequenceControlInitialize(u8 *state) {
    ((SequenceControlState *)state)->resetPending = 0;
    ((SequenceControlState *)state)->stopDelay = -1.0;
    ((SequenceControlState *)state)->fadeDuration = -1.0;
    ((SequenceControlState *)state)->immediateLevel = -1.0;
    midiSequenceInitialize(state);
}

s32 midiSequenceStart(u8 *, s32); /* extern */

/// Request playback and an engine reset for a stopped sequence.
/// \param state Initialized sequence control state.
/// \param romAddress ROM address of the MIDI sequence.
/// \returns 1 when playback starts, or 0 when already playing or the sequence cannot be opened.
/// \note Original decompilation identifier: \c func_80005808 (0x80005808).
s32 sequenceControlStart(u8 *state, s32 romAddress) {
    if (midiSequenceIsStopped(state) != 0) {
        ((SequenceControlState *)state)->resetPending = 1;
        ((SequenceControlState *)state)->stopDelay = -1.0;
        ((SequenceControlState *)state)->fadeDuration = -1.0;
        return midiSequenceStart(state, romAddress);
    }
    return 0;
}

/// Stop playback immediately or shorten its pending stop deadline.
/// \param state Sequence control state to update.
/// \param delay Nonnegative delay in seconds; zero stops playback immediately.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005864 (0x80005864).
s32 sequenceControlScheduleStop(u8 *state, f64 delay) {
    f64 cur;

    if (midiSequenceIsStopped(state) == 0) {
        if (delay == 0.0) {
            return midiSequenceStop(state);
        }
        cur = ((SequenceControlState *)state)->stopDelay;
        if ((cur < 0.0) || (delay < cur)) {
            ((SequenceControlState *)state)->stopDelay = delay;
        }
    }
    return 0;
}

/// Queue an immediate volume change or a timed volume fade.
/// \param state Sequence control state receiving the request.
/// \param duration Nonnegative fade duration in seconds; zero requests an immediate change.
/// \param level Target volume fraction, clamped to [0, 1] when the event is produced.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800058F4 (0x800058F4).
s32 sequenceControlRequestVolume(u8 *state, f64 duration, f64 level) {
    if (duration == 0.0) {
        ((SequenceControlState *)state)->immediateLevel = level;
    } else {
        ((SequenceControlState *)state)->fadeDuration = duration;
        ((SequenceControlState *)state)->fadeLevel = level;
    }
    return 0;
}

/// Encode a sequence-engine volume change event.
/// \param duration Fade duration in seconds, quantized downward to sixteenths and clamped to [0, 127/16].
/// \param level Target volume fraction, quantized downward to 127ths and clamped to [0, 1].
/// \param event Destination with room for eight event bytes.
/// \returns 8, the encoded event length.
/// \note Original decompilation identifier: \c func_80005938 (0x80005938).
s32 sequenceControlEncodeVolumeEvent(f64 duration, f64 level, u8 *event) {
    s32 var_a0;
    s32 var_v1;

    var_v1 = (s32)(duration * 16.0);
    if (var_v1 < 0) {
        var_v1 = 0;
    }
    if (var_v1 >= 0x80) {
        var_v1 = 0x7F;
    }
    var_a0 = (s32)(level * 127.0);
    if (var_a0 < 0) {
        var_a0 = 0;
    }
    if (var_a0 >= 0x80) {
        var_a0 = 0x7F;
    }
    event[0] = 0xF0;
    event[1] = 0x7F;
    event[2] = 0x7F;
    event[3] = 4;
    event[4] = 3;
    event[5] = var_v1;
    event[6] = var_a0;
    event[7] = 0xF7;
    return 8;
}

s32 midiSequenceReadEvent(u8 *, u8 *, s32, f64 *);
/// Get the next engine control or MIDI event and update the playback stop deadline.
/// \param state Sequence control state to advance.
/// \param event Destination for the event; must have room for at least eight bytes.
/// \param capacity Event buffer capacity in bytes.
/// \param delta Receives the delay in seconds, zero for an immediate control event, or -1 after playback releases have ended.
/// \returns The event length, including a one-byte reset marker when requested, or zero when no event remains.
/// \note Original decompilation identifier: \c func_800059E4 (0x800059E4).
s32 sequenceControlReadEvent(u8 *state, u8 *event, s32 capacity, f64 *delta) {
    s32 result;
    if (((SequenceControlState *)state)->stopDelay == 0.0) {
        midiSequenceStop(state);
        ((SequenceControlState *)state)->stopDelay = -1.0;
    }
    if (((SequenceControlState *)state)->resetPending != 0) {
        *delta = 0.0;
        *event = 0xFF;
        ((SequenceControlState *)state)->resetPending = 0;
        return 1;
    }
    if (((SequenceControlState *)state)->immediateLevel >= 0.0) {
        result = sequenceControlEncodeVolumeEvent(0.0, ((SequenceControlState *)state)->immediateLevel, event);
        ((SequenceControlState *)state)->immediateLevel = -1.0;
        *delta = 0.0;
        return result;
    }
    if (((SequenceControlState *)state)->fadeDuration >= 0.0) {
        result = sequenceControlEncodeVolumeEvent(((SequenceControlState *)state)->fadeDuration,
                                                  ((SequenceControlState *)state)->fadeLevel, event);
        ((SequenceControlState *)state)->fadeDuration = -1.0;
        *delta = 0.0;
        return result;
    }
    result = midiSequenceReadEvent(state, event, capacity, delta);
    if (*delta > 0.0 && ((SequenceControlState *)state)->stopDelay >= 0.0) {
        if (((SequenceControlState *)state)->stopDelay < *delta) {
            ((SequenceControlState *)state)->stopDelay = 0.0;
        } else {
            ((SequenceControlState *)state)->stopDelay -= *delta;
        }
    }
    return result;
}
