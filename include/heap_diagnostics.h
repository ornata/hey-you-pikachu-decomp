/// \file
/// Declares the retained free-space diagnostic interface for game heaps.

#ifndef HEAP_DIAGNOSTICS_H
#define HEAP_DIAGNOSTICS_H

#include "heap_types.h"

/// Inspect free space for the disabled heap diagnostic report.
/// \param heap Borrowed initialized heap to inspect.
/// \param label Diagnostic label; unused by the retail build.
/// \returns Nothing.
void heapDebugReportFreeSpace(HpHdr *heap, const char *label);

#endif
