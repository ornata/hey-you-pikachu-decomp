/// \file
/// Defines timed actor action-cue commands and borrowed program selection/update interfaces.

#ifndef ACTOR_ACTION_CUE_H
#define ACTOR_ACTION_CUE_H

#include "ultra64.h"

/// Recovered selection tag and timed operations in the actor cue resource format.
/// Other tags below ACTION_CUE_FIRST_HANDLER remain action-kind selectors.
/// These encodings are separate from VM bytecode and action-sequencer opcodes.
typedef enum ActorActionCueCode {
    ACTION_CUE_ANY_ACTION = 18,    ///< Fallback selection tag; action kind 18 accepts a direct program.
    ACTION_CUE_FIRST_HANDLER = 19, ///< First table-dispatched operation; not a separate operation.
    ACTION_CUE_CONFIGURE_PART_ANIMATION = 19,
    ACTION_CUE_CHANGE_HELD_ITEM_ANIMATION = 20,
    ACTION_CUE_PLAY_TRACKED_SOUND_WITH_PITCH = 21,
    ACTION_CUE_PLAY_GROUND_CONTACT_EFFECTS = 22,
    ACTION_CUE_CONFIGURE_PART_SCALE = 23,
    ACTION_CUE_SET_ACTOR_TINT = 24,
    ACTION_CUE_LAUNCH_HELD_ITEM = 25,
    ACTION_CUE_PICK_UP_NEARBY_ITEM = 26,
    ACTION_CUE_CREATE_RANDOM_HELD_ITEM = 27,
    ACTION_CUE_EMIT_PAIRED_TARGETED_BURSTS = 28,
    ACTION_CUE_CREATE_FLICKERING_PART_EFFECT = 29,
    ACTION_CUE_CREATE_ITEM_TRANSFORMATION_EFFECT = 30,
    ACTION_CUE_CREATE_STACKED_EFFECTS = 31,
    ACTION_CUE_EMIT_PARTICLES_AT_TARGET = 32,
    ACTION_CUE_CREATE_SCATTERED_PARTICLE_EFFECT = 33,
    ACTION_CUE_CREATE_LINKED_PART_EFFECT = 34,
    ACTION_CUE_CREATE_ANIMATED_RADIAL_EFFECT = 35,
    ACTION_CUE_EMIT_EFFECT_AT_SELECTED_ATTACHMENT = 36,
    ACTION_CUE_CREATE_EFFECT_AT_ACTOR_OR_TARGET = 37,
    ACTION_CUE_CREATE_ATTACHED_EFFECT = 38,
    ACTION_CUE_STUN_PLAYER_IN_RANGE = 39,
    ACTION_CUE_START_DIRECTION_MOTION = 40,
    ACTION_CUE_CONFIGURE_HELD_ITEM = 41,
    ACTION_CUE_TRANSFORM_TARGET_ITEM = 42,
    ACTION_CUE_NOTIFY_STORED_TARGET = 43,
    ACTION_CUE_ADVANCE_CLOCK = 44,
    ACTION_CUE_ADVANCE_CLOCK_RANDOMLY = 45,
    ACTION_CUE_JUMP = 46,              ///< Continue at a borrowed target, optionally resetting the cue clock.
    ACTION_CUE_SYNC_MOTION_CLOCK = 47, ///< Wait for equal motion timers within tolerance, then reset the cue clock.
    ACTION_CUE_STOP = 48,              ///< Stop the current cue program.
    ACTION_CUE_END_COLLECTION = 49     ///< End selection search; also stops cue execution.
} ActorActionCueCode;

/// A timed action cue or selection tag in a borrowed actor resource.
/// Tags below nineteen select action kinds; timed operations begin at nineteen.
/// This format is distinct from the 0x10-byte action-sequencer command.
typedef struct ActorActionCueCommand {
    s32 opcode;   ///< Action-kind tag or ActorActionCueCode selector, retained as a target word.
    f32 time;     ///< Earliest cue time in seconds for timed operations.
    u8 args[0xC]; ///< Operation-specific operands; selection-tag payload is unused by selection.
} ActorActionCueCommand;

typedef char ActorActionCueCommandSizeCheck[(sizeof(ActorActionCueCommand) == 0x14) ? 1 : -1];
typedef char ActorActionCueArgumentsOffsetCheck[(((u32) & ((ActorActionCueCommand *)0)->args) == 8) ? 1 : -1];

/// Advance an actor's timed action script through the commands ready to run.
/// \pre Commands and jump targets must remain valid, use supported handlers, and eventually finish or wait for a future time.
/// \param actor Actor performing the actions.
/// \param nextCommand Current command, updated to the next pending command or null when finished.
/// \param clock Writable elapsed script time in seconds, updated as the script advances.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024DD0 (0x80024DD0).
void actorUpdateActionScript(u8 *actor, ActorActionCueCommand **nextCommand, f32 *clock);

/// Select an action script for the actor's current action state.
/// \param actor Actor whose action state selects the script.
/// \param list Borrowed script pointer for action state 18, otherwise a nullable tagged collection terminated by tag 49; it must remain valid during playback.
/// \param unusedFlag Legacy flag that does not affect the selection.
/// \returns A borrowed selected script, the supplied list when no state selection is needed, or null if no script matches.
/// \note Original decompilation identifier: \c func_80024F9C (0x80024F9C).
ActorActionCueCommand *actorSelectActionScript(u8 *actor, ActorActionCueCommand *list, s32 unusedFlag);

#endif
