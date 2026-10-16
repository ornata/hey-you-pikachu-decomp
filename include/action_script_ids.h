/// \file
/// Names timed action programs with roles established by their motion definitions.

#ifndef ACTION_SCRIPT_IDS_H
#define ACTION_SCRIPT_IDS_H

/// Known indices in the timed sequencer's program table.
/// These are distinct from the motion-action IDs requested by program commands.
/// Names describe roles supported by retained motion debug labels; this is not
/// an exhaustive list of valid programs or a claim of rendered-animation validation.
/// Fixed cue-kind variants select an authored cue mode, not a sound identifier.
/// Other named programs may inherit the actor default cue mode.
/// Failure names identify authored variants and do not guarantee gameplay outcomes.
typedef enum ActionScriptId {
    ACTION_SCRIPT_NO_PROGRAM = -1,       ///< Select no timed commands while preserving independent motion playback.
    ACTION_SCRIPT_WALK = 0,              ///< Ordinary walking program.
    ACTION_SCRIPT_RUN_ON_ALL_FOURS = 13, ///< Run on all fours.
    ACTION_SCRIPT_TURN_TO_DESIRED_HEADING = 17, ///< Turn toward the requested heading.
    ACTION_SCRIPT_SHIVER_CUE_12 = 44,           ///< Shivering motion with fixed cue-kind selector twelve.
    ACTION_SCRIPT_GREET = 59,                   ///< Greeting with inherited cue selection.
    ACTION_SCRIPT_GREET_CUE_2 = 62,             ///< Greeting with fixed cue-kind selector two.
    ACTION_SCRIPT_GREET_CUE_3 = 63,             ///< Greeting with fixed cue-kind selector three.
    ACTION_SCRIPT_PICK_UP_ITEM = 67,            ///< Item pick-up program.
    ACTION_SCRIPT_DISCARD_HELD_ITEM_VARIANT_2_CUE_0 =
        79,                                   ///< Held-item discard variant two with fixed cue-kind selector zero.
    ACTION_SCRIPT_IDLE_STANCE = 87,           ///< Idle stance program.
    ACTION_SCRIPT_EAT_CUE_0 = 105,            ///< Eating motion with fixed cue-kind selector zero.
    ACTION_SCRIPT_SLEEP_START_CUE_0 = 125,    ///< Begin sleep with fixed cue-kind selector zero.
    ACTION_SCRIPT_WAKE_UP = 128,              ///< Wake from sleep.
    ACTION_SCRIPT_PEACE_SIGN_CUE_0 = 135,     ///< Peace-sign gesture with fixed cue-kind selector zero.
    ACTION_SCRIPT_SCRATCH_HEAD_CUE_10 = 140,  ///< Head-scratching gesture with fixed cue-kind selector ten.
    ACTION_SCRIPT_PROUD_POSE_CUE_2 = 142,     ///< Proud-pose gesture with fixed cue-kind selector two.
    ACTION_SCRIPT_LOOK_AROUND = 168,          ///< Look around the actor's surroundings.
    ACTION_SCRIPT_VOICE_TOO_QUIET = 173,      ///< Authored quiet-voice response with fixed cue-kind selector eleven.
    ACTION_SCRIPT_THUNDER = 212,              ///< Thunder timed program.
    ACTION_SCRIPT_THUNDER_FAILURE = 213,      ///< Thunder timed program failure variant.
    ACTION_SCRIPT_THUNDERBOLT = 214,          ///< Thunderbolt timed program.
    ACTION_SCRIPT_THUNDERBOLT_FAILURE = 215,  ///< Thunderbolt timed program failure variant.
    ACTION_SCRIPT_THUNDERSHOCK = 216,         ///< Thundershock timed program.
    ACTION_SCRIPT_THUNDERSHOCK_FAILURE = 217, ///< Thundershock timed program failure variant.
    ACTION_SCRIPT_SALUTE_CUE_0 = 221,         ///< Saluting gesture with fixed cue-kind selector zero.
    ACTION_SCRIPT_NOD_CUE_3 = 235,            ///< Nodding gesture with fixed cue-kind selector three.
    ACTION_SCRIPT_VOICE_TOO_LOUD = 236,       ///< Authored loud-voice response with fixed cue-kind selector eleven.
    ACTION_SCRIPT_SIT_DOWN = 273,             ///< Begin sitting down.
    ACTION_SCRIPT_SITTING = 276,              ///< Continue the sitting posture.
    ACTION_SCRIPT_SITTING_SCRATCH_EAR = 277,  ///< Ear-scratching motion from the sitting posture.
    ACTION_SCRIPT_FACE_DESTINATION_UNDERGROUND = 358, ///< Face the destination while underground.
    ACTION_SCRIPT_MOVE_TO_DESTINATION = 365           ///< Move toward the destination.
} ActionScriptId;

#endif
