/// \file
/// Defines borrowed action resources copied into actor state when selecting playback.

#ifndef ACTION_DEFINITION_TYPES_H
#define ACTION_DEFINITION_TYPES_H

#include "ultra64.h"
#include "actor_action_callbacks.h"
#include "actor_action_cue.h"

/// Initialize a selected action for an actor.
/// \param actor Actor whose selected action is initialized.
/// \returns Zero to accept the selected action, or nonzero to select its continuation immediately.
typedef s32 (*ActorActionInitializer)(u8 *actor);

/// One action definition in the ROM's 330-entry action table.
/// Names, callbacks and cue programs are borrowed for the lifetime of playback.
/// This resource layout corresponds to actor-state bytes 0xEC through 0x117.
typedef struct ActionDefinition {
    s32 id;                                     ///< Index of this definition in the action table.
    char *name;                                 ///< Borrowed display/debug name.
    f32 duration;                               ///< Duration value supplied to selected action state.
    s32 attributes;                             ///< Playback kind and action behavior flags.
    s16 motionId;                               ///< Motion archive identifier.
    u8 initialPoseId;                           ///< Pose selector at the beginning of the action.
    u8 finalPoseId;                             ///< Pose selector at the end of the action.
    s16 continuationAction;                     ///< Next action identifier, or an out-of-range termination sentinel.
    u8 unknown016[2];                           ///< Unresolved resource bytes retained without assigning meaning.
    f32 movementScale;                          ///< Action movement multiplier or timed-playback duration.
    ActorActionInitializer initializer;         ///< Optional action acceptance/continuation callback.
    ActorActionUpdateCallback update;           ///< Optional per-frame behavior callback.
    ActorActionCueCommand *primaryCueProgram;   ///< Borrowed primary cue program or selection collection.
    ActorActionCueCommand *secondaryCueProgram; ///< Borrowed secondary cue program or selection collection.
} ActionDefinition;

typedef char ActionDefinitionSizeCheck[(sizeof(ActionDefinition) == 0x2C) ? 1 : -1];
#define ACTION_DEFINITION_OFFSET_CHECK(name, field, offset)                                                            \
    typedef char name[(((u32) & ((ActionDefinition *)0)->field) == (offset)) ? 1 : -1]
ACTION_DEFINITION_OFFSET_CHECK(ActionDefinitionMotionOffsetCheck, motionId, 0x10);
ACTION_DEFINITION_OFFSET_CHECK(ActionDefinitionContinuationOffsetCheck, continuationAction, 0x14);
ACTION_DEFINITION_OFFSET_CHECK(ActionDefinitionScaleOffsetCheck, movementScale, 0x18);
ACTION_DEFINITION_OFFSET_CHECK(ActionDefinitionInitializerOffsetCheck, initializer, 0x1C);
ACTION_DEFINITION_OFFSET_CHECK(ActionDefinitionUpdateOffsetCheck, update, 0x20);
ACTION_DEFINITION_OFFSET_CHECK(ActionDefinitionPrimaryCueOffsetCheck, primaryCueProgram, 0x24);
ACTION_DEFINITION_OFFSET_CHECK(ActionDefinitionSecondaryCueOffsetCheck, secondaryCueProgram, 0x28);
#undef ACTION_DEFINITION_OFFSET_CHECK

#endif
