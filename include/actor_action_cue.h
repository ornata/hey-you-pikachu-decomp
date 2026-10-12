/// \file
/// Defines timed actor action-cue commands and borrowed program selection/update interfaces.

#ifndef ACTOR_ACTION_CUE_H
#define ACTOR_ACTION_CUE_H

#include "ultra64.h"

/// A timed action cue or selection tag in a borrowed actor resource.
/// Tags below nineteen select action kinds; timed operations begin at nineteen.
/// This format is distinct from the 0x10-byte action-sequencer command.
typedef struct ActorActionCueCommand {
    s32 opcode;   ///< Action-kind tag or timed operation selector.
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
