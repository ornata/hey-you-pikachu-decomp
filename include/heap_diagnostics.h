/// \file
/// Declares free-space measurement and diagnostic interfaces for game heaps.

#ifndef HEAP_DIAGNOSTICS_H
#define HEAP_DIAGNOSTICS_H

#include "heap_types.h"

/// Measure total free space and the largest available heap allocation.
/// \param heap Borrowed initialized heap whose free payload total fits in signed 32 bits.
/// \param largest Writable result receiving the largest free payload size in bytes; on validation failure it contains the largest size found before failure.
/// \returns Total free payload bytes, or zero if block validation fails.
/// \note Original decompilation identifier: \c func_800385D0 (0x800385D0).
s32 heapGetFreeSpace(HpHdr *heap, u32 *largest);

/// Inspect free space for the disabled heap diagnostic report.
/// \param heap Borrowed initialized heap to inspect.
/// \param label Diagnostic label; unused by the retail build.
/// \returns Nothing.
void heapDebugReportFreeSpace(HpHdr *heap, const char *label);

#endif
