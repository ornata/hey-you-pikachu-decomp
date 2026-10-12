/// \file
/// Defines integer animation keys and retained playback state used by UI indicators.

#ifndef INTEGER_ANIMATION_TYPES_H
#define INTEGER_ANIMATION_TYPES_H

#include <ultra64.h>

/// Borrowed integer-animation key describing a timed value or control command.
typedef struct ValueKey {
    s32 mode;       ///< Key command interpreted by the integer animation evaluator.
    f32 duration;   ///< Key duration in seconds; negative values select control keys.
    s16 start, end; ///< Integer endpoints interpreted according to the key mode.
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
