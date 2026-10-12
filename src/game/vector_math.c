#include "matrix_orientation.h"
#include "audio_spatial.h"
/* harvested: matched m2c drafts; pending style pass */
/* Vector, angle and interpolation helpers. The unit ends at randomBelowWithHistory:
 * its .rodata is float literals only, padded to 16 bytes before motion_playback. */
#include "common.h"
#include "vector_direction.h"

extern float fabsf(float);
#pragma intrinsic(fabsf)
void matrix34TransformPointVector(f32 *, f32 *, f32 *);
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
f32 mathUnitDirectionDegrees(f32, f32);
void motionExtractHeadingFromMatrix(f32 *, f32 *);
f32 vector3Normalize(f32 *);

/// Get the Y-Z projection angle of a transformation's Z row.
/// \param matrix Input affine transformation matrix.
/// \returns Angle in degrees from positive Z toward positive Y, or zero for a near-zero projection.
/// \note Original decompilation identifier: \c func_8000C7F0 (0x8000C7F0).
f32 matrix34GetZRowYZAngleDegrees(u8 *matrix) {
    f32 projectedZ;
    f32 projectedY;
    f32 projectionLength;

    projectedZ = *(f32 *)(matrix + 0x20);
    projectedY = *(f32 *)(matrix + 0x14);
    projectionLength = sqrtf((projectedZ * projectedZ) + (projectedY * projectedY));
    if ((projectionLength < 0.001f) && (-0.001f < projectionLength)) {
        return 0.0f;
    }
    projectedZ /= projectionLength;
    projectedY /= projectionLength;
    return mathUnitDirectionDegrees(projectedZ, projectedY);
}

/// Evaluate the elevation of a transformation's X basis without exposing a result.
/// \param matrix Input affine transformation matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C868 (0x8000C868).
void matrix34EvaluateXBasisElevationDegrees(u8 *matrix) {
    f32 projectionLength;
    f32 signedProjectionLength;
    f32 basisX;
    f32 basisY;
    f32 negativeBasisZ;

    basisY = *(f32 *)(matrix + 4);
    basisX = *(f32 *)(matrix + 0);
    negativeBasisZ = -*(f32 *)(matrix + 8);
    projectionLength = sqrtf((basisY * basisY) + (basisX * basisX));
    signedProjectionLength = projectionLength;
    if (*(f32 *)(matrix + 0x20) <= 0.0f) {
        signedProjectionLength = -projectionLength;
    }
    mathUnitDirectionDegrees(signedProjectionLength, negativeBasisZ);
}

/// Get the direction of a two-dimensional vector.
/// \param vector Input vector with X and Y components.
/// \returns Direction in degrees from positive X toward positive Y, or zero for a near-zero vector.
/// \note Original decompilation identifier: \c func_8000C8CC (0x8000C8CC).
f32 vector2GetDirectionDegrees(u8 *vector) {
    f32 x;
    f32 y;
    f32 length;

    x = *(f32 *)(vector + 0);
    y = *(f32 *)(vector + 4);
    length = sqrtf((x * x) + (y * y));
    if ((length < 0.001f) && (-0.001f < length)) {
        return 0.0f;
    }
    x /= length;
    y /= length;
    return mathUnitDirectionDegrees(x, y);
}

void matrixFixedTo34(Mtx, f32 *);
f32 vector3Length(f32 *);
f32 mathMapRangeClamped(f32, f32, f32, f32, f32);

/// Determine positional sound volume and stereo pan relative to the camera.
/// \param pos World-space sound position with a nonzero distance from the camera.
/// \param cam Camera transformation matrix.
/// \param vol Receives the distance-dependent volume level, from 80 through 256.
/// \param pan Receives stereo pan from zero through 255, with 128 centered.
/// \param near Distance at which volume reaches its maximum.
/// \param far Distance greater than near at which volume reaches its minimum.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C944 (0x8000C944).
void audioGetPositionVolumeAndPan(f32 *pos, Mtx cam, s32 *vol, s32 *pan, f32 near, f32 far) {
    s32 p;
    f32 dist;
    f32 v[3];
    f32 m[12];
    s32 level;

    matrixFixedTo34(cam, m);
    matrix34TransformPointVector(m, pos, v);
    dist = vector3Length(v);
    p = v[0] / dist * 128.0f;
    if (p < -128) {
        p = -128;
    } else if (p >= 128) {
        p = 127;
    }
    p += 128;
    level = mathMapRangeClamped(near, 256.0f, far, 80.0f, dist);
    *vol = level;
    *pan = p;
}

/// Get elevation and heading for the opposite of an input direction.
/// \param direction Input three-component direction.
/// \param pitch Receives opposite-direction elevation in degrees.
/// \param yaw Receives opposite-direction heading from negative Z toward negative X in degrees.
/// \returns Nothing; both outputs are zero for a near-zero direction.
/// \note Original decompilation identifier: \c func_8000CA60 (0x8000CA60).
void vector3EvaluateOppositeLookAngles(f32 *direction, f32 *pitch, f32 *yaw) {
    f32 v[3];

    v[0] = -direction[0];
    v[1] = -direction[1];
    v[2] = -direction[2];
    vector3GetLookAnglesDegrees(v, pitch, yaw);
}

/// Get the horizontal direction of a three-dimensional vector.
/// \param vector Input vector whose X and Z components determine the direction.
/// \returns Yaw in degrees from positive Z toward positive X, or zero for a near-zero horizontal vector.
/// \note Original decompilation identifier: \c func_8000CAA4 (0x8000CAA4).
f32 vector3GetYawDegrees(f32 *vector) {
    f32 z;
    f32 x;
    f32 horizontalLength;

    x = vector[0];
    z = vector[2];
    horizontalLength = sqrtf((x * x) + (z * z));
    if ((horizontalLength < 0.001f) && (-0.001f < horizontalLength)) {
        return 0.0f;
    }
    x /= horizontalLength;
    z /= horizontalLength;
    return mathUnitDirectionDegrees(z, x);
}

/// Get the horizontal direction relative to the positive X axis.
/// \param vector Input vector whose X and Z components determine the direction.
/// \returns Direction in the range zero inclusive to 360 exclusive, increasing toward negative Z; near-zero horizontal vectors return 270 degrees.
/// \note Original decompilation identifier: \c func_8000CB1C (0x8000CB1C).
f32 vector3GetYawFromPositiveXDegrees(f32 *vector) {
    f32 var_f2;

    var_f2 = vector3GetYawDegrees(vector) - 90.0f;
    if (var_f2 < 0.0f) {
        do {
            var_f2 += 360.0f;
        } while (var_f2 < 0.0f);
    }
    if (var_f2 >= 360.0f) {
        do {
            var_f2 -= 360.0f;
        } while (var_f2 >= 360.0f);
    }
    return var_f2;
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
/// Move a position toward a target with distance-dependent easing.
/// \param target Target three-component position.
/// \param position Current three-component position, updated in place.
/// \param speed Movement-speed control.
/// \returns Minus one when the position reaches a target within one world unit and is snapped to it; otherwise zero.
/// \note Original decompilation identifier: \c func_8000CBA8 (0x8000CBA8).
s32 vector3ApproachTarget(f32 *target, f32 *position, f32 speed) {
    f32 delta[3];
    f32 step;
    f32 distanceSquared;
    f32 distance;
    f32 scale;
    s32 count;
    s32 i;

    count = gRuntimeFrameSeconds * 60.0f > 1 ? gRuntimeFrameSeconds * 60.0f : 1;
    step = 0.2f / count;
    speed *= step;
    for (i = 0; i < count; i++) {
        delta[0] = target[0] - position[0];
        delta[1] = target[1] - position[1];
        delta[2] = target[2] - position[2];
        distanceSquared = vector3Length(delta);
        if (distanceSquared <= 1.0f) {
            position[0] = target[0];
            position[1] = target[1];
            position[2] = target[2];
            return -1;
        }
        distance = mathMapRangeClamped(1.0f, step, 100.0f, speed, distanceSquared);
        scale = distance / sqrtf(distanceSquared);
        position[0] += delta[0] * scale;
        position[1] += delta[1] * scale;
        position[2] += delta[2] * scale;
    }
    return 0;
}

/// Get a direction's yaw and its pitch projected onto the X-Y plane.
/// \param direction Input three-component direction.
/// \param yaw Receives yaw in degrees from positive Z toward positive X.
/// \param pitch Receives signed projected pitch in degrees, positive downward; both outputs are zero for a zero-length direction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000CD64 (0x8000CD64).
void vector3GetYawAndProjectedPitchDegrees(f32 *direction, f32 *yaw, f32 *pitch) {
    f32 v[3];
    v[0] = direction[0];
    v[1] = direction[1];
    v[2] = direction[2];
    if (vector3Normalize(v) == 0.0f) {
        *yaw = 0.0f;
        *pitch = 0.0f;
        return;
    }
    *yaw = vector3GetYawDegrees(v);
    *pitch = 360.0f - mathUnitDirectionDegrees(fabsf(v[0]), v[1]);
    if (*pitch >= 180.0f) {
        *pitch -= 360.0f;
    }
}

/// Construct a unit direction using Z-axis pitch and planar yaw.
/// \param pitch Pitch in degrees from the XY plane toward negative Z.
/// \param yaw Yaw in degrees from positive X toward positive Y.
/// \param direction Receives the three-component unit direction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000CE20 (0x8000CE20).
void vector3FromZAxisPitchYawDegrees(f32 pitch, f32 yaw, f32 *direction) {
    f32 sA;
    f32 cA;
    f32 sB;
    f32 cB;
    sA = mathCosDegrees(pitch);
    cA = mathSinDegrees(pitch);
    sB = mathCosDegrees(yaw);
    cB = mathSinDegrees(yaw);
    direction[0] = sA * sB;
    direction[1] = sA * cB;
    direction[2] = -cA;
}

/// Construct a unit direction from pitch and yaw.
/// \param pitch Pitch in degrees, positive downward.
/// \param yaw Yaw in degrees from positive Z toward positive X.
/// \param direction Receives the three-component unit direction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000CE9C (0x8000CE9C).
void vector3FromPitchYawDegrees(f32 pitch, f32 yaw, f32 *direction) {
    f32 cosPitch;
    f32 sinPitch;
    f32 cosYaw;
    f32 sinYaw;
    cosPitch = mathCosDegrees(pitch);
    sinPitch = mathSinDegrees(pitch);
    cosYaw = mathCosDegrees(yaw);
    sinYaw = mathSinDegrees(yaw);
    direction[0] = cosPitch * sinYaw;
    direction[1] = -sinPitch;
    direction[2] = cosPitch * cosYaw;
}

s32 mathCosSignDegrees(f32);
/// Get direction angles using a reference heading to choose the orientation.
/// \param vector Input three-component direction vector.
/// \param heading Reference planar yaw in degrees, also used at a vertical direction.
/// \param pitch Receives pitch in degrees toward negative Z.
/// \param yaw Receives planar yaw in degrees from positive X toward positive Y.
/// \returns Nothing. Near-zero vectors use the positive X direction as a fallback.
/// \note Original decompilation identifier: \c func_8000CF14 (0x8000CF14).
void vector3GetZAxisAnglesNearHeading(f32 *vector, f32 heading, f32 *pitch, f32 *yaw) {
    f32 xSquared;
    f32 ySquared;
    f32 zSquared;
    f32 cosPitch;
    f32 sinPitch;
    f32 cosYaw;
    f32 sinYaw;
    f32 lengthSquared;
    f32 x;
    f32 y;
    f32 z;
    f32 length;
    s32 direction;
    x = vector[0];
    y = vector[1];
    z = vector[2];
    xSquared = x * x;
    ySquared = y * y;
    zSquared = z * z;
    lengthSquared = xSquared + ySquared + zSquared;
    length = sqrtf(lengthSquared);
    if (length < 0.001f && -0.001f < length) {
        x = 1.0f;
        z = 0.0f;
        y = 0.0f;
    } else {
        length = 1.0f / length;
        x *= length;
        y *= length;
        z *= length;
    }
    sinPitch = -z;
    if (0.999f <= sinPitch || sinPitch <= -0.999f) {
        if (sinPitch > 0.0f)
            *pitch = 90.0f;
        else
            *pitch = 270.0f;
        *yaw = heading;
        return;
    }
    if (sinPitch < -1.0f)
        sinPitch = -1.0f;
    else if (sinPitch > 1.0f)
        sinPitch = 1.0f;
    cosPitch = sqrtf(1.0f - sinPitch * sinPitch);
    cosYaw = x / cosPitch;
    sinYaw = y / cosPitch;
    direction = mathCosSignDegrees(heading);
    if ((direction < 0 && cosYaw > 0.0f) || (direction > 0 && cosYaw < 0.0f)) {
        cosPitch = -cosPitch;
        cosYaw = -cosYaw;
        sinYaw = -sinYaw;
    }
    *pitch = mathUnitDirectionDegrees(cosPitch, sinPitch);
    *yaw = mathUnitDirectionDegrees(cosYaw, sinYaw);
}

/* Scale a 3-component vector held in scalars, in place. */
#define VEC3_SCALE(x, y, z, k)                                                                                         \
    do {                                                                                                               \
        (x) *= (k);                                                                                                    \
        (y) *= (k);                                                                                                    \
        (z) *= (k);                                                                                                    \
    } while (0)

/// Get Z-axis pitch and planar yaw for a direction vector.
/// \param v Input three-component direction vector.
/// \param pitch Receives pitch in degrees from the XY plane toward negative Z.
/// \param yaw Receives yaw in degrees from positive X toward positive Y.
/// \returns One for a nondegenerate direction; zero for a near-zero or near-vertical direction, with fallback angles still supplied.
/// \note Original decompilation identifier: \c func_8000D13C (0x8000D13C).
s32 vector3GetZAxisPitchYawDegrees(f32 *v, f32 *pitch, f32 *yaw) {
    f32 x, y, z;
    f32 s;
    f32 c;
    f32 qx;
    f32 qy;
    f32 len;
    f32 n;

    x = v[0];
    y = v[1];
    z = v[2];
    len = sqrtf(x * x + y * y + z * z);
    if (len < 0.001f && len > -0.001f) {
        *yaw = 0.0f;
        *pitch = 0.0f;
        return 0;
    }
    len = 1.0f / len;
    VEC3_SCALE(x, y, z, len);
    n = -z;
    s = n;
    if (n >= 0.999f || n <= -0.999f) {
        if (n > 0.0f) {
            *pitch = 90.0f;
        } else {
            *pitch = 270.0f;
        }
        *yaw = 0.0f;
        return 0;
    }
    if (n < -1.0f) {
        s = -1.0f;
    } else if (n > 1.0f) {
        s = 1.0f;
    }
    c = sqrtf(1.0f - s * s);
    qx = x / c;
    qy = y / c;
    *pitch = mathUnitDirectionDegrees(c, s);
    *yaw = mathUnitDirectionDegrees(qx, qy);
    return 1;
}

/// Extract roll, pitch, and yaw from a rotation matrix.
/// \param matrix Input nine-component rotation matrix.
/// \param roll Receives roll in degrees.
/// \param pitch Receives pitch in degrees.
/// \param yaw Receives yaw in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D2E4 (0x8000D2E4).
void matrix3GetEulerAnglesDegrees(f32 *matrix, f32 *roll, f32 *pitch, f32 *yaw) {
    f32 sinRoll;
    f32 cosRoll;
    f32 rollSquared;
    f32 cosPitch;
    f32 sinYaw;
    s32 verticalSign;
    f32 sinPitch;
    f32 cosYaw;

    sinPitch = -matrix[2];
    if ((sinPitch <= -0.999f) || (0.999f <= sinPitch)) {
        if (sinPitch <= 0.0f) {
            *pitch = 270.0f;
        } else {
            *pitch = 90.0f;
        }
        *roll = 0.0f;
        cosYaw = matrix[4];
        *yaw = mathUnitDirectionDegrees(cosYaw, -matrix[3]);
        return;
    }
    if ((sinPitch < 0.001f) && (-0.001f < sinPitch)) {
        *pitch = 0.0f;
        cosYaw = matrix[0];
        sinYaw = matrix[1];
        cosRoll = matrix[8];
        sinRoll = matrix[5];
    } else {
        if (sinPitch < -1.0f) {
            sinPitch = -1.0f;
        } else if (sinPitch > 1.0f) {
            sinPitch = 1.0f;
        }
        cosPitch = sqrtf(1.0f - (sinPitch * sinPitch));
        cosYaw = matrix[0] / cosPitch;
        if (cosYaw < -1.0f) {
            cosYaw = -1.0f;
        } else if (cosYaw > 1.0f) {
            cosYaw = 1.0f;
        }
        sinYaw = sqrtf(1.0f - (cosYaw * cosYaw));
        if (matrix[1] < 0.0f) {
            sinYaw = -sinYaw;
        }
        cosRoll = ((-(sinPitch * sinYaw) * matrix[3]) + (matrix[4] * (sinPitch * cosYaw))) / sinPitch;
        if (cosRoll < -1.0f) {
            cosRoll = -1.0f;
        } else if (cosRoll > 1.0f) {
            cosRoll = 1.0f;
        }
        rollSquared = cosRoll * cosRoll;
        sinRoll = sqrtf(1.0f - rollSquared);
        verticalSign = matrix[5] > 0.0f ? 1 : (matrix[5] < 0.0f ? -1 : 0);
        if ((cosPitch > 0.0f ? 1 : (cosPitch < 0.0f ? -1 : 0)) != verticalSign) {
            sinRoll = -sinRoll;
        }
        *pitch = mathUnitDirectionDegrees(cosPitch, sinPitch);
    }
    *roll = mathUnitDirectionDegrees(cosRoll, sinRoll);
    *yaw = mathUnitDirectionDegrees(cosYaw, sinYaw);
}

f32 mathHypotApprox(f32, f32); /* extern */

/// Normalize a planar vector using an approximate length.
/// \param x First component, updated in place.
/// \param y Second component, updated in place.
/// \returns The approximate original length, or zero for a near-zero vector whose components are cleared.
/// \note Original decompilation identifier: \c func_8000D5E0 (0x8000D5E0).
f32 vector2NormalizeComponentsApprox(f32 *x, f32 *y) {
    f32 length;

    length = mathHypotApprox(*x, *y);
    if (length <= 0.001f) {
        *y = 0.0f;
        *x = 0.0f;
        return 0.0f;
    }
    *x /= length;
    *y /= length;
    return length;
}

/// Rotate a planar vector in place.
/// \param angle Rotation in degrees from the first axis toward the second.
/// \param x First component, updated in place.
/// \param y Second component, updated in place.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D660 (0x8000D660).
void vector2RotateComponentsDegrees(f32 angle, f32 *x, f32 *y) {
    f32 sine;
    f32 cosine;
    f32 originalY;
    f32 originalX;
    cosine = mathCosDegrees(angle);
    sine = mathSinDegrees(angle);
    originalX = *x;
    originalY = *y;
    *x = (originalX * cosine) - (originalY * sine);
    *y = (originalX * sine) + (originalY * cosine);
}

void matrix34TranslateLocal(s32, f32, f32, f32); /* extern */
void matrix34MakeRotationZYXDegrees(s32, f32, f32, f32); /* extern */

/// Construct a view transform from a position and Euler orientation.
/// \param position Three-component viewing position.
/// \param angles Three Euler rotation angles in degrees, in the engine rotation order.
/// \param matrix Address of the twelve-component destination affine matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D6D8 (0x8000D6D8).
void matrix34MakeViewFromEulerDegrees(f32 *position, f32 *angles, s32 matrix) {
    matrix34MakeRotationZYXDegrees(matrix, -angles[0], -angles[1], -angles[2]);
    matrix34TranslateLocal(matrix, -position[0], -position[1], -position[2]);
}

/// Rotate an affine transform about the world Y axis.
/// \param angle Rotation angle in degrees.
/// \param m Twelve-component affine matrix updated in place, including its translation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D764 (0x8000D764).
void matrix34RotateWorldYDegrees(f32 angle, f32 *m) {
    f32 c;
    f32 s;
    f32 x0;
    f32 x1;
    f32 x2;
    f32 x3;
    f32 z0;
    f32 z1;
    f32 z2;
    f32 z3;

    while (angle < 0.0f) {
        angle += 360.0f;
    }
    while (angle >= 360.0f) {
        angle -= 360.0f;
    }
    c = mathCosDegrees(angle);
    s = mathSinDegrees(angle);
    x0 = m[0];
    x1 = m[3];
    x2 = m[6];
    x3 = m[9];
    z0 = m[2];
    z1 = m[5];
    z2 = m[8];
    z3 = m[11];
    m[0] = c * x0 + s * z0;
    m[3] = c * x1 + s * z1;
    m[6] = c * x2 + s * z2;
    m[9] = c * x3 + s * z3;
    m[2] = -s * x0 + c * z0;
    m[5] = -s * x1 + c * z1;
    m[8] = -s * x2 + c * z2;
    m[11] = -s * x3 + c * z3;
}

/// Find the first table entry at least as large as a requested value.
/// \param table Integer table containing a qualifying entry before its end.
/// \param value Minimum qualifying value.
/// \returns The zero-based index of the first qualifying entry.
/// \note Original decompilation identifier: \c func_8000D8E4 (0x8000D8E4).
s32 integerTableFindFirstAtLeast(s32 *table, s32 value) {
    s32 i;

    i = 0;
    while (*table < value) {
        i++;
        table++;
    }
    return i;
}

/// Blend two orientations into an affine matrix with zero translation.
/// \param w Weight of the second orientation, normally from zero through one.
/// \param a First input orientation matrix.
/// \param b Second input orientation matrix.
/// \param out Receives a twelve-component matrix with unit nondegenerate basis vectors; near-zero blended vectors are cleared.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D918 (0x8000D918).
void matrix34BlendOrientation(f32 w, f32 *a, f32 *b, f32 *out) {
    f32 *left = a;
    f32 *right = b;
    f32 *dst = out;
    s32 i;
    f32 complement;
    complement = 1.0f - w;
    for (i = 0; i < 9; i++) {
        *dst = complement * *left + *right * w;
        left++;
        right++;
        dst++;
    }
    vector3Normalize(out);
    vector3Normalize(out + 3);
    vector3Normalize(out + 6);
    out[9] = 0.0f;
    out[10] = 0.0f;
    out[11] = 0.0f;
}

/// Get the horizontal direction of a point after an affine transformation.
/// \param matrix Input affine transformation matrix.
/// \param point Input three-component point.
/// \returns Yaw in degrees from positive Z toward positive X, or zero for a near-zero horizontal result.
/// \note Original decompilation identifier: \c func_8000DAB8 (0x8000DAB8).
f32 matrix34GetTransformedPointYawDegrees(f32 *matrix, f32 *point) {
    f32 transformedPoint[3];
    matrix34TransformPointVector(matrix, point, transformedPoint);
    return vector3GetYawDegrees(transformedPoint);
}

/// Evaluate a cubic Hermite interpolation over a timed interval.
/// \param start Value at the start of the interval.
/// \param end Value at the end of the interval.
/// \param startSlope Rate of change at the start.
/// \param endSlope Rate of change at the end.
/// \param duration Nonzero interval duration.
/// \param time Evaluation time relative to the interval start.
/// \returns The interpolated value.
/// \note Original decompilation identifier: \c func_8000DAE0 (0x8000DAE0).
f32 mathCubicHermite(f32 start, f32 end, f32 startSlope, f32 endSlope, f32 duration, f32 time) {
    f32 inverse;
    f32 quadratic;
    f32 timeSquared;
    f32 cubic;
    f32 cubicPosition;
    f32 quadraticSlope;
    f32 endWeight;
    f32 startWeight;
    f32 endSlopeWeight;
    f32 startSlopeWeight;
    inverse = 1.0 / duration;
    /* Build the quadratic basis coefficient from the squared inverse duration. */
    quadratic = inverse * inverse;
    timeSquared = time * time;
    cubic = timeSquared * time * quadratic;
    cubicPosition = 2.0 * cubic * inverse;
    quadratic = timeSquared * 3.0 * quadratic;
    quadraticSlope = timeSquared * inverse;
    endSlopeWeight = cubic - quadraticSlope;
    startSlopeWeight = endSlopeWeight - quadraticSlope + time;
    startWeight = cubicPosition - quadratic + 1.0f;
    endWeight = quadratic - cubicPosition;
    return startWeight * start + end * endWeight + startSlope * startSlopeWeight + endSlope * endSlopeWeight;
}

f32 mathCubicHermite(f32, f32, f32, f32, f32, f32);

/// Interpolate smoothly between two positions with zero endpoint velocities.
/// \param a Starting three-component position.
/// \param b Ending three-component position.
/// \param out Receives the interpolated position.
/// \param len Positive interpolation duration.
/// \param t Elapsed interpolation time; values beyond the duration reach the endpoint, while negative values extrapolate.
/// \returns Minus one when the result reaches the endpoint within tolerance; otherwise zero.
/// \note Original decompilation identifier: \c func_8000DBE4 (0x8000DBE4).
s32 vector3InterpolateSmoothTimed(f32 *a, f32 *b, f32 *out, f32 len, f32 t) {
    f32 dist;
    f32 d[3];

    if (len < t) {
        t = len;
    }
    out[0] = mathCubicHermite(a[0], b[0], 0.0f, 0.0f, len, t);
    out[1] = mathCubicHermite(a[1], b[1], 0.0f, 0.0f, len, t);
    out[2] = mathCubicHermite(a[2], b[2], 0.0f, 0.0f, len, t);
    d[0] = b[0] - out[0];
    d[1] = b[1] - out[1];
    d[2] = b[2] - out[2];
    dist = vector3Length(d);
    if (dist < 0.001f && -0.001f < dist) {
        out[0] = b[0];
        out[1] = b[1];
        out[2] = b[2];
        return -1;
    }
    return 0;
}

/// Raise a floating-point value to an integer power.
/// \param base Base value; must be nonzero for a negative exponent.
/// \param exponent Integer exponent, excluding the minimum signed integer.
/// \returns The base raised to the requested power.
/// \note Original decompilation identifier: \c func_8000DD40 (0x8000DD40).
f32 mathPowInteger(f32 base, s32 exponent) {
    f32 var_f2;
    s32 var_v0;

    var_v0 = (exponent >= 0) ? exponent : -exponent;
    var_f2 = 1.0f;
    for (; var_v0 != 0; var_v0 >>= 1) {
        if (var_v0 & 1) {
            var_f2 *= base;
        }
        base *= base;
    }
    if (exponent >= 0) {
        return var_f2;
    }
    return 1.0f / var_f2;
}

/// Compute the standard CRC-32 checksum of a byte buffer.
/// \param len Number of bytes to checksum; nonpositive lengths produce the empty checksum.
/// \param buf Input byte buffer containing at least len bytes when len is positive.
/// \returns The CRC-32 checksum.
/// \note Original decompilation identifier: \c func_8000DD9C (0x8000DD9C).
u32 checksumCrc32(s32 len, u8 *buf) {
    u32 c;
    s32 i;
    s32 j;

    c = 0xFFFFFFFF;
    for (i = 0; i < len; i++) {
        c ^= buf[i];
        for (j = 0; j < 8; j++) {
            if (c & 1) {
                c >>= 1;
                c ^= 0xEDB88320;
            } else {
                c >>= 1;
            }
        }
    }
    return c ^ 0xFFFFFFFF;
}

/// Exclusive upper bound associated with the retained random-selection history.
/// \note Original identifier: \c D_8006B580 (0x8006B580).
extern s32 gRandomHistoryRange;
/// Retained ordering of up to 32 choices used by history-aware random selection.
/// \note Original identifier: \c D_800B6810 (0x800B6810).
extern u8 gRandomSelectionHistory[32];

/// Choose a random index while accounting for recent selections in small ranges.
/// \param n Positive exclusive upper bound; ranges through 32 retain selection history.
/// \returns A selected index from zero through n minus one.
/// \note Original decompilation identifier: \c func_8000DE58 (0x8000DE58).
s32 randomBelowWithHistory(s32 n) {
    s32 i;
    s32 j;
    s32 r;
    u8 t;
    u8 prev;

    if (n <= 0 || n > sizeof(gRandomSelectionHistory)) {
        return randomBelow(n);
    }
    if (n != gRandomHistoryRange) {
        for (i = 0; i < n; i++) {
            gRandomSelectionHistory[i] = i;
            j = randomBelow(i + 1);
            prev = gRandomSelectionHistory[j];
            gRandomSelectionHistory[j] = gRandomSelectionHistory[i];
            gRandomSelectionHistory[i] = prev;
        }
        gRandomHistoryRange = n;
    }
    r = randomBelow(n * n) - 1;
    prev = gRandomSelectionHistory[0];
    for (i = 1; r >= 0;) {
        i++;
        r = r - i - i + 1;
        t = prev;
        prev = gRandomSelectionHistory[i - 1];
        gRandomSelectionHistory[i - 1] = t;
    }
    gRandomSelectionHistory[0] = prev;
    return prev;
}
