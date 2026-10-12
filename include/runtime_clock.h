/// \file
/// Declares unsigned elapsed-time readings from the runtime's resettable clocks.

#ifndef RUNTIME_CLOCK_H
#define RUNTIME_CLOCK_H

#include <ultra64.h>

/// Read elapsed runtime time since the timing clocks were reset.
/// \returns Elapsed milliseconds as an unsigned 64-bit counter.
/// \note Original decompilation identifier: \c func_8000B0FC (0x8000B0FC).
u64 runtimeGetElapsedMilliseconds(void);

/// Read elapsed runtime time since the timing clocks were reset.
/// \returns Elapsed whole seconds as an unsigned 64-bit counter.
/// \note Original decompilation identifier: \c func_8000B110 (0x8000B110).
u64 runtimeGetElapsedSeconds(void);

#endif
