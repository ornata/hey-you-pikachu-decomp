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
/// Response and sequence names describe requested resources, not live emotions or
/// guaranteed transitions; authored command times are not wall-clock promises.
typedef enum ActionScriptId {
    ACTION_SCRIPT_NO_PROGRAM = -1,           ///< Select no timed commands while preserving independent motion playback.
    ACTION_SCRIPT_WALK = 0,                  ///< Ordinary walking program.
    ACTION_SCRIPT_WALK_START_CUE_0 = 1,      ///< Requests walk start with fixed cue-kind selector 0.
    ACTION_SCRIPT_WALK_START_CUE_2 = 2,      ///< Requests walk start with fixed cue-kind selector 2.
    ACTION_SCRIPT_WALK_START_CUE_3 = 3,      ///< Requests walk start with fixed cue-kind selector 3.
    ACTION_SCRIPT_WALK_START_CUE_4 = 4,      ///< Requests walk start with fixed cue-kind selector 4.
    ACTION_SCRIPT_WALK_START_CUE_6 = 5,      ///< Requests walk start with fixed cue-kind selector 6.
    ACTION_SCRIPT_WALK_START_CUE_7 = 6,      ///< Requests walk start with fixed cue-kind selector 7.
    ACTION_SCRIPT_WALK_START_CUE_8 = 7,      ///< Requests walk start with fixed cue-kind selector 8.
    ACTION_SCRIPT_WALK_START_CUE_9 = 8,      ///< Requests walk start with fixed cue-kind selector 9.
    ACTION_SCRIPT_WALK_START_CUE_10 = 9,     ///< Requests walk start with fixed cue-kind selector 10.
    ACTION_SCRIPT_WALK_START_CUE_11 = 10,    ///< Requests walk start with fixed cue-kind selector 11.
    ACTION_SCRIPT_WALK_START_CUE_12 = 11,    ///< Requests walk start with fixed cue-kind selector 12.
    ACTION_SCRIPT_WALK_START_CUE_13 = 12,    ///< Requests walk start with fixed cue-kind selector 13.
    ACTION_SCRIPT_RUN_ON_ALL_FOURS = 13,     ///< Run on all fours.
    ACTION_SCRIPT_WALK_TO_TARGET_START = 14, ///< Requests walk to target start with inherited cue selection.
    ACTION_SCRIPT_RUN_ON_ALL_FOURS_TO_TARGET_START =
        15, ///< Requests run on all fours to target start with inherited cue selection.
    ACTION_SCRIPT_RUN_ON_ALL_FOURS_TO_TARGET_FAST_START =
        16, ///< Requests run on all fours to target fast start with inherited cue selection.
    ACTION_SCRIPT_TURN_TO_DESIRED_HEADING = 17, ///< Turn toward the requested heading.
    ACTION_SCRIPT_JUMP_TURN_TOWARD_TARGET = 18, ///< Jumping target turn with inherited cue selection.
    ACTION_SCRIPT_JUMP_TURN_TOWARD_TARGET_POSITION_ADJUSTMENT =
        19, ///< Requests jump turn toward target position adjustment with inherited cue selection.
    ACTION_SCRIPT_ANGER_RESPONSE_LARGE_CUE_3 =
        20, ///< Requests authored anger response large with fixed cue-kind selector 3.
    ACTION_SCRIPT_ANGER_RESPONSE_LARGE_CUE_6 =
        21, ///< Requests authored anger response large with fixed cue-kind selector 6.
    ACTION_SCRIPT_ANGER_RESPONSE_MEDIUM_CUE_0 =
        22, ///< Requests authored anger response medium with fixed cue-kind selector 0.
    ACTION_SCRIPT_JOY_RESPONSE_SMALL_CUE_0 =
        23, ///< Requests authored joy response small with fixed cue-kind selector 0.
    ACTION_SCRIPT_ANGER_RESPONSE_MEDIUM_CUE_4 =
        24, ///< Requests authored anger response medium with fixed cue-kind selector 4.
    ACTION_SCRIPT_ANGER_RESPONSE_MEDIUM_CUE_6 =
        25, ///< Requests authored anger response medium with fixed cue-kind selector 6.
    ACTION_SCRIPT_ANGER_RESPONSE_MEDIUM_CUE_11 =
        26, ///< Requests authored anger response medium with fixed cue-kind selector 11.
    ACTION_SCRIPT_ANGER_RESPONSE_MEDIUM_CUE_13 =
        27,                           ///< Requests authored anger response medium with fixed cue-kind selector 13.
    ACTION_SCRIPT_STEP_IN_PLACE = 28, ///< Requests step in place with inherited cue selection.
    ACTION_SCRIPT_SWING_FOOT = 29,    ///< Requests swing foot with inherited cue selection.
    ACTION_SCRIPT_BALLET_CUE_3 = 30,  ///< Requests ballet with fixed cue-kind selector 3.
    ACTION_SCRIPT_SURPRISE_RESPONSE_CUE_9 = 31, ///< Requests authored surprise response with fixed cue-kind selector 9.
    ACTION_SCRIPT_SURPRISE_RESPONSE_CUE_12 =
        32,                                    ///< Requests authored surprise response with fixed cue-kind selector 12.
    ACTION_SCRIPT_SULKING_RESPONSE_CUE_7 = 35, ///< Requests authored sulking response with fixed cue-kind selector 7.
    ACTION_SCRIPT_GOODBYE_GESTURE_CUE_2 = 36,  ///< Requests goodbye gesture with fixed cue-kind selector 2.
    ACTION_SCRIPT_GOODBYE_GESTURE_CUE_3 = 37,  ///< Requests goodbye gesture with fixed cue-kind selector 3.
    ACTION_SCRIPT_GOODBYE_GESTURE_CUE_4 = 38,  ///< Requests goodbye gesture with fixed cue-kind selector 4.
    ACTION_SCRIPT_GOODBYE_GESTURE_CUE_7 = 39,  ///< Requests goodbye gesture with fixed cue-kind selector 7.
    ACTION_SCRIPT_HEAD_TURN = 40,              ///< Requests head turn with inherited cue selection.
    ACTION_SCRIPT_HEAD_LOOK_LEFT = 41,         ///< Requests head look left with inherited cue selection.
    ACTION_SCRIPT_HEAD_LOOK_RIGHT = 42,        ///< Requests head look right with inherited cue selection.
    ACTION_SCRIPT_SHIVER_CUE_9 = 43,           ///< Requests shiver with fixed cue-kind selector 9.
    ACTION_SCRIPT_SHIVER_CUE_12 = 44,          ///< Shivering motion with fixed cue-kind selector twelve.
    ACTION_SCRIPT_SHIVER_CUE_13 = 45,          ///< Shivering gesture with cue selector 13.
    ACTION_SCRIPT_SHIVER_CUE_11 = 46,          ///< Shivering gesture with cue selector 11.
    ACTION_SCRIPT_DISAPPOINTMENT_RESPONSE_VARIANT_M_CUE_7 =
        47, ///< Requests authored disappointment response variant m with fixed cue-kind selector 7.
    ACTION_SCRIPT_DISAPPOINTMENT_RESPONSE_VARIANT_M_CUE_12 =
        48, ///< Requests authored disappointment response variant m with fixed cue-kind selector 12.
    ACTION_SCRIPT_DISAPPOINTMENT_RESPONSE_VARIANT_M_CUE_13 =
        49, ///< Requests authored disappointment response variant m with fixed cue-kind selector 13.
    ACTION_SCRIPT_JOY_RESPONSE_MEDIUM_CUE_2 =
        50, ///< Requests authored joy response medium with fixed cue-kind selector 2.
    ACTION_SCRIPT_JOY_RESPONSE_SMALL_CUE_2 =
        51, ///< Requests authored joy response small with fixed cue-kind selector 2.
    ACTION_SCRIPT_JOY_RESPONSE_SMALL_CUE_4 =
        52, ///< Requests authored joy response small with fixed cue-kind selector 4.
    ACTION_SCRIPT_JOY_RESPONSE_SMALL_CUE_13 =
        53, ///< Requests authored joy response small with fixed cue-kind selector 13.
    ACTION_SCRIPT_CALLING_RESPONSE_CUE_0 = 54, ///< Requests authored calling response with fixed cue-kind selector 0.
    ACTION_SCRIPT_JOY_RESPONSE_SMALL_CUE_9 =
        55, ///< Requests authored joy response small with fixed cue-kind selector 9.
    ACTION_SCRIPT_JOY_RESPONSE_SMALL_CUE_12 =
        56,                                  ///< Requests authored joy response small with fixed cue-kind selector 12.
    ACTION_SCRIPT_JOY_RESPONSE_SMALL = 57,   ///< Requests authored joy response small with inherited cue selection.
    ACTION_SCRIPT_CALLING_RESPONSE = 58,     ///< Requests authored calling response with inherited cue selection.
    ACTION_SCRIPT_GREET = 59,                ///< Greeting with inherited cue selection.
    ACTION_SCRIPT_GREET_CUE_0 = 60,          ///< Requests greet with fixed cue-kind selector 0.
    ACTION_SCRIPT_GREET_CUE_1 = 61,          ///< Requests greet with fixed cue-kind selector 1.
    ACTION_SCRIPT_GREET_CUE_2 = 62,          ///< Greeting with fixed cue-kind selector two.
    ACTION_SCRIPT_GREET_CUE_3 = 63,          ///< Greeting with fixed cue-kind selector three.
    ACTION_SCRIPT_GREET_CUE_4 = 64,          ///< Requests greet with fixed cue-kind selector 4.
    ACTION_SCRIPT_GREET_CUE_7 = 65,          ///< Requests greet with fixed cue-kind selector 7.
    ACTION_SCRIPT_GREET_CUE_10 = 66,         ///< Requests greet with fixed cue-kind selector 10.
    ACTION_SCRIPT_PICK_UP_ITEM = 67,         ///< Item pick-up program.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_0 = 68,   ///< Requests pick up item with fixed cue-kind selector 0.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_1 = 69,   ///< Requests pick up item with fixed cue-kind selector 1.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_2 = 70,   ///< Requests pick up item with fixed cue-kind selector 2.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_3 = 71,   ///< Requests pick up item with fixed cue-kind selector 3.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_4 = 72,   ///< Requests pick up item with fixed cue-kind selector 4.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_7 = 73,   ///< Requests pick up item with fixed cue-kind selector 7.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_11 = 74,  ///< Requests pick up item with fixed cue-kind selector 11.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_6 = 75,   ///< Requests pick up item with fixed cue-kind selector 6.
    ACTION_SCRIPT_PICK_UP_ITEM_CUE_14 = 76,  ///< Requests pick up item with fixed cue-kind selector 14.
    ACTION_SCRIPT_PRETEND_PICK_UP_ITEM = 77, ///< Requests pretend pick up item with inherited cue selection.
    ACTION_SCRIPT_DISCARD_HELD_ITEM_VARIANT_2_CUE_11 =
        78, ///< Requests discard held item variant 2 with fixed cue-kind selector 11.
    ACTION_SCRIPT_DISCARD_HELD_ITEM_VARIANT_2_CUE_0 =
        79, ///< Held-item discard variant two with fixed cue-kind selector zero.
    ACTION_SCRIPT_DISCARD_HELD_ITEM_VARIANT_2_CUE_2 =
        80, ///< Requests discard held item variant 2 with fixed cue-kind selector 2.
    ACTION_SCRIPT_DISCARD_HELD_ITEM_VARIANT_2_CUE_17 =
        81,                                ///< Requests discard held item variant 2 with fixed cue-kind selector 17.
    ACTION_SCRIPT_JUMP_50_CM = 85,         ///< Requests jump 50 cm with inherited cue selection.
    ACTION_SCRIPT_FORWARD_JUMP_START = 86, ///< Requests forward jump start with inherited cue selection.
    ACTION_SCRIPT_IDLE_STANCE = 87,        ///< Idle stance program.
    ACTION_SCRIPT_IDLE_STANCE_ENDING_VARIANT =
        88,                                   ///< Requests idle stance ending variant with inherited cue selection.
    ACTION_SCRIPT_SIDE_SWAY_STANCE = 89,      ///< Requests side sway stance with inherited cue selection.
    ACTION_SCRIPT_TILT_QUESTION_VARIANT = 90, ///< Requests tilt question variant with inherited cue selection.
    ACTION_SCRIPT_FORWARD_EXTENSION_START_CUE_4 =
        91, ///< Requests forward extension start with fixed cue-kind selector 4.
    ACTION_SCRIPT_FORWARD_EXTENSION_START_CUE_2 =
        92, ///< Requests forward extension start with fixed cue-kind selector 2.
    ACTION_SCRIPT_FORWARD_EXTENSION_START_CUE_13 =
        93, ///< Requests forward extension start with fixed cue-kind selector 13.
    ACTION_SCRIPT_FORWARD_EXTENSION_LOOP_ENTRY_CUE_9 =
        94,                                 ///< Requests forward extension loop entry with fixed cue-kind selector 9.
    ACTION_SCRIPT_LOOK_UP_START = 95,       ///< Requests look up start with inherited cue selection.
    ACTION_SCRIPT_LOOK_UP_SHORT_START = 96, ///< Requests look up short start with inherited cue selection.
    ACTION_SCRIPT_LOOK_UP_SHORT_START_CUE_0 = 97, ///< Requests look up short start with fixed cue-kind selector 0.
    ACTION_SCRIPT_FIDGETING_RESPONSE_CUE_1 =
        98, ///< Requests authored fidgeting response with fixed cue-kind selector 1.
    ACTION_SCRIPT_FIDGETING_RESPONSE_CUE_2 =
        99, ///< Requests authored fidgeting response with fixed cue-kind selector 2.
    ACTION_SCRIPT_FIDGETING_RESPONSE_CUE_3 =
        100, ///< Requests authored fidgeting response with fixed cue-kind selector 3.
    ACTION_SCRIPT_FIDGETING_RESPONSE_CUE_4 =
        101, ///< Requests authored fidgeting response with fixed cue-kind selector 4.
    ACTION_SCRIPT_FIDGETING_RESPONSE_CUE_8 =
        102, ///< Requests authored fidgeting response with fixed cue-kind selector 8.
    ACTION_SCRIPT_FIDGETING_RESPONSE_CUE_10 =
        103, ///< Requests authored fidgeting response with fixed cue-kind selector 10.
    ACTION_SCRIPT_FIDGETING_RESPONSE_CUE_7 =
        104,                        ///< Requests authored fidgeting response with fixed cue-kind selector 7.
    ACTION_SCRIPT_EAT_CUE_0 = 105,  ///< Eating motion with fixed cue-kind selector zero.
    ACTION_SCRIPT_EAT_CUE_17 = 106, ///< Eating motion with cue selector 17.
    ACTION_SCRIPT_EAT_CUE_2 = 107,  ///< Eating motion with cue selector 2.
    ACTION_SCRIPT_TURN_AWAY_RESPONSE_CUE_6 =
        108, ///< Requests authored turn away response with fixed cue-kind selector 6.
    ACTION_SCRIPT_TURN_AWAY_RESPONSE_CUE_5 =
        109, ///< Requests authored turn away response with fixed cue-kind selector 5.
    ACTION_SCRIPT_IGNORING_RESPONSE_CUE_6 =
        111,                                  ///< Requests authored ignoring response with fixed cue-kind selector 6.
    ACTION_SCRIPT_THROW_ITEM = 112,           ///< Item-throwing gesture with inherited cue selection.
    ACTION_SCRIPT_THROW_ITEM_GENTLY = 113,    ///< Gentle item-throwing variant with inherited cue selection.
    ACTION_SCRIPT_THROW_ITEM_TO_PLAYER = 114, ///< Requests throw item to player with inherited cue selection.
    ACTION_SCRIPT_THROW_ITEM_TO_TARGET = 115, ///< Requests throw item to target with inherited cue selection.
    ACTION_SCRIPT_DISCARD_HELD_ITEM_VARIANT_2_CUE_6 =
        116, ///< Requests discard held item variant 2 with fixed cue-kind selector 6.
    ACTION_SCRIPT_USE_RAISED_HELD_ITEM_ATTACHMENT_CUE_0 =
        117,                              ///< Request raised held-item attachment handling with fixed cue selector 0.
    ACTION_SCRIPT_LOOK_UP = 118,          ///< Requests look up with inherited cue selection.
    ACTION_SCRIPT_LOOK_UP_CUE_0 = 119,    ///< Requests look up with fixed cue-kind selector 0.
    ACTION_SCRIPT_REFUSAL_RESPONSE = 120, ///< Requests authored refusal response with inherited cue selection.
    ACTION_SCRIPT_REFUSAL_RESPONSE_CUE_11 =
        121,                                    ///< Requests authored refusal response with fixed cue-kind selector 11.
    ACTION_SCRIPT_REFUSAL_RESPONSE_CUE_4 = 122, ///< Requests authored refusal response with fixed cue-kind selector 4.
    ACTION_SCRIPT_TAP_HEAD_CUE_10 = 123,        ///< Requests tap head with fixed cue-kind selector 10.
    ACTION_SCRIPT_SLEEP_START_CUE_11 = 124,     ///< Requests sleep start with fixed cue-kind selector 11.
    ACTION_SCRIPT_SLEEP_START_CUE_0 = 125,      ///< Begin sleep with fixed cue-kind selector zero.
    ACTION_SCRIPT_SLEEP_START_CUE_3 = 126,      ///< Beginning of the sleep sequence with cue selector 3.
    ACTION_SCRIPT_SLEEP_LOOP_CUE_11 = 127,      ///< Requests sleep loop with fixed cue-kind selector 11.
    ACTION_SCRIPT_WAKE_UP = 128,                ///< Wake from sleep.
    ACTION_SCRIPT_WAKE_UP_CUE_0 = 129,          ///< Requests wake up with fixed cue-kind selector 0.
    ACTION_SCRIPT_WAKE_UP_CUE_2 = 130,          ///< Requests wake up with fixed cue-kind selector 2.
    ACTION_SCRIPT_WAKE_UP_CUE_11 = 131,         ///< Requests wake up with fixed cue-kind selector 11.
    ACTION_SCRIPT_WAKE_TO_SITTING = 132,        ///< Requests wake to sitting with inherited cue selection.
    ACTION_SCRIPT_PUT_DOWN = 133,               ///< Putting-down gesture with inherited cue selection.
    ACTION_SCRIPT_PUSH_START = 134,             ///< Requests push start with inherited cue selection.
    ACTION_SCRIPT_PEACE_SIGN_CUE_0 = 135,       ///< Peace-sign gesture with fixed cue-kind selector zero.
    ACTION_SCRIPT_PEACE_SIGN_CUE_1 = 136,       ///< Requests peace sign with fixed cue-kind selector 1.
    ACTION_SCRIPT_PEACE_SIGN_CUE_2 = 137,       ///< Requests peace sign with fixed cue-kind selector 2.
    ACTION_SCRIPT_PEACE_SIGN_CUE_3 = 138,       ///< Requests peace sign with fixed cue-kind selector 3.
    ACTION_SCRIPT_FROG_JUMP = 139,              ///< Requests frog jump with inherited cue selection.
    ACTION_SCRIPT_SCRATCH_HEAD_CUE_10 = 140,    ///< Head-scratching gesture with fixed cue-kind selector ten.
    ACTION_SCRIPT_SCRATCH_HEAD_CUE_0 = 141,     ///< Requests scratch head with fixed cue-kind selector 0.
    ACTION_SCRIPT_PROUD_POSE_CUE_2 = 142,       ///< Proud-pose gesture with fixed cue-kind selector two.
    ACTION_SCRIPT_GROUND_SPIN_WITH_LIFT_CUE_3 = 144, ///< Requests ground spin with lift with fixed cue-kind selector 3.
    ACTION_SCRIPT_GROUND_SPIN_WITH_LIFT_CUE_2 = 145, ///< Requests ground spin with lift with fixed cue-kind selector 2.
    ACTION_SCRIPT_GROUND_SPIN_WITH_LIFT_CUE_1 = 146, ///< Requests ground spin with lift with fixed cue-kind selector 1.
    ACTION_SCRIPT_GROUND_SPIN_WITH_LIFT_CUE_0 = 147, ///< Requests ground spin with lift with fixed cue-kind selector 0.
    ACTION_SCRIPT_POKE = 148,                        ///< Requests poke with inherited cue selection.
    ACTION_SCRIPT_TURN_ON_TV = 149,                  ///< Requests turn on tv with inherited cue selection.
    ACTION_SCRIPT_LAUGH_MEDIUM_VARIANT_CUE_2 = 150,  ///< Medium-laugh variant with cue selector 2.
    ACTION_SCRIPT_LAUGH_SMALL_VARIANT_CUE_1 = 151,   ///< Small-laugh variant with cue selector 1.
    ACTION_SCRIPT_LAUGH_SMALL_VARIANT_CUE_4 = 152,   ///< Small-laugh variant with cue selector 4.
    ACTION_SCRIPT_TILT_RIGHT_LEFT = 153,             ///< Requests tilt right left with inherited cue selection.
    ACTION_SCRIPT_TILT_RIGHT_LEFT_CUE_3 = 154,       ///< Requests tilt right left with fixed cue-kind selector 3.
    ACTION_SCRIPT_AFFIRMATIVE_RESPONSE = 155, ///< Requests authored affirmative response with inherited cue selection.
    ACTION_SCRIPT_TILT_QUESTION_VARIANT_CUE_13 =
        156, ///< Requests tilt question variant with fixed cue-kind selector 13.
    ACTION_SCRIPT_TILT_QUESTION_VARIANT_CUE_6 = 157, ///< Requests tilt question variant with fixed cue-kind selector 6.
    ACTION_SCRIPT_TILT_QUESTION_VARIANT_CUE_8 = 158, ///< Requests tilt question variant with fixed cue-kind selector 8.
    ACTION_SCRIPT_TILT_QUESTION_VARIANT_CUE_11 = 159, ///< Question-labelled tilting variant with cue selector 11.
    ACTION_SCRIPT_TILT_QUESTION_VARIANT_CUE_2 = 160, ///< Requests tilt question variant with fixed cue-kind selector 2.
    ACTION_SCRIPT_TILT_QUESTION_VARIANT_CUE_3 = 161, ///< Requests tilt question variant with fixed cue-kind selector 3.
    ACTION_SCRIPT_TILT_VARIANT_2_CUE_3 = 162,        ///< Requests tilt variant 2 with fixed cue-kind selector 3.
    ACTION_SCRIPT_TILT_VARIANT_2_CUE_2 = 163,        ///< Second tilting variant with cue selector 2.
    ACTION_SCRIPT_TILT_VARIANT_2_CUE_1 = 164,        ///< Requests tilt variant 2 with fixed cue-kind selector 1.
    ACTION_SCRIPT_TILT_VARIANT_2_CUE_7 = 165,        ///< Requests tilt variant 2 with fixed cue-kind selector 7.
    ACTION_SCRIPT_SLOW_TARGET_APPROACH_WALK_CUE_7 =
        166, ///< Requests slow target approach walk with fixed cue-kind selector 7.
    ACTION_SCRIPT_SLOW_TARGET_APPROACH_THEN_SLEEP =
        167, ///< Requests a slow target approach with cue 7, followed by sleep start with cue 0.
    ACTION_SCRIPT_LOOK_AROUND = 168, ///< Look around the actor's surroundings.
    ACTION_SCRIPT_ITEM_REQUEST_RESPONSE_CUE_13 =
        169, ///< Requests authored item request response with fixed cue-kind selector 13.
    ACTION_SCRIPT_ITEM_REQUEST_RESPONSE_CUE_2 =
        170, ///< Requests authored item request response with fixed cue-kind selector 2.
    ACTION_SCRIPT_ITEM_REQUEST_RESPONSE_CUE_9 =
        171, ///< Requests authored item request response with fixed cue-kind selector 9.
    ACTION_SCRIPT_VOICE_TOO_LOUD_CUE_11 = 172, ///< Requests voice too loud with fixed cue-kind selector 11.
    ACTION_SCRIPT_VOICE_TOO_QUIET = 173,       ///< Authored quiet-voice response with fixed cue-kind selector eleven.
    ACTION_SCRIPT_HEAD_SHAKE = 176,            ///< Requests head shake with inherited cue selection.
    ACTION_SCRIPT_JOY_HEAD_SHAKE_RESPONSE_CUE_3 =
        177,                              ///< Requests authored joy head shake response with fixed cue-kind selector 3.
    ACTION_SCRIPT_SHAKE_HIPS = 178,       ///< Requests shake hips with inherited cue selection.
    ACTION_SCRIPT_STANCE_VARIANT_L = 179, ///< Requests authored stance variant l with inherited cue selection.
    ACTION_SCRIPT_WALK_TO_TRIP_CUE_0 = 180,       ///< Requests walk to trip with fixed cue-kind selector 0.
    ACTION_SCRIPT_WALK_TO_TRIP_CUE_9 = 181,       ///< Requests walk to trip with fixed cue-kind selector 9.
    ACTION_SCRIPT_WALK_TO_TRIP_CUE_13 = 182,      ///< Requests walk to trip with fixed cue-kind selector 13.
    ACTION_SCRIPT_TRIP_LOOP_CUE_9 = 183,          ///< Requests trip loop with fixed cue-kind selector 9.
    ACTION_SCRIPT_TURN_AWAY_START_CUE_6 = 184,    ///< Requests turn away start with fixed cue-kind selector 6.
    ACTION_SCRIPT_WATERING_CAN_CUE_1 = 185,       ///< Requests watering can with fixed cue-kind selector 1.
    ACTION_SCRIPT_WATERING_CAN_CUE_0 = 186,       ///< Requests watering can with fixed cue-kind selector 0.
    ACTION_SCRIPT_TODDLE_TO_TARGET_CUE_11 = 187,  ///< Requests toddle to target with fixed cue-kind selector 11.
    ACTION_SCRIPT_TODDLE_TO_TARGET_CUE_0 = 188,   ///< Requests toddle to target with fixed cue-kind selector 0.
    ACTION_SCRIPT_TODDLE_TO_TARGET_CUE_14 = 189,  ///< Requests toddle to target with fixed cue-kind selector 14.
    ACTION_SCRIPT_TODDLE_TO_TARGET_CUE_13 = 190,  ///< Requests toddle to target with fixed cue-kind selector 13.
    ACTION_SCRIPT_STEP_BACK = 191,                ///< Requests step back with inherited cue selection.
    ACTION_SCRIPT_STEP_BACK_FAST = 192,           ///< Requests step back fast with inherited cue selection.
    ACTION_SCRIPT_LEAN_FORWARD_CUE_0 = 193,       ///< Requests lean forward with fixed cue-kind selector 0.
    ACTION_SCRIPT_HAND_OVER_START = 194,          ///< Requests hand over start with inherited cue selection.
    ACTION_SCRIPT_HAND_OVER_START_CUE_13 = 195,   ///< Requests hand over start with fixed cue-kind selector 13.
    ACTION_SCRIPT_HAND_OVER_START_CUE_4 = 196,    ///< Requests hand over start with fixed cue-kind selector 4.
    ACTION_SCRIPT_HAND_OVER_START_CUE_0 = 197,    ///< Requests hand over start with fixed cue-kind selector 0.
    ACTION_SCRIPT_HAND_OVER_START_CUE_1 = 198,    ///< Requests hand over start with fixed cue-kind selector 1.
    ACTION_SCRIPT_HOLD_UP_START_CUE_2 = 199,      ///< Requests hold up start with fixed cue-kind selector 2.
    ACTION_SCRIPT_HOLD_UP_HIGH_START_CUE_2 = 200, ///< Requests hold up high start with fixed cue-kind selector 2.
    ACTION_SCRIPT_MISS_RESPONSE_CUE_4 = 201,      ///< Requests authored miss response with fixed cue-kind selector 4.
    ACTION_SCRIPT_FOOT_TAP_START_CUE_4 = 202,     ///< Requests foot tap start with fixed cue-kind selector 4.
    ACTION_SCRIPT_FOOT_TAP_START_CUE_13 = 203,    ///< Requests foot tap start with fixed cue-kind selector 13.
    ACTION_SCRIPT_FLUSTERED_RESPONSE_CUE_9 =
        204,                                ///< Requests authored flustered response with fixed cue-kind selector 9.
    ACTION_SCRIPT_FLUSTERED_RESPONSE = 205, ///< Requests authored flustered response with inherited cue selection.
    ACTION_SCRIPT_HELD_ITEM_PRESENTATION_START_CUE_0 =
        207, ///< Requests held-item presentation start with cue 0; sequencing has an authored 1.1-second terminal time.
    ACTION_SCRIPT_HELD_ITEM_PRESENTATION_END_CUE_0 =
        208,                           ///< Requests authored held item presentation end with fixed cue-kind selector 0.
    ACTION_SCRIPT_SMELL_GESTURE = 209, ///< Smelling gesture with inherited cue selection.
    ACTION_SCRIPT_SMELL_GESTURE_CUE_0 = 210,  ///< Smelling gesture with cue selector 0.
    ACTION_SCRIPT_SNEEZE_CUE_0 = 211,         ///< Sneezing gesture with cue selector 0.
    ACTION_SCRIPT_THUNDER = 212,              ///< Thunder timed program.
    ACTION_SCRIPT_THUNDER_FAILURE = 213,      ///< Thunder timed program failure variant.
    ACTION_SCRIPT_THUNDERBOLT = 214,          ///< Thunderbolt timed program.
    ACTION_SCRIPT_THUNDERBOLT_FAILURE = 215,  ///< Thunderbolt timed program failure variant.
    ACTION_SCRIPT_THUNDERSHOCK = 216,         ///< Thundershock timed program.
    ACTION_SCRIPT_THUNDERSHOCK_FAILURE = 217, ///< Thundershock timed program failure variant.
    ACTION_SCRIPT_QUESTIONING_RESPONSE_CUE_7 =
        218, ///< Requests authored questioning response with fixed cue-kind selector 7.
    ACTION_SCRIPT_QUESTIONING_RESPONSE_CUE_0 =
        219, ///< Requests authored questioning response with fixed cue-kind selector 0.
    ACTION_SCRIPT_QUESTIONING_RESPONSE_CUE_9 =
        220,                          ///< Requests authored questioning response with fixed cue-kind selector 9.
    ACTION_SCRIPT_SALUTE_CUE_0 = 221, ///< Saluting gesture with fixed cue-kind selector zero.
    ACTION_SCRIPT_PULL_OUT_START_CUE_0 = 222, ///< Requests pull out start with fixed cue-kind selector 0.
    ACTION_SCRIPT_PULL_OUT_SUCCESS_VARIANT_CUE_0 =
        223, ///< Requests pull out success variant with fixed cue-kind selector 0.
    ACTION_SCRIPT_PULL_OUT_FAILURE_VARIANT_CUE_0 =
        224, ///< Requests pull out failure variant with fixed cue-kind selector 0.
    ACTION_SCRIPT_PULL_TREASURE_CHEST_START_CUE_0 =
        225,                                 ///< Requests pull treasure chest start with fixed cue-kind selector 0.
    ACTION_SCRIPT_FISHING_START_CUE_0 = 226, ///< Requests fishing start with fixed cue-kind selector 0.
    ACTION_SCRIPT_FISHING_CATCH_VARIANT_CUE_0 = 227, ///< Requests fishing catch variant with fixed cue-kind selector 0.
    ACTION_SCRIPT_FISHING_CAST = 228,                ///< Requests fishing cast with inherited cue selection.
    ACTION_SCRIPT_COUGH_CUE_0 = 231,                 ///< Coughing gesture with cue selector 0.
    ACTION_SCRIPT_NOD = 232,                         ///< Requests nod with inherited cue selection.
    ACTION_SCRIPT_NOD_CUE_1 = 233,                   ///< Nodding gesture with cue selector 1.
    ACTION_SCRIPT_NOD_CUE_2 = 234,                   ///< Requests nod with fixed cue-kind selector 2.
    ACTION_SCRIPT_NOD_CUE_3 = 235,                   ///< Nodding gesture with fixed cue-kind selector three.
    ACTION_SCRIPT_VOICE_TOO_LOUD = 236, ///< Authored loud-voice response with fixed cue-kind selector eleven.
    ACTION_SCRIPT_PONDERING_RESPONSE_CUE_0 =
        237, ///< Requests authored pondering response with fixed cue-kind selector 0.
    ACTION_SCRIPT_PONDERING_RESPONSE_CUE_1 =
        238, ///< Requests authored pondering response with fixed cue-kind selector 1.
    ACTION_SCRIPT_DRAW_PICTURE_START_CUE_0 = 240,   ///< Requests draw picture start with fixed cue-kind selector 0.
    ACTION_SCRIPT_DRAW_PICTURE_LOOK_UP_CUE_0 = 241, ///< Requests draw picture look up with fixed cue-kind selector 0.
    ACTION_SCRIPT_DRAW_PICTURE_LOOK_UP_CUE_2 = 242, ///< Requests draw picture look up with fixed cue-kind selector 2.
    ACTION_SCRIPT_DRAW_PICTURE_END_STEP_BACK_CUE_0 =
        243,                                    ///< Requests draw picture end step back with fixed cue-kind selector 0.
    ACTION_SCRIPT_LETS_GO_RESPONSE_CUE_2 = 244, ///< Requests authored lets go response with fixed cue-kind selector 2.
    ACTION_SCRIPT_LETS_GO_RESPONSE_CUE_11 =
        245, ///< Requests authored lets go response with fixed cue-kind selector 11.
    ACTION_SCRIPT_FORWARD_THROW_RESPONSE =
        246, ///< Request the authored forward-throw response with the default cue selector.
    ACTION_SCRIPT_FORWARD_THROW_RESPONSE_CUE_11 =
        247, ///< Request the authored forward-throw response with fixed cue selector 11.
    ACTION_SCRIPT_FORWARD_THROW_RESPONSE_CUE_0 =
        248, ///< Request the authored forward-throw response with fixed cue selector 0.
    ACTION_SCRIPT_DROWNING_START_CUE_9 = 249, ///< Requests drowning start with fixed cue-kind selector 9.
    ACTION_SCRIPT_DROWNING_LOOP_CUE_9 = 250,  ///< Requests drowning loop with fixed cue-kind selector 9.
    ACTION_SCRIPT_COUNTING_GESTURE = 251,     ///< Requests counting gesture with inherited cue selection.
    ACTION_SCRIPT_DIZZINESS_RESPONSE_CUE_14 =
        252, ///< Requests authored dizziness response with fixed cue-kind selector 14.
    ACTION_SCRIPT_SIT_CROSS_LEGGED_CUE_1 = 253, ///< Requests sit cross legged with fixed cue-kind selector 1.
    ACTION_SCRIPT_CAUTION_RESPONSE_CUE_1 = 254, ///< Requests authored caution response with fixed cue-kind selector 1.
    ACTION_SCRIPT_CAUTION_RESPONSE_CUE_2 = 255, ///< Requests authored caution response with fixed cue-kind selector 2.
    ACTION_SCRIPT_CAUTION_RESPONSE_CUE_0 = 256, ///< Requests authored caution response with fixed cue-kind selector 0.
    ACTION_SCRIPT_UNSTEADY_RESPONSE_CUE_1 =
        257, ///< Requests authored unsteady response with fixed cue-kind selector 1.
    ACTION_SCRIPT_UNSTEADY_RESPONSE_CUE_3 =
        258,                       ///< Requests authored unsteady response with fixed cue-kind selector 3.
    ACTION_SCRIPT_HOP_CUE_2 = 259, ///< Requests hop with fixed cue-kind selector 2.
    ACTION_SCRIPT_HOP_CUE_4 = 260, ///< Requests hop with fixed cue-kind selector 4.
    ACTION_SCRIPT_MOCKING_RESPONSE_CUE_11 =
        261,                                    ///< Requests authored mocking response with fixed cue-kind selector 11.
    ACTION_SCRIPT_MOCKING_RESPONSE_CUE_8 = 262, ///< Requests authored mocking response with fixed cue-kind selector 8.
    ACTION_SCRIPT_HEAD_DOWN_START_CUE_7 = 264,  ///< Requests head down start with fixed cue-kind selector 7.
    ACTION_SCRIPT_COUNTING_GESTURE_CUE_1 = 266, ///< Requests counting gesture with fixed cue-kind selector 1.
    ACTION_SCRIPT_EXASPERATION_RESPONSE_CUE_0 =
        267, ///< Requests authored exasperation response with fixed cue-kind selector 0.
    ACTION_SCRIPT_EXASPERATION_RESPONSE_CUE_8 =
        268,                           ///< Requests authored exasperation response with fixed cue-kind selector 8.
    ACTION_SCRIPT_HARMONICA = 269,     ///< Harmonica gesture with inherited cue selection.
    ACTION_SCRIPT_TRUMPET = 270,       ///< Trumpet gesture with inherited cue selection.
    ACTION_SCRIPT_GRASS_WHISTLE = 271, ///< Grass-whistle gesture with inherited cue selection.
    ACTION_SCRIPT_RUN_ON_ALL_FOURS_THEN_FORWARD_JUMP_COLLISION =
        272, ///< Requests an all-fours run start followed by the forward-jump collision response; both inherit cue selection.
    ACTION_SCRIPT_SIT_DOWN = 273,             ///< Begin sitting down.
    ACTION_SCRIPT_SIT_DOWN_CUE_1 = 274,       ///< Requests sit down with fixed cue-kind selector 1.
    ACTION_SCRIPT_SIT_DOWN_CUE_7 = 275,       ///< Requests sit down with fixed cue-kind selector 7.
    ACTION_SCRIPT_SITTING = 276,              ///< Continue the sitting posture.
    ACTION_SCRIPT_SITTING_SCRATCH_EAR = 277,  ///< Ear-scratching motion from the sitting posture.
    ACTION_SCRIPT_THREAT_DISPLAY_START = 278, ///< Requests authored threat display start with inherited cue selection.
    ACTION_SCRIPT_WATERMELON_STRIKE_START = 279, ///< Requests watermelon strike start with inherited cue selection.
    ACTION_SCRIPT_WATERMELON_STRIKE_END_CUE_17 =
        280, ///< Requests watermelon strike end with fixed cue-kind selector 17.
    ACTION_SCRIPT_WATERMELON_STRIKE_END_CUE_0 = 281, ///< Requests watermelon strike end with fixed cue-kind selector 0.
    ACTION_SCRIPT_WATERMELON_STRIKE_CANCEL = 282, ///< Requests watermelon strike cancel with inherited cue selection.
    ACTION_SCRIPT_WATERMELON_STANCE = 283,        ///< Requests watermelon stance with inherited cue selection.
    ACTION_SCRIPT_WATERMELON_STEP_IN_PLACE = 284, ///< Requests watermelon step in place with inherited cue selection.
    ACTION_SCRIPT_WATERMELON_WALK = 285,          ///< Requests watermelon walk with inherited cue selection.
    ACTION_SCRIPT_GREETING_CUE_PLAYBACK =
        286, ///< Request retained greeting cue playback with the default cue selector.
    ACTION_SCRIPT_SQUIRTLE_BIPED_STANCE = 287, ///< Requests squirtle biped stance with inherited cue selection.
    ACTION_SCRIPT_BIPED_FACE_TARGET = 288,     ///< Requests biped face target with inherited cue selection.
    ACTION_SCRIPT_SQUIRTLE_GREET = 289,        ///< Requests squirtle greet with inherited cue selection.
    ACTION_SCRIPT_SQUIRTLE_GREET_ANYWHERE_VARIANT =
        290, ///< Requests squirtle greet anywhere variant with inherited cue selection.
    ACTION_SCRIPT_SQUIRTLE_BAD_TASTE_RESPONSE =
        291, ///< Requests authored squirtle bad taste response with inherited cue selection.
    ACTION_SCRIPT_SQUIRTLE_WHISPER = 292, ///< Requests squirtle whisper with inherited cue selection.
    ACTION_SCRIPT_SQUIRTLE_EAT = 293,     ///< Requests squirtle eat with inherited cue selection.
    ACTION_SCRIPT_CHARMANDER_EAT = 294,   ///< Requests charmander eat with inherited cue selection.
    ACTION_SCRIPT_ODDISH_RUN_TO_TARGET_START =
        296,                                  ///< Requests oddish run to target start with inherited cue selection.
    ACTION_SCRIPT_ODDISH_JUMP_UP = 297,       ///< Requests oddish jump up with inherited cue selection.
    ACTION_SCRIPT_ODDISH_JUMP_UP_CUE_3 = 298, ///< Requests oddish jump up with fixed cue-kind selector 3.
    ACTION_SCRIPT_SQUIRTLE_BIPED_STILL_CUE_0 = 299, ///< Requests squirtle biped still with fixed cue-kind selector 0.
    ACTION_SCRIPT_POLIWAG_FLOAT = 301,              ///< Requests poliwag float with inherited cue selection.
    ACTION_SCRIPT_BIPED_FLOAT_ANYWHERE_VARIANT =
        302,                                 ///< Requests biped float anywhere variant with inherited cue selection.
    ACTION_SCRIPT_POLIWAG_NOD = 303,         ///< Requests poliwag nod with inherited cue selection.
    ACTION_SCRIPT_POLIWAG_ENTER_WATER = 304, ///< Requests poliwag enter water with inherited cue selection.
    ACTION_SCRIPT_POLIWAG_LEAVE_WATER = 305, ///< Requests poliwag leave water with inherited cue selection.
    ACTION_SCRIPT_POLIWAG_JUMP_FACE_TARGET = 306,  ///< Requests poliwag jump face target with inherited cue selection.
    ACTION_SCRIPT_BULBASAUR_STANCE_CUE_0 = 307,    ///< Requests bulbasaur stance with fixed cue-kind selector 0.
    ACTION_SCRIPT_BULBASAUR_WALK_LOOP_CUE_0 = 308, ///< Requests bulbasaur walk loop with fixed cue-kind selector 0.
    ACTION_SCRIPT_VENUSAUR_STANCE_CUE_0 = 309,     ///< Requests venusaur stance with fixed cue-kind selector 0.
    ACTION_SCRIPT_VENUSAUR_OPEN_MOUTH_CUE_0 = 310, ///< Requests venusaur open mouth with fixed cue-kind selector 0.
    ACTION_SCRIPT_VENUSAUR_SLEEP_CUE_0 = 311,      ///< Requests venusaur sleep with fixed cue-kind selector 0.
    ACTION_SCRIPT_TREASURE_CHEST_BURROW_TO_STANCE =
        312, ///< Requests treasure chest burrow to stance with inherited cue selection.
    ACTION_SCRIPT_TREASURE_CHEST_UNDERGROUND_TILT_STANCE =
        313, ///< Requests treasure chest underground tilt stance with inherited cue selection.
    ACTION_SCRIPT_TREASURE_CHEST_BURIED_JUMP_START =
        314, ///< Requests treasure chest buried jump start with inherited cue selection.
    ACTION_SCRIPT_TREASURE_CHEST_JUMP_UP = 315, ///< Requests treasure chest jump up with inherited cue selection.
    ACTION_SCRIPT_TREASURE_CHEST_OPEN = 316,    ///< Requests treasure chest open with inherited cue selection.
    ACTION_SCRIPT_TREASURE_CHEST_TARGET_EFFECT =
        318, ///< Requests treasure chest target effect with inherited cue selection.
    ACTION_SCRIPT_TREASURE_CHEST_STATIC_POSE =
        317,                         ///< Request the authored static treasure-chest pose with the default cue selector.
    ACTION_SCRIPT_HAUNTER_FLY = 319, ///< Requests haunter fly with inherited cue selection.
    ACTION_SCRIPT_HAUNTER_STANCE = 320, ///< Requests haunter stance with inherited cue selection.
    ACTION_SCRIPT_HAUNTER_SCARE_DISPLAY =
        321, ///< Requests authored haunter scare display with inherited cue selection.
    ACTION_SCRIPT_HAUNTER_QUESTIONING_RESPONSE =
        322, ///< Requests authored haunter questioning response with inherited cue selection.
    ACTION_SCRIPT_BUTTERFREE_STANCE = 324,   ///< Requests butterfree stance with inherited cue selection.
    ACTION_SCRIPT_BUTTERFREE_FLY = 325,      ///< Requests butterfree fly with inherited cue selection.
    ACTION_SCRIPT_BUTTERFREE_FLY_SLOW = 326, ///< Requests butterfree fly slow with inherited cue selection.
    ACTION_SCRIPT_BUTTERFREE_FACE_TARGET_AIRBORNE =
        327, ///< Requests butterfree face target airborne with inherited cue selection.
    ACTION_SCRIPT_BUTTERFREE_MOVE_TO_TARGET = 328, ///< Requests butterfree move to target with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_WALK = 329,             ///< Requests caterpie walk with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_WALK_TO_TARGET = 330,   ///< Requests caterpie walk to target with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_RUN = 331,              ///< Requests caterpie run with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_RUN_TO_TARGET = 332,    ///< Requests caterpie run to target with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_STANCE = 333,           ///< Requests caterpie stance with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_EAT = 334,              ///< Requests caterpie eat with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_CURLED = 335,           ///< Requests caterpie curled with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_JUMP_IN_PLACE = 336,    ///< Requests caterpie jump in place with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_FACE_TARGET = 337,      ///< Requests caterpie face target with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_DASH = 338,             ///< Requests caterpie dash with inherited cue selection.
    ACTION_SCRIPT_CATERPIE_SLEEP = 339,            ///< Requests caterpie sleep with inherited cue selection.
    ACTION_SCRIPT_ABRA_STANCE = 340,               ///< Requests abra stance with inherited cue selection.
    ACTION_SCRIPT_CRAB_SWIM = 341,                 ///< Requests crab swim with inherited cue selection.
    ACTION_SCRIPT_JELLYFISH_SWIM = 342,            ///< Requests jellyfish swim with inherited cue selection.
    ACTION_SCRIPT_FISH_SWIM = 343,                 ///< Requests fish swim with inherited cue selection.
    ACTION_SCRIPT_OTHER_SWIM = 344,                ///< Requests other swim with inherited cue selection.
    ACTION_SCRIPT_CRAB_WRITHE_ZERO_GRAVITY = 345,  ///< Requests crab writhe zero gravity with inherited cue selection.
    ACTION_SCRIPT_JELLYFISH_WRITHE_ZERO_GRAVITY =
        346, ///< Requests jellyfish writhe zero gravity with inherited cue selection.
    ACTION_SCRIPT_FISH_WRITHE_ZERO_GRAVITY = 347, ///< Requests fish writhe zero gravity with inherited cue selection.
    ACTION_SCRIPT_CRAB_WRITHE_STANDARD_GRAVITY =
        348, ///< Requests crab writhe standard gravity with inherited cue selection.
    ACTION_SCRIPT_JELLYFISH_WRITHE_STANDARD_GRAVITY =
        349, ///< Requests jellyfish writhe standard gravity with inherited cue selection.
    ACTION_SCRIPT_FISH_WRITHE_STANDARD_GRAVITY =
        350, ///< Requests fish writhe standard gravity with inherited cue selection.
    ACTION_SCRIPT_CREATURE_WALK_TO_TARGET = 351, ///< Requests creature walk to target with inherited cue selection.
    ACTION_SCRIPT_CREATURE_WALK_SLOW = 352,      ///< Requests creature walk slow with inherited cue selection.
    ACTION_SCRIPT_CREATURE_WALK_FAST = 353,      ///< Requests creature walk fast with inherited cue selection.
    ACTION_SCRIPT_CREATURE_TUMBLE_FORWARD = 354, ///< Requests creature tumble forward with inherited cue selection.
    ACTION_SCRIPT_CREATURE_JUMP_IN_PLACE_START =
        355,                                    ///< Requests creature jump in place start with inherited cue selection.
    ACTION_SCRIPT_CREATURE_TURN_IN_PLACE = 356, ///< Requests creature turn in place with inherited cue selection.
    ACTION_SCRIPT_CREATURE_FACE_TARGET = 357,   ///< Requests creature face target with inherited cue selection.
    ACTION_SCRIPT_FACE_DESTINATION_UNDERGROUND = 358, ///< Face the destination while underground.
    ACTION_SCRIPT_CREATURE_FACE_TARGET_UNDERGROUND_QUICK =
        359, ///< Requests creature face target underground quick with inherited cue selection.
    ACTION_SCRIPT_CREATURE_FACE_TARGET_UNDERGROUND_INSTANT =
        360, ///< Requests creature face target underground instant with inherited cue selection.
    ACTION_SCRIPT_CREATURE_BURROW_CUE_0 = 361, ///< Requests creature burrow with fixed cue-kind selector 0.
    ACTION_SCRIPT_CREATURE_BURROW_CUE_1 = 362, ///< Requests creature burrow with fixed cue-kind selector 1.
    ACTION_SCRIPT_CREATURE_BURROW_TO_STANCE_CUE_0 =
        363, ///< Requests creature burrow to stance with fixed cue-kind selector 0.
    ACTION_SCRIPT_CREATURE_BURROW_TO_STANCE_CUE_1 =
        364,                                 ///< Requests creature burrow to stance with fixed cue-kind selector 1.
    ACTION_SCRIPT_MOVE_TO_DESTINATION = 365, ///< Move toward the destination.
    ACTION_SCRIPT_CREATURE_UNDERGROUND_STANCE =
        366,                                  ///< Requests creature underground stance with inherited cue selection.
    ACTION_SCRIPT_VERTICAL_SCALE_PULSE = 367, ///< Requests vertical scale pulse with inherited cue selection.
    ACTION_SCRIPT_VERTICAL_SCALE_PULSE_BYPASS_TERRAIN =
        368, ///< Requests vertical scale pulse bypass terrain with inherited cue selection.
    ACTION_SCRIPT_DAMPED_VERTICAL_SCALE_PULSES_BYPASS_TERRAIN =
        369, ///< Requests damped vertical scale pulses bypass terrain with inherited cue selection.
    ACTION_SCRIPT_CREATURE_SCATTER_ITEMS = 370, ///< Requests creature scatter items with inherited cue selection.
    ACTION_SCRIPT_FISHING_ROD_BEND = 371        ///< Requests fishing rod bend with inherited cue selection.
} ActionScriptId;

#endif
