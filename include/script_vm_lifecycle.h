/// \file
/// Creates, resets, and releases retained actor script contexts.

#ifndef SCRIPT_VM_LIFECYCLE_H
#define SCRIPT_VM_LIFECYCLE_H

#include "script_vm.h"

/// Reset execution and local values while retaining resources and total elapsed time.
/// \param vm Context to reuse, or null.
/// \returns Nothing.
/// \note Original identifier: \c func_80115070 (0x80115070).
void scriptVmReset(u8 *vm);

/// Create a context with an explicit value-stack capacity.
/// \param config Configuration whose borrowed program, strings, and commands must outlive the context.
/// \param capacity Positive maximum number of stack values.
/// \returns Owned initialized context to release with scriptVmDestroy.
/// \note Original identifier: \c func_80115114 (0x80115114).
ScriptVm *scriptVmCreateWithCapacity(ScriptVmConfiguration *config, s32 capacity);

/// Create a context using its configured value-stack capacity.
/// \param config Configuration whose borrowed resources must outlive the context;
/// a nonpositive stack capacity selects 200 values.
/// \returns Owned initialized context to release with scriptVmDestroy.
/// \note Original identifier: \c func_801151B0 (0x801151B0).
ScriptVm *scriptVmCreate(ScriptVmConfiguration *config);

/// Release a context and its owned storage, retaining borrowed resources.
/// \param vm Context to release, or null; its address becomes invalid after release.
/// \returns Nothing.
/// \note Original identifier: \c func_801151E4 (0x801151E4).
void scriptVmDestroy(u8 *vm);

#endif
