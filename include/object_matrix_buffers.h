/// \file
/// Declares owned transform-storage allocation for runtime object hierarchies.
#ifndef OBJECT_MATRIX_BUFFERS_H
#define OBJECT_MATRIX_BUFFERS_H
#include <ultra64.h>

/// Allocate transform storage and assign slots for a hierarchy.
/// \param root Live acyclic hierarchy with assigned roots and no unreleased transform storage; required allocation size must fit signed 32 bits.
/// \returns Nothing.
/// \note The root owns one allocation containing both buffers; child slots and the second buffer borrow interior storage.
/// \note Original decompilation identifier: \c func_80008338 (0x80008338).
void objectAllocateHierarchyMatrixBuffers(u8 *root);

#endif
