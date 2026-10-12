#include "common.h"
#include "integer_animation_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/* One cubic keyframe: value = ((a*t + b)*t + c)*t + base + frac/256; lasts len ticks (0xFF = hold). */
typedef struct Key16 {
    f32 a;
    f32 b;
    f32 c;
    s16 base;
    u8 frac;
    u8 len;
} Key16;

s32 animationIntegerTrackEvaluate(void *, s32 *, f32);

/// Advance a scalar animation track to the key for the updated time.
/// \param t Current key time in seconds, updated by the step.
/// \param keys Current key pointer, advanced when the key duration is exceeded.
/// \param out Receives the held value for a hold key; otherwise unchanged.
/// \param idx Address of the writable key index, incremented when advancing keys.
/// \param step Time step in seconds.
/// \returns 0 for an active key, 1 for a hold key, or 2 for an absent track.
///
/// \note NONPORTABLE: idx carries a writable s32 pointer in the original 32-bit ABI. UB: keys is a
/// Key16 pointer slot exposed through the existing f32 ** interface.
/// \note Original decompilation identifier: \c func_8000A000 (0x8000A000).
s32 animationTrackAdvanceKey(f32 *t, f32 **keys, f32 *out, s32 idx, f32 step) {
    Key16 **kp = (Key16 **)keys;
    f32 tick;
    f32 d;

    if (*kp == NULL) {
        return 2;
    }
    *t += step;
    tick = 0.03333333507180214f;
    for (;;) {
        if ((*kp)->len == 0xFF) {
            *out = (*kp)->base + (*kp)->frac * (1.0f / 256.0f);
            *t = 0.0f;
            return 1;
        }
        d = (*kp)->len * tick;
        if (*t <= d) {
            break;
        }
        *t -= d;
        (*kp)++;
        (*(s32 *)idx)++;
    }
    return 0;
}

s32 animationTrackAdvanceKey(f32 *, f32 **, f32 *, s32, f32);

/// Advance and evaluate a cubic scalar animation track.
/// \param t Current key time in seconds, advanced before evaluation.
/// \param keys Current key pointer, updated as the track advances.
/// \param out Receives the evaluated or held value; unchanged for an absent track.
/// \param vel Optional destination for the active curve slope; unchanged for held or absent tracks.
/// \param idx Address of the writable key index, updated as the track advances.
/// \param step Time step in seconds.
/// \returns 0 for an active key, 1 for a hold key, or 2 for an absent track.
/// \note Original decompilation identifier: \c func_8000A0F4 (0x8000A0F4).
s32 animationTrackEvaluateCubic(f32 *t, f32 **keys, f32 *out, f32 *vel, s32 idx, f32 step) {
    s32 r;
    Key16 *k;
    f32 x;
    f32 a;
    f32 b;
    f32 c;
    f32 base;

    r = animationTrackAdvanceKey(t, keys, out, idx, step);
    if (r != 0) {
        return r;
    }
    x = *t;
    k = *(Key16 **)keys;
    a = k->a;
    b = k->b;
    c = k->c;
    base = k->frac * (1.0f / 256.0f) + k->base;
    *out = ((a * x + b) * x + c) * x + base;
    if (vel != NULL) {
        *vel = 3.0f * x * x * a + (x + x) * b + c;
    }
    return 0;
}


/// Advance and evaluate a linear scalar animation track.
/// \param t Current key time in seconds, advanced before evaluation.
/// \param keys Current key pointer, updated as the track advances.
/// \param out Receives the evaluated or held value; unchanged for an absent track.
/// \param vel Optional destination for the active curve slope; unchanged for held or absent tracks.
/// \param idx Address of the writable key index, updated as the track advances.
/// \param step Time step in seconds.
/// \returns 0 for an active key, 1 for a hold key, or 2 for an absent track.
/// \note Original decompilation identifier: \c func_8000A1F0 (0x8000A1F0).
s32 animationTrackEvaluateLinear(f32 *t, f32 **keys, f32 *out, f32 *vel, s32 idx, f32 step) {
    s32 r;
    f32 x;
    f32 a;
    f32 b;

    r = animationTrackAdvanceKey(t, keys, out, idx, step);
    if (r != 0) {
        return r;
    }
    x = *t;
    a = (*keys)[0];
    b = (*keys)[1];
    *out = a * x + b;
    if (vel != NULL) {
        *vel = a;
    }
    return 0;
}

s32 animationTrackEvaluateCubic(f32 *, f32 **, f32 *, f32 *, s32, f32);

/// Advance and evaluate an animation curve according to its curve type.
/// \param curve Curve descriptor; keyed curves advance their current key pointer.
/// \param time Current curve time in seconds, updated by the step.
/// \param value Receives the evaluated value when available.
/// \param velocity Optional slope output; zero for a constant curve and unchanged for held or absent tracks.
/// \param keyIndex Address of the writable key index.
/// \param step Time step in seconds.
/// \returns 0 for an active key, 1 for a hold key, or 2 for a constant, absent, or unsupported curve.
/// \note Original decompilation identifier: \c func_8000A26C (0x8000A26C).
s32 animationCurveEvaluate(s32 *curve, f32 *time, f32 *value, f32 *velocity, s32 keyIndex, f32 step) {
    switch (curve[0]) {
    case 0:
        *time += step;
        *value = curve[1] * (1.0f / 65536.0f);
        if (velocity != NULL) {
            *velocity = 0.0f;
        }
        return 2;
    case 1:
        return animationTrackEvaluateCubic(time, (f32 **)(curve + 1), value, velocity, keyIndex, step);
    case 2:
        return animationTrackEvaluateLinear(time, (f32 **)(curve + 1), value, velocity, keyIndex, step);
    default:
        TRACE("error: calcAnime: Illegal keyType(%d)\n", curve[0]);
        return 2;
    }
}

typedef struct Pair8 {
    s32 a;
    s32 b;
} Pair8;
typedef struct Track3 {
    Pair8 p[3];
    f32 f[3];
    s32 c[3];
} Track3;

/// Initialize three animation channels.
/// \param dst Track state to initialize.
/// \param src Three source curve descriptors.
/// \param off Address offset added to each keyed curve reference.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000A34C (0x8000A34C).
void animationTrack3Initialize(Track3 *dst, Pair8 *src, s32 off) {
    s32 i;

    for (i = 0; i < 3; i++, src++) {
        dst->p[i] = *src;
        if (src->a != 0) {
            dst->p[i].b = src->b + off;
        }
        dst->f[i] = 0.0f;
        dst->c[i] = 0;
    }
}

typedef struct Track3b {
    Pair8 p[3];
    f32 f[3];
    s32 c[3];
    f32 g[3];
} Track3b;

/// Initialize three animation channels for incremental output.
/// \param dst Track state to initialize.
/// \param src Three source curve descriptors.
/// \param off Address offset added to each keyed curve reference.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000A3AC (0x8000A3AC).
void animationDeltaTrack3Initialize(Track3b *dst, Pair8 *src, s32 off) {
    s32 i;

    for (i = 0; i < 3; i++, src++) {
        dst->p[i] = *src;
        if (src->a != 0) {
            dst->p[i].b = src->b + off;
        }
        dst->g[i] = 0.0f;
        dst->f[i] = 0.0f;
        dst->c[i] = 0;
    }
}

/// Advance three animation channels and write their available values.
/// \param track Three-channel track state to advance.
/// \param output Receives up to three channel values; absent channels leave their values unchanged.
/// \param velocities Optional three-element slope output; constant channels write
/// zero, while held or absent channels leave their slopes unchanged.
/// \param step Time step in seconds.
/// \returns 1 when every channel is constant, absent, or unsupported; 0 otherwise.
/// \note Original decompilation identifier: \c func_8000A410 (0x8000A410).
s32 animationTrack3Evaluate(Track3 *track, f32 *output, f32 *velocities, f32 step) {
    s32 channel;
    s32 allConstant;
    s32 status;
    f32 *velocity;

    velocity = velocities;
    channel = 0;
    allConstant = 1;
    do {
        status = animationCurveEvaluate(
            (s32 *)((u8 *)track + (channel << 3)), (f32 *)((u8 *)track + (channel << 2) + 0x18),
            (f32 *)((u8 *)output + (channel << 2)), velocity, (s32)((u8 *)track + (channel << 2) + 0x24), step);
        channel++;
        if (status != 2) {
            allConstant = 0;
        }
        if (velocity != NULL) {
            velocity++;
        }
    } while (channel != 3);
    return allConstant;
}

/// Advance three animation channels and add their changes to existing output values.
/// \param track Three-channel track state to advance.
/// \param output Three output values receiving each active or held channel's
/// change; constant, absent, or unsupported channels leave output unchanged.
/// \param velocities Optional three-element slope output; constant channels write
/// zero, while held or absent channels leave their slopes unchanged.
/// \param step Time step in seconds.
/// \returns 1 when every channel is constant, absent, or unsupported; 0 otherwise.
/// \note Original decompilation identifier: \c func_8000A4D4 (0x8000A4D4).
s32 animationDeltaTrack3Evaluate(Track3b *track, f32 *output, f32 *velocities, f32 step) {
    s32 channel;
    s32 allConstant;
    f32 previous;
    f32 *value;
    f32 *velocity;

    velocity = velocities;
    channel = 0;
    value = track->g;
    allConstant = 1;
    do {
        previous = *value;
        if (animationCurveEvaluate((s32 *)((u8 *)track + (channel << 3)), (f32 *)((u8 *)track + (channel << 2) + 0x18),
                                   value, velocity, (s32)((u8 *)track + (channel << 2) + 0x24), step) != 2) {
            allConstant = 0;
            output[channel] += *value - previous;
        }
        channel++;
        if (velocity != NULL) {
            velocity++;
        }
        value++;
    } while (channel != 3);
    return allConstant;
}

/// Initialize an integer animation track.
/// \param track Track state to initialize.
/// \param keys Address of the first integer key, or zero for no keys; key data
/// must remain valid while the track uses it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000A5CC (0x8000A5CC).
void animationIntegerTrackInitialize(f32 *track, s32 keys) {
    s32 sp1C;
    *(s32 *)((u8 *)track + 4) = keys;
    *(s16 *)((u8 *)track + 8) = 0;
    track[0] = 0.0f;
    animationIntegerTrackEvaluate(track, &sp1C, 0.0f);
}

/// Evaluate the current integer animation value and advance its time.
/// \param state Integer animation track state to update.
/// \param out Receives the current or emitted integer value.
/// \param step Time step in seconds, applied after evaluation.
/// \returns 0 for an active value, 1 for a held or absent track, or 2 for a one-shot value.
/// \note Original decompilation identifier: \c func_8000A600 (0x8000A600).
s32 animationIntegerTrackEvaluate(void *state, s32 *out, f32 step) {
    ValueTrack *track = (ValueTrack *)state;
    ValueKey *key = track->key;
    f32 duration;
    s32 lower;
    s32 upper;
    s32 span;

    *out = track->value;
    if (key == NULL) {
        return 1;
    }
    for (;;) {
        duration = key->duration;
        if (duration < 0.0f) {
            if (key->mode == 3) {
                key += key->start;
                track->key = key;
                continue;
            }
            *out = key->start;
            track->value = *out;
            track->time = 0.0f;
            return 1;
        }
        if (key->mode == 2) {
            *out = key->start;
            track->key = key + 1;
            track->time += step;
            return 2;
        }
        if (track->time <= duration) {
            break;
        }
        key++;
        track->key = key;
        track->time -= duration;
    }
    switch (key->mode) {
    case 0:
        *out = key->start;
        break;
    case 1:
        span = key->end - key->start;
        if (key->start < key->end) {
            lower = key->start;
            upper = key->end;
            span++;
        } else {
            lower = key->end;
            upper = key->start;
            span--;
        }
        *out = key->start + (s32)((span * track->time) / duration);
        if (*out < lower) {
            *out = lower;
        } else if (upper < *out) {
            *out = upper;
        }
        break;
    }
    track->value = *out;
    track->time += step;
    return 0;
}
