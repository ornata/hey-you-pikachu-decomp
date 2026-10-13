/// \file
/// Declares actor script action selectors and the interface for scheduling and advancing them.

#ifndef ACTOR_SCRIPT_ACTION_H
#define ACTOR_SCRIPT_ACTION_H

#include "ultra64.h"

/// Action-handler selectors retained by the actor script VM while an action waits.
/// \ingroup ActorScriptVm
typedef enum ActorScriptActionHandler {
    ACTOR_SCRIPT_ACTION_DEFAULT_ACTION = 0, ///< Ensure the default action is running and finish with a zero result.
    ACTOR_SCRIPT_ACTION_POLL_ACTION_STOPPED = 1,
    ACTOR_SCRIPT_ACTION_PLAY_ACTION = 2,
    ACTOR_SCRIPT_ACTION_EXIT_ACTION_LOOP = 3,
    ACTOR_SCRIPT_ACTION_WAIT_FOR_ACTION_OR_MESSAGE = 4,
    ACTOR_SCRIPT_ACTION_PLAY_INTERRUPTIBLE_ACTION = 5,
    ACTOR_SCRIPT_ACTION_THROW_HELD_ITEM = 6,
    ACTOR_SCRIPT_ACTION_ACQUIRE_ITEM = 7,
    ACTOR_SCRIPT_ACTION_EAT_HELD_ITEM = 8,
    ACTOR_SCRIPT_ACTION_PERFORM_COSTED_ACTION = 9,
    ACTOR_SCRIPT_ACTION_SELECT_CONTROL_SCHEME = 10,
    ACTOR_SCRIPT_ACTION_BEGIN_AREA_TRANSITION = 11,
    ACTOR_SCRIPT_ACTION_FINISH_AREA_TRANSITION = 12,
    ACTOR_SCRIPT_ACTION_OPEN_TEXT_WINDOW = 13,
    ACTOR_SCRIPT_ACTION_TRANSFORM_KIND = 14,
} ActorScriptActionHandler;

/// Select a script action handler and request its initialization.
/// \param actor Actor with a script context to update.
/// \param handler An ActorScriptActionHandler selector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801167A0 (0x801167A0).
void actorBeginScriptActionHandler(u8 *actor, s32 handler);

/// Advance the actor currently selected script action handler.
/// \param object Actor whose script action handler is dispatched.
/// \returns SCRIPT_VM_YIELDED when the handler completes,
/// SCRIPT_VM_WAITING_FOR_ACTION while it waits, or SCRIPT_VM_ERROR for an unknown handler.
/// \note Original decompilation identifier: \c func_80117098 (0x80117098).
s32 actorScriptDispatchActionHandler(u8 *object);

#endif
