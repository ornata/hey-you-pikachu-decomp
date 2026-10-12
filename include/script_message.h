/// \file
/// Defines the shared message exchanged by gameplay producers and actor scripts.

#ifndef SCRIPT_MESSAGE_H
#define SCRIPT_MESSAGE_H

#include <ultra64.h>

/// Message copied into an actor's single pending-message slot.
///
/// Argument meanings are defined by the producer. Voice commands carry a
/// command value, quality flags, voice level, duration, and interaction candidate.
/// Inspecting message fields does not acknowledge delivery; a forced post may
/// replace an unread message.
/// \ingroup ActorScriptVm
/// \see actorPostScriptMessage
/// \see actorAcknowledgeScriptMessage
/// \see voiceCommandTakeMessage
typedef struct ScriptMessage {
    s32 type; ///< Producer-defined message category.
    s32 arg0; ///< First producer-defined argument; voice command value.
    s32 arg1; ///< Second producer-defined argument; voice quality flags.
    s32 arg2; ///< Third producer-defined argument; voice level.
    s32 arg3; ///< Fourth producer-defined argument; voice duration.
    s32 arg4; ///< Fifth producer-defined argument; voice interaction candidate.
} ScriptMessage;

/* Messages are passed by value and copied into the original six-word mailbox. */
typedef char ScriptMessageSizeCheck[(sizeof(ScriptMessage) == 0x18) ? 1 : -1];

#endif
