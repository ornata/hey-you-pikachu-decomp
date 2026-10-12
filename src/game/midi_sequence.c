/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "midi_sequence_types.h"
void midiSequencePlaybackTransitionHook(void);
s32 midiReaderOpen(u8 *, s32);

/// Provide the inactive notification hook for sequence start and stop transitions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800051A0 (0x800051A0).
void midiSequencePlaybackTransitionHook(void) {
}

/// Initialize the sequence playback and pending-release state.
/// \param state Sequence state to initialize.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800051A8 (0x800051A8).
s32 midiSequenceInitialize(u8 *state) {
    s32 i;

    state[0x59C] = 1;
    state[0x59D] = 0;
    state[0x59E] = 0;
    *(s32 *)(state + 0x458) = 0;
    for (i = 0; i < 16; i++) {
        ((s32 *)(state + 0x55C))[i] = 0;
    }
    return 0;
}

/// Vary enabled MIDI channels while keeping the lowest eight channels enabled.
/// \param state Sequence state whose channel mask is changed.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800051EC (0x800051EC).
s32 midiSequenceRandomizeUpperChannels(u8 *state) {
    ((SequenceState *)state)->channelMask = (s16)(osGetCount() | 0xFF);
    return 0;
}

/// Enable note playback on every MIDI channel.
/// \param state Sequence state whose channel mask is changed.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_8000521C (0x8000521C).
s32 midiSequenceEnableAllChannels(u8 *state) {
    *(u16 *)(state + 0x5A8) = 0xFFFF;
    return 0;
}

/// Start reading a ROM-backed MIDI sequence when playback is stopped.
/// \param state Initialized sequence state to start.
/// \param romAddress ROM address of the sequence data.
/// \returns 1 when playback starts, or 0 when already playing or the sequence cannot be opened.
/// \note Original decompilation identifier: \c func_8000522C (0x8000522C).
s32 midiSequenceStart(u8 *state, s32 romAddress) {
    s32 var_v0;
    var_v0 = 0;
    if (state[0x59C] != 0) {
        *(s16 *)(state + 0x444) = midiReaderOpen(state, romAddress);
        if (*(s16 *)(state + 0x444) == 0) {
            return 0;
        }
        *(s32 *)(state + 0x448) = 0x7A120;
        *(s8 *)(state + 0x59F) = 0;
        *(f64 *)(state + 0x450) = 1.0;
        *(u16 *)(state + 0x5A8) = 0xFFFF;
        *(s16 *)(state + 0x5AA) = 0;
        midiSequencePlaybackTransitionHook();
        var_v0 = 1;
        state[0x59C] = 0;
        return var_v0;
    }
    return var_v0;
}

/// Stop sequence playback and request release of held notes and sustain.
/// \param state Sequence state to stop.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800052BC (0x800052BC).
s32 midiSequenceStop(u8 *state) {
    if (state[0x59C] == 0) {
        state[0x59D] = 1;
        state[0x59E] = 1;
        midiSequencePlaybackTransitionHook();
        state[0x59C] = 1;
        return 0;
    }
    return 0;
}

s32 midiReaderRestoreMark(u8 *, u8 *);
s32 midiReaderSaveMark(u8 *, u8 *);
s32 midiReaderReadEvent(u8 *, u8 *, s32 *, s32);
/// Conversion factor from MIDI tempo microseconds to elapsed seconds.
/// \note Original identifier: \c D_800A0210 (0x800A0210).
extern f64 gMidiMicrosecondsToSeconds;
/// Get the next playable MIDI event and its delay, including pending playback releases.
/// \param state Sequence state to advance.
/// \param event Destination with room for at least capacity + 1 event bytes.
/// \param capacity Event data limit in bytes, at least six to retain tempo events.
/// \param delta Receives the delay in seconds before the returned event, or -1 when playback and pending releases have ended.
/// \returns The event length in bytes, or zero when playback and pending releases have ended.
/// \note Original decompilation identifier: \c func_8000530C (0x8000530C).
s32 midiSequenceReadEvent(SequenceState *state, u8 *event, s32 capacity, f64 *delta) {
    s32 length;
    f64 elapsed;
    s32 channel;
    s32 ticks;
    s32 i;
    u8 sub;
    *delta = 0.0;
    for (;;) {
        if (state->releaseSustain) {
            for (channel = 0; channel < 16; channel++) {
                if (state->sustain[channel] >= 64) {
                    state->sustain[channel] = 0;
                    event[0] = channel | 0xB0;
                    event[1] = 64;
                    event[2] = 0;
                    return 3;
                }
            }
            state->releaseSustain = 0;
        }
        if (state->releaseNotes) {
            if (state->noteCount != 0) {
                --state->noteCount;
                event[0] = state->noteChannels[state->noteCount] | 0x90;
                event[1] = state->noteKeys[state->noteCount];
                event[2] = 0;
                return 3;
            }
            state->releaseNotes = 0;
        }
        if (state->stopped) {
            *delta = -1.0;
            return 0;
        }
        ticks = midiReaderReadEvent((u8 *)state, event, &length, capacity);
        if (ticks < 0) {
            midiSequenceStop((u8 *)state);
            continue;
        }
        elapsed = (ticks * gMidiMicrosecondsToSeconds * state->tempo) / state->division;
        *delta += elapsed / state->rate;
        switch (event[0] & 0xF0) {
        case 0xF0:
            if (event[0] == 0xF0 || event[0] == 0xF7)
                continue;
            if (event[0] == 0xFF) {
                sub = event[1];
                switch (sub) {
                case 0x51:
                    state->tempo = ((event[3] << 16) | ((u32)event[4] << 8)) | (u32)event[5];
                    break;
                case 0x2F:
                    midiSequenceStop((u8 *)state);
                    break;
                }
                continue;
            }
            break;
        case 0xB0:
            sub = event[1];
            switch (sub) {
            case 0x40:
                state->sustain[event[0] & 0xF] = event[2];
                break;
            case 0x66:
                midiReaderSaveMark((u8 *)state, (u8 *)&state->loop);
                state->hasLoop = 1;
                continue;
            case 0x67:
                state->releaseNotes = 1;
                if (state->repetitions % 3 > 0 && state->repetitions >= 4)
                    midiSequenceRandomizeUpperChannels((u8 *)state);
                else
                    midiSequenceEnableAllChannels((u8 *)state);
                ++state->repetitions;
                if (state->hasLoop)
                    midiReaderRestoreMark((u8 *)state, (u8 *)&state->loop);
                continue;
            }
            break;
        case 0x90:
            if (event[2] != 0) {
                if (((1 << (event[0] & 0xF)) & state->channelMask) && state->noteCount < 32) {
                    state->noteChannels[state->noteCount] = event[0] & 0xF;
                    state->noteKeys[state->noteCount] = event[1];
                    ++state->noteCount;
                    break;
                }
                continue;
            }
        case 0x80:
            for (i = 0; i < state->noteCount; i++) {
                if (state->noteChannels[i] == (event[0] & 0xF) && state->noteKeys[i] == event[1]) {
                    --state->noteCount;
                    state->noteChannels[i] = state->noteChannels[state->noteCount];
                    state->noteKeys[i] = state->noteKeys[state->noteCount];
                    return length;
                }
            }
            break;
        default:
            break;
        }
        return length;
    }
}

/// Check whether playback is stopped and all requested releases have been emitted.
/// \param state Sequence state to inspect.
/// \returns 1 when playback and pending releases have ended, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80005768 (0x80005768).
s32 midiSequenceIsDrained(u8 *state) {
    if ((((SequenceState *)state)->stopped != 0) && (((SequenceState *)state)->releaseNotes == 0) &&
        (((SequenceState *)state)->releaseSustain == 0)) {
        return 1;
    }
    return 0;
}

/// Check whether sequence playback is stopped.
/// \param state Sequence state to inspect.
/// \returns Nonzero when playback is stopped, even if release events remain, or zero while playing.
/// \note Original decompilation identifier: \c func_800057A0 (0x800057A0).
u8 midiSequenceIsStopped(u8 *state) {
    return ((SequenceState *)state)->stopped;
}

/// Set the sequence playback speed multiplier.
/// \param state Sequence state to update.
/// \param rate Positive playback multiplier; 1 preserves the original tempo.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800057A8 (0x800057A8).
s32 midiSequenceSetRate(u8 *state, f64 rate) {
    *(f64 *)(state + 0x450) = rate;
    return 0;
}

/// Get the sequence playback speed multiplier.
/// \param state Sequence state to inspect.
/// \returns The configured playback multiplier.
/// \note Original decompilation identifier: \c func_800057BC (0x800057BC).
f64 midiSequenceGetRate(u8 *state) {
    return ((SequenceState *)state)->rate;
}
