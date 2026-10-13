/// \file
/// Advances retained actor script contexts across gameplay updates.

#ifndef SCRIPT_VM_EXECUTION_H
#define SCRIPT_VM_EXECUTION_H

#include <ultra64.h>

/// Run bytecode until a scheduling boundary or an execution error.
/// \param actor Actor with initialized script state and well-formed bytecode;
/// stack, frame, and variable accesses must remain within their capacities.
/// \returns SCRIPT_VM_YIELDED, SCRIPT_VM_WAITING_FOR_ACTION, or
/// SCRIPT_VM_FINISHED; SCRIPT_VM_ERROR reports a missing context or invalid opcode.
/// \note Original identifier: \c func_80116284 (0x80116284).
s32 actorRunScriptVm(u8 *actor);

/// Advance an actor's script timers and scheduled execution for one frame.
/// \param actor Actor with initialized script state; an absent context is allowed.
/// \returns Nothing.
/// \note Original identifier: \c func_80116658 (0x80116658).
void actorUpdateScriptVm(u8 *actor);

#endif
