/// \file
/// Defines the shared message exchanged by gameplay producers and actor scripts.

#ifndef SCRIPT_MESSAGE_H
#define SCRIPT_MESSAGE_H

#include <ultra64.h>

/// Actor-mailbox categories established by their gameplay producers.
/// These encodings are separate from scheduler GameMessageKind values.
/// Other categories, including dictionary-defined voice commands, remain producer-defined.
typedef enum ActorScriptMessageKind {
    SCRIPT_MESSAGE_ITEM_IMPACT = 14,         ///< Item handle in arguments 0/4 and impact speed in argument 2.
    SCRIPT_MESSAGE_COOPERATION_REQUEST = 15, ///< Requested interaction target handle in argument 4.
    SCRIPT_MESSAGE_ACTION_NOTIFICATION = 16, ///< Cue arguments in 0/2/3 and sending actor handle in argument 4.
    SCRIPT_MESSAGE_PROXY_USED = 17,          ///< Proxy metadata in argument 0, remaining uses in 2, proxy handle in 4.
    SCRIPT_MESSAGE_WORLD_MAP_DESTINATION =
        18 ///< Recognized map destination selector in argument 0, including special training.
} ActorScriptMessageKind;

/// Message copied into an actor's single pending-message slot.
///
/// Categories can be shared by recognized voice traffic and scripted
/// notifications; a category alone does not establish a voice payload.
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
