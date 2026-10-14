/// \file
/// Defines the strided callback views used by the runtime program dispatcher.

#ifndef RUNTIME_PROGRAM_H
#define RUNTIME_PROGRAM_H

#include <ultra64.h>

/// Execute a runtime program's current lifecycle operation.
/// \pre Initialization runs after runtime resource setup; update and drawing require an initialized program.
/// \note Only program zero is present in the retained table. The hooks borrow current runtime state.
/// \returns Nothing.
typedef void (*RuntimeProgramCallback)(void);

/// Interior callback-slot view with the dispatcher-established program stride.
/// The skipped bytes belong to adjacent callback slots or retained storage; this view does not establish ownership
/// or a complete program allocation and must not be used to allocate, clear or copy program records.
typedef struct RuntimeProgramCallbackSlot {
    RuntimeProgramCallback callback; ///< Optional initialization, update or drawing operation for this view.
    u8 skipped[12]; ///< Storage between this callback and the same slot of the next program.
} RuntimeProgramCallbackSlot;

/// Initialization-slot view for program zero; callbacks remain callable while the program is active.
/// \note Original identifier: \c D_80063828 (0x80063828).
extern RuntimeProgramCallbackSlot gRuntimeProgramInitializeEntries[];
/// Update-slot view of the same program table.
/// \note Original identifier: \c D_80063820 (0x80063820).
extern RuntimeProgramCallbackSlot gRuntimeProgramUpdateEntries[];
/// Drawing-slot view of the same program table.
/// \note Original identifier: \c D_80063824 (0x80063824).
extern RuntimeProgramCallbackSlot gRuntimeProgramDrawEntries[];

typedef char RuntimeProgramCallbackStrideCheck[(sizeof(RuntimeProgramCallbackSlot) == 0x10) ? 1 : -1];

#endif
