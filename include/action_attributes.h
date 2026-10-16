/// \file
/// Defines recovered action-resource attribute fields governing playback and pose transitions.

#ifndef ACTION_ATTRIBUTES_H
#define ACTION_ATTRIBUTES_H

/// Known fields in the signed 32-bit action attribute word.
/// The playback selector shares MotionPlaybackMode encodings. Other bits remain
/// explicit; these names neither change stored widths nor guarantee a transition.
typedef enum ActionAttribute {
    ACTION_ATTRIBUTE_PLAYBACK_MODE_MASK = 7, ///< Low bits selecting the motion playback policy.
    ACTION_ATTRIBUTE_LOOP = 8,               ///< Permit another cycle unless exit, update or initialization rejects it.
    ACTION_ATTRIBUTE_SUPPRESS_TRANSITION_CUES =
        0x20000, ///< Suppress cues when a tracked action is loaded as an intermediary transition.
    ACTION_ATTRIBUTE_REGISTER_POSE_TRANSITION =
        0x40000, ///< Offer differing endpoint poses to the transition map when their pair is unclaimed.
    ACTION_ATTRIBUTE_ALLOW_TURNING = 0x100000, ///< Allows action-controlled orientation updates.
    ACTION_ATTRIBUTE_TRY_CONTINUATION_ON_POSE_MISMATCH =
        0x02000000, ///< Try the requested record's continuation when its initial pose is incompatible.
    ACTION_ATTRIBUTE_PRIORITIZE_CONTINUATION = 0x04000000 ///< Prefer an existing continuation over a pending request.
} ActionAttribute;

#endif
