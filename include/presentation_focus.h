/// \file
/// Declares smooth shared presentation focus transitions in presentation coordinates.

#ifndef PRESENTATION_FOCUS_H
#define PRESENTATION_FOCUS_H

#include "vector3_types.h"

/// Begin a smooth 0.6-second transition of the shared presentation focus point.
/// \param target Target point in presentation coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D260 (0x8002D260).
void presentationSetFocusTarget(Vec3f *target);

/// Set the shared presentation focus point immediately.
/// \param point Focus point in presentation coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D2C8 (0x8002D2C8).
void presentationResetFocusPoint(Vec3f *point);

/// Update and obtain the shared presentation focus point during its transition.
/// \param point Receives the current point in presentation coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D348 (0x8002D348).
void presentationUpdateFocusPoint(Vec3f *point);

#endif
