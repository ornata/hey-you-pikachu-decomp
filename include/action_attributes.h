/// \file
/// Defines recovered action-resource attributes for playback, pose transitions and messenger approach.

#ifndef ACTION_ATTRIBUTES_H
#define ACTION_ATTRIBUTES_H

/// Known fields in the signed 32-bit action attribute word.
/// The playback selector shares MotionPlaybackMode encodings. Other bits remain
/// explicit; these names neither change stored widths nor guarantee a transition.
typedef enum ActionAttribute {
    ACTION_ATTRIBUTE_PLAYBACK_MODE_MASK = 7, ///< Low bits selecting the motion playback policy.
    ACTION_ATTRIBUTE_LOOP = 8,               ///< Permit another cycle unless exit, update or initialization rejects it.
    ACTION_ATTRIBUTE_CONSTRAIN_FACING_TO_TERRAIN =
        0x10, ///< Apply action-facing restrictions from movement collision flags.
    ACTION_ATTRIBUTE_ENABLE_MESSENGER_ORBIT =
        0x10000, ///< Enable orbital steering under messenger contact policies two and three; contact eligibility uses the unoffset target distance.
    ACTION_ATTRIBUTE_SUPPRESS_OPTIONAL_MESSENGER_ORBIT =
        0x8000, ///< Suppress policy-three orbital steering unless ENABLE_MESSENGER_ORBIT is set.
    ACTION_ATTRIBUTE_HIGH_UPWARD_LIMIT =
        0x200, ///< Permit upward world motion up to 30 instead of 5 during motion displacement updates.
    ACTION_ATTRIBUTE_ENABLE_TARGET_FACING =
        0x400000, ///< Enable target-facing child blending unless actor behavior inhibits it.
    ACTION_ATTRIBUTE_ENABLE_CAMERA_FACING =
        0x800000, ///< Enable camera-facing child blending unless actor behavior inhibits it.
    ACTION_ATTRIBUTE_WORLD_MOTION_AIRBORNE =
        0x01000000, ///< Use retained world motion for airborne displacement when ground clearance permits.
    ACTION_ATTRIBUTE_BYPASS_TERRAIN_AND_GRAVITY =
        0x20000000, ///< Bypass terrain/gravity constraints and ground-slope updates.
    ACTION_ATTRIBUTE_METER_POLICY_MASK =
        0xC0000000, ///< Select the action-resource change applied during gameplay model updates.
    ACTION_ATTRIBUTE_SUPPRESS_TRANSITION_CUES =
        0x20000, ///< Suppress cues when a tracked action is loaded as an intermediary transition.
    ACTION_ATTRIBUTE_REGISTER_POSE_TRANSITION =
        0x40000, ///< Offer differing endpoint poses to the transition map when their pair is unclaimed.
    ACTION_ATTRIBUTE_ALLOW_TURNING = 0x100000, ///< Allows action-controlled orientation updates.
    ACTION_ATTRIBUTE_TRY_CONTINUATION_ON_POSE_MISMATCH =
        0x02000000, ///< Try the requested record's continuation when its initial pose is incompatible.
    ACTION_ATTRIBUTE_PRIORITIZE_CONTINUATION = 0x04000000 ///< Prefer an existing continuation over a pending request.
} ActionAttribute;

/// Per-second action-resource policy selected by the action attribute word.
/// The updater applies these signed rates before clamping the resource to 0..100;
/// other explicit resource adjustments remain independent of this policy.
typedef enum ActionMeterPolicy {
    ACTION_METER_POLICY_RECOVER = 0,              ///< Recover one resource unit per second.
    ACTION_METER_POLICY_DRAIN_FAST = 0x40000000,  ///< Spend ten resource units per second.
    ACTION_METER_POLICY_HOLD = 0x80000000,        ///< Leave the resource unchanged by the automatic update.
    ACTION_METER_POLICY_RECOVER_FAST = 0xC0000000 ///< Recover ten resource units per second.
} ActionMeterPolicy;

#endif
