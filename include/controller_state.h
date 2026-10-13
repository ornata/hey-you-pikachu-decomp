/// \file
/// Declares shared processed controller samples and their consumption lifetime.

#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

#include "controller_types.h"

/// Current processed sample for each of the four controller ports.
/// Input updates refresh the records. UI consumers may clear primary button
/// state after handling input; retained references observe subsequent updates.
/// \note Original identifier: \c D_800AD540 (0x800AD540).
extern ControllerState gControllerInputState[4];

#endif
