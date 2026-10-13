#include "midi_sequence.h"
#include "sequence_control.h"
/* Sequence control API; its read-only constants end at ROM A0E30. */
#include "common.h"
#include "midi_sequence_types.h"


/// Initialize sequence playback with no pending control requests.
/// \param state Sequence control state to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800057D0 (0x800057D0).
void sequenceControlInitialize(SequenceControlState *state) {
    state->resetPending = 0;
    state->stopDelay = -1.0;
    state->fadeDuration = -1.0;
    state->immediateLevel = -1.0;
    midiSequenceInitialize(&state->playback);
}


/// Request playback and an engine reset for a stopped sequence.
/// \param state Initialized sequence control state.
/// \param romAddress ROM address of the MIDI sequence.
/// \returns 1 when playback starts, or 0 when already playing or the sequence cannot be opened.
/// \note Original decompilation identifier: \c func_80005808 (0x80005808).
s32 sequenceControlStart(SequenceControlState *state, s32 romAddress) {
    if (midiSequenceIsStopped(&state->playback) != 0) {
        state->resetPending = 1;
        state->stopDelay = -1.0;
        state->fadeDuration = -1.0;
        return midiSequenceStart(&state->playback, romAddress);
    }
    return 0;
}

/// Stop playback immediately or shorten its pending stop deadline.
/// \param state Sequence control state to update.
/// \param delay Nonnegative delay in seconds; zero stops playback immediately.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005864 (0x80005864).
s32 sequenceControlScheduleStop(SequenceControlState *state, f64 delay) {
    f64 cur;

    if (midiSequenceIsStopped(&state->playback) == 0) {
        if (delay == 0.0) {
            return midiSequenceStop(&state->playback);
        }
        cur = state->stopDelay;
        if ((cur < 0.0) || (delay < cur)) {
            state->stopDelay = delay;
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
s32 sequenceControlRequestVolume(SequenceControlState *state, f64 duration, f64 level) {
    if (duration == 0.0) {
        state->immediateLevel = level;
    } else {
        state->fadeDuration = duration;
        state->fadeLevel = level;
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
    s32 encodedLevel;
    s32 encodedDuration;

    encodedDuration = (s32)(duration * 16.0);
    if (encodedDuration < 0) {
        encodedDuration = 0;
    }
    if (encodedDuration >= 0x80) {
        encodedDuration = 0x7F;
    }
    encodedLevel = (s32)(level * 127.0);
    if (encodedLevel < 0) {
        encodedLevel = 0;
    }
    if (encodedLevel >= 0x80) {
        encodedLevel = 0x7F;
    }
    event[0] = 0xF0;
    event[1] = 0x7F;
    event[2] = 0x7F;
    event[3] = 4;
    event[4] = 3;
    event[5] = encodedDuration;
    event[6] = encodedLevel;
    event[7] = 0xF7;
    return 8;
}

/// Get the next engine control or MIDI event and update the playback stop deadline.
/// \param state Sequence control state to advance.
/// \param event Destination for the event; must have room for at least eight bytes.
/// \param capacity Event buffer capacity in bytes.
/// \param delta Receives the delay in seconds, zero for an immediate control event, or -1 after playback releases have ended.
/// \returns The event length, including a one-byte reset marker when requested, or zero when no event remains.
/// \note Original decompilation identifier: \c func_800059E4 (0x800059E4).
s32 sequenceControlReadEvent(SequenceControlState *state, u8 *event, s32 capacity, f64 *delta) {
    s32 result;
    if (state->stopDelay == 0.0) {
        midiSequenceStop(&state->playback);
        state->stopDelay = -1.0;
    }
    if (state->resetPending != 0) {
        *delta = 0.0;
        *event = 0xFF;
        state->resetPending = 0;
        return 1;
    }
    if (state->immediateLevel >= 0.0) {
        result = sequenceControlEncodeVolumeEvent(0.0, state->immediateLevel, event);
        state->immediateLevel = -1.0;
        *delta = 0.0;
        return result;
    }
    if (state->fadeDuration >= 0.0) {
        result = sequenceControlEncodeVolumeEvent(state->fadeDuration,
                                                  state->fadeLevel, event);
        state->fadeDuration = -1.0;
        *delta = 0.0;
        return result;
    }
    result = midiSequenceReadEvent(&state->playback, event, capacity, delta);
    if (*delta > 0.0 && state->stopDelay >= 0.0) {
        if (state->stopDelay < *delta) {
            state->stopDelay = 0.0;
        } else {
            state->stopDelay -= *delta;
        }
    }
    return result;
}
