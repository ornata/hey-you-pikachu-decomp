#include "common.h"

extern double sqrt(double);
#pragma intrinsic(sqrt);

/// Compute the double-precision square root of a number.
/// \param x Number whose square root is requested; nonnegative for a real result.
/// \returns The nonnegative square root, or NaN for a negative input.
/// \note Original decompilation identifier: \c sqrt (0x80048F50).
double sqrt(double x) {
    return sqrt(x);
}
