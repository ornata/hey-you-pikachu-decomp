/// \file
/// Declares packed normal conversion and direction-to-angle interfaces.
#ifndef VECTOR_DIRECTION_H
#define VECTOR_DIRECTION_H
#include <ultra64.h>

/// Convert a normalized direction to packed signed-byte components.
/// \param source Three floating-point components in [-1, 1].
/// \param destination Storage for three packed components.
/// \returns Nothing.
void vector3PackNormalBytes(f32 *source, u8 *destination);

/// Expand a packed signed-byte normal to a floating-point direction.
/// \param source Three signed-byte components.
/// \param destination Storage for three floating-point components.
/// \returns Nothing.
void vector3UnpackNormalBytes(s8 *source, f32 *destination);

/// Get elevation and heading with Y as the vertical axis.
/// \param direction Three-component input direction.
/// \param pitch Receives elevation in degrees, or zero for a near-zero direction.
/// \param yaw Receives heading from negative Z toward negative X in degrees, or zero for a near-zero direction.
/// \returns Nothing.
void vector3GetLookAnglesDegrees(f32 *direction, f32 *pitch, f32 *yaw);

/// Get elevation and heading for the opposite of a direction.
/// \param direction Three-component input direction.
/// \param pitch Receives opposite-direction elevation in degrees.
/// \param yaw Receives opposite-direction heading from negative Z toward negative X in degrees.
/// \returns Nothing. Both outputs are zero for a near-zero direction.
void vector3EvaluateOppositeLookAngles(f32 *direction, f32 *pitch, f32 *yaw);
#endif
