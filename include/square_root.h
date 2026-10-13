/// \file
/// Declares the target square-root entry point.

#ifndef SQUARE_ROOT_H
#define SQUARE_ROOT_H

#include <ultra64.h>

/// Compute the double-precision square root of a number.
/// \param x Number whose square root is requested; nonnegative for a real result.
/// \returns The nonnegative square root, or NaN for a negative input.
/// \note Original decompilation identifier: \c sqrt (0x80048F50).
double sqrt(double x);

#endif
