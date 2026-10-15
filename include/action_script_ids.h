/// \file
/// Names timed action programs with roles established by their motion definitions.

#ifndef ACTION_SCRIPT_IDS_H
#define ACTION_SCRIPT_IDS_H

/// Known indices in the timed sequencer's program table.
/// These are distinct from the motion-action IDs requested by program commands.
/// Names describe roles supported by retained motion debug labels; this is not
/// an exhaustive list of valid programs or a claim of rendered-animation validation.
typedef enum ActionScriptId {
    ACTION_SCRIPT_TURN_TO_DESIRED_HEADING = 17,       ///< Turn toward the requested heading.
    ACTION_SCRIPT_LOOK_AROUND = 168,                  ///< Look around the actor's surroundings.
    ACTION_SCRIPT_FACE_DESTINATION_UNDERGROUND = 358, ///< Face the destination while underground.
    ACTION_SCRIPT_MOVE_TO_DESTINATION = 365           ///< Move toward the destination.
} ActionScriptId;

#endif
