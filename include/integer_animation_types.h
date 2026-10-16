/// \file
/// Defines integer animation keys and retained playback state used by UI indicators and animated object textures.

#ifndef INTEGER_ANIMATION_TYPES_H
#define INTEGER_ANIMATION_TYPES_H

#include <ultra64.h>

/// Commands recognized by integer animation keys; duration also selects control behavior.
/// Negative-duration keys hold their start value unless the mode is a relative jump.
/// Nonnegative-duration keys use constant, inclusive-range or one-shot output modes.
typedef enum IntegerAnimationKeyMode {
    INTEGER_ANIMATION_KEY_CONSTANT = 0, ///< Keep the start value for the timed key.
    INTEGER_ANIMATION_KEY_RANGE = 1,    ///< Traverse the inclusive integer range from start to end.
    INTEGER_ANIMATION_KEY_EMIT = 2,     ///< Emit start once without replacing the retained value.
    INTEGER_ANIMATION_KEY_JUMP = 3      ///< Use start as a signed key displacement when duration is negative.
} IntegerAnimationKeyMode;

/// Outcome of evaluating one integer animation track.
typedef enum IntegerAnimationResult {
    INTEGER_ANIMATION_ACTIVE = 0, ///< Timed output remains active.
    INTEGER_ANIMATION_HELD = 1,   ///< Absent track or negative-duration held output.
    INTEGER_ANIMATION_EMITTED = 2 ///< One-shot output supplied; the retained value is unchanged.
} IntegerAnimationResult;

/// Borrowed integer-animation key describing a timed value or control command.
typedef struct ValueKey {
    s32 mode;     ///< IntegerAnimationKeyMode command; negative duration also selects control behavior.
    f32 duration; ///< Key duration in seconds; negative values select control keys.
    s16 start;    ///< Initial, held or one-shot value, or signed relative key displacement.
    s16 end;      ///< Inclusive range endpoint; ignored by other supported modes.
} ValueKey;

/// Playback cursor and retained value for an integer animation.
typedef struct ValueTrack {
    f32 time;      ///< Elapsed time within the current key.
    ValueKey *key; ///< Borrowed current key; NULL selects a held value.
    s16 value;     ///< Most recently retained integer value.
} ValueTrack;

typedef char ValueKeySizeCheck[(sizeof(ValueKey) == 12) ? 1 : -1];
typedef char ValueTrackSizeCheck[(sizeof(ValueTrack) == 12) ? 1 : -1];
typedef char ValueTrackValueOffsetCheck[((u32) & ((ValueTrack *)0)->value == 8) ? 1 : -1];

#endif
