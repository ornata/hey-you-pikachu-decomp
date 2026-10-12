#include "matrix_translation.h"
#include "matrix_orientation.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "vector_direction.h"
#include "matrix_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata and the dead branch still ends a scheduling block. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on


/// Retained four-by-four floating-point identity matrix; no reader is identified.
/// \note Original identifier: \c D_800A6CC0 (0x800A6CC0).
const f32 gMatrix44Identity[4][4] = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
};
/// Affine identity transform copied by matrix initialization.
/// \note Original identifier: \c D_800A6D00 (0x800A6D00).
const Matrix34 gMatrix34Identity = {{
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f},
}};
const s32 D_800A6D30[3][3] = {
    {1, 1, 0},
    {1, 1, 1},
    {0, 1, 1},
};

f32 mathUnitDirectionDegrees(f32, f32);

f32 __cosf(f32); /* extern */
f32 __sinf(f32); /* extern */

/// Compute the tangent of an angle.
/// \param angle Angle in radians.
/// \returns Tangent of the angle.
/// \note Original decompilation identifier: \c func_80038B50 (0x80038B50).
f32 mathTanRadians(f32 angle) {
    f32 sp1C;

    sp1C = __sinf(angle);
    return sp1C / __cosf(angle);
}

/// Compute the inverse tangent of a value.
/// \param x Value whose inverse tangent is required.
/// \returns Inverse tangent in radians.
/// \note Original decompilation identifier: \c func_80038B84 (0x80038B84).
f32 mathAtanRadians(f32 x) {
    s32 fold;
    f32 x2;
    f32 r;

    if (x == 0.0f) {
        return 0.0f;
    }
    if (x > 1.0) {
        fold = 1;
        x = 1.0f / x;
    } else if (x < -1.0) {
        fold = 2;
        x = 1.0f / x;
    } else {
        fold = 0;
    }
    x2 = x * x;
    r = (x2 / (x2 / (x2 / (x2 / (x2 / (x2 / -0.10810675f + -44.57192f) + -0.1619081f) + -15.774018f) + -0.55556977f) +
               -3.000003f) +
         1.0f) *
        x;
    switch (fold) {
    case 0:
        return r;
    case 1:
        return 1.5707963267948966 - r;
    case 2:
        return -1.5707963267948966 - r;
    }
    return 0.0f;
}

f32 fabsf(f32);
#pragma intrinsic(fabsf)

#define SIGN(v) ((v) > 0.0f ? 1 : ((v) < 0.0f ? -1 : 0))

/// Compute a signed planar direction angle.
/// \param y Vertical component of the direction.
/// \param x Horizontal component of the direction.
/// \returns Direction in radians; a zero Y component produces zero.
/// \note Original decompilation identifier: \c func_80038CDC (0x80038CDC).
f32 mathDirectionRadians(f32 y, f32 x) {
    if (x > 0.0f) {
        return mathAtanRadians(y / x);
    }
    if (x < 0.0f) {
        return SIGN(y) * (3.141592653589793 - mathAtanRadians(fabsf(y / x)));
    }
    if (y != 0.0f) {
        return SIGN(y) * 1.5707963267948966;
    }
    return 0.0f;
}

#undef SIGN

/// Compute an inverse sine with saturation near the domain endpoints.
/// \param value Input ratio, saturated near either end of [-1, 1].
/// \returns Inverse sine in radians.
/// \note Original decompilation identifier: \c func_80038DFC (0x80038DFC).
f32 mathAsinClampedRadians(f32 value) {
    f64 wideValue;

    wideValue = (f64)value;
    if (0.99999 < wideValue) {
        return 1.5707964f;
    }
    if (wideValue < -0.99999) {
        return -1.5707964f;
    }
    return mathAtanRadians(value / sqrtf(1.0f - (value * value)));
}

/// Compute an inverse cosine with saturation near the domain endpoints.
/// \param value Input ratio, saturated near either end of [-1, 1].
/// \returns Inverse cosine in radians.
/// \note Original decompilation identifier: \c func_80038E80 (0x80038E80).
f32 mathAcosClampedRadians(f32 value) {
    f64 wideValue;

    wideValue = (f64)value;
    if (0.99999 < wideValue) {
        return 0.0f;
    }
    if (wideValue < -0.99999) {
        return 3.1415927f;
    }
    return (f32)(1.5707963267948966 - (f64)mathAtanRadians(value / sqrtf(1.0f - (value * value))));
}

/// Compute the cosecant of an angle.
/// \param angle Angle in radians.
/// \returns Cosecant of the angle.
/// \note Original decompilation identifier: \c func_80038F18 (0x80038F18).
f32 mathCosecantRadians(f32 angle) {
    return 1.0f / __sinf(angle);
}

/// Compute the secant of an angle.
/// \param angle Angle in radians.
/// \returns Secant of the angle.
/// \note Original decompilation identifier: \c func_80038F40 (0x80038F40).
f32 mathSecantRadians(f32 angle) {
    return 1.0f / __cosf(angle);
}

f32 mathTanRadians(f32);

/// Compute the cotangent of an angle.
/// \param angle Angle in radians.
/// \returns Cotangent of the angle.
/// \note Original decompilation identifier: \c func_80038F68 (0x80038F68).
f32 mathCotangentRadians(f32 angle) {
    return 1.0f / mathTanRadians(angle);
}

/// Provide the disabled diagnostic report for an affine matrix.
/// \param m Diagnostic matrix; unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038F90 (0x80038F90).
void matrix34DebugReport(Matrix34 *m) {
    s32 i;
    s32 j;

    TRACE("Matrix34 %p\n", m);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            TRACE("%5.3f ", m->m[j][i]);
        }
        TRACE("\n");
    }
}

/// Provide the disabled diagnostic report for a 4x4 floating-point matrix.
/// \param m Diagnostic matrix; unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038FB0 (0x80038FB0).
void matrix44DebugReport(f32 (*m)[4]) {
    s32 i;
    s32 j;

    TRACE("Matrix %p\n", m);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            TRACE("%5.3f ", m[i][j]);
        }
        TRACE("\n");
    }
}

/// Provide the disabled diagnostic report for a fixed-point matrix.
/// \param m Diagnostic matrix; unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038FD0 (0x80038FD0).
void matrixFixedDebugReport(Mtx *m) {
    s32 i;
    s32 j;

    TRACE("Mtx %p\n", m);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            TRACE("%5.3f ", m->m[i][j] / 65536.0f);
        }
        TRACE("\n");
    }
}

/// Compose two linear transforms as b times a.
/// \param a First input 3x3 matrix.
/// \param b Second input 3x3 matrix.
/// \param c Output 3x3 matrix; may alias either input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038FF0 (0x80038FF0).
void matrix33Multiply(f32 *a, f32 *b, f32 *c) {
    f32 ax0 = a[0];
    f32 ax1 = a[3];
    f32 ax2 = a[6];
    f32 ay0 = a[1];
    f32 ay1 = a[4];
    f32 ay2 = a[7];
    f32 az0 = a[2];
    f32 az1 = a[5];
    f32 az2 = a[8];
    f32 bx0 = b[0];
    f32 bx1 = b[3];
    f32 bx2 = b[6];
    f32 by0 = b[1];
    f32 by1 = b[4];
    f32 by2 = b[7];
    f32 bz0 = b[2];
    f32 bz1 = b[5];
    f32 bz2 = b[8];

    c[0] = bx0 * ax0 + bx1 * ay0 + bx2 * az0;
    c[3] = bx0 * ax1 + bx1 * ay1 + bx2 * az1;
    c[6] = bx0 * ax2 + bx1 * ay2 + bx2 * az2;
    c[1] = by0 * ax0 + by1 * ay0 + by2 * az0;
    c[4] = by0 * ax1 + by1 * ay1 + by2 * az1;
    c[7] = by0 * ax2 + by1 * ay2 + by2 * az2;
    c[2] = bz0 * ax0 + bz1 * ay0 + bz2 * az0;
    c[5] = bz0 * ax1 + bz1 * ay1 + bz2 * az1;
    c[8] = bz0 * ax2 + bz1 * ay2 + bz2 * az2;
}

/// Compose an affine transform with a 4x4 transform as b times a.
/// \param a Input affine matrix.
/// \param b Input 4x4 matrix.
/// \param c Output 4x4 matrix with room for sixteen floats; may share input storage.
/// \returns Nothing.
/// \note Portability: Adjacent column-result statements retain the IDO scheduling required by the ROM.
/// \note Original decompilation identifier: \c func_800391FC (0x800391FC).
void matrix34ComposeWith44(f32 *a, f32 *b, f32 *c) {
    f32 a00 = a[0], a10 = a[3], a20 = a[6], a30 = a[9];
    f32 a01 = a[1], a11 = a[4], a21 = a[7], a31 = a[10];
    f32 a02 = a[2], a12 = a[5], a22 = a[8], a32 = a[11];
    f32 b00 = b[0], b10 = b[4], b20 = b[8], b30 = b[12];
    f32 b01 = b[1], b11 = b[5], b21 = b[9], b31 = b[13];
    f32 b02 = b[2], b12 = b[6], b22 = b[10], b32 = b[14];
    f32 b03 = b[3], b13 = b[7], b23 = b[11], b33 = b[15];
    c[0] = b00 * a00 + b10 * a01 + b20 * a02;
    c[4] = b00 * a10 + b10 * a11 + b20 * a12;
    c[8] = b00 * a20 + b10 * a21 + b20 * a22;
    c[12] = b00 * a30 + b10 * a31 + b20 * a32 + b30;
    c[1] = b01 * a00 + b11 * a01 + b21 * a02;
    c[5] = b01 * a10 + b11 * a11 + b21 * a12;
    c[9] = b01 * a20 + b11 * a21 + b21 * a22;
    c[13] = b01 * a30 + b11 * a31 + b21 * a32 + b31;
    c[2] = b02 * a00 + b12 * a01 + b22 * a02;
    c[6] = b02 * a10 + b12 * a11 + b22 * a12;
    c[10] = b02 * a20 + b12 * a21 + b22 * a22;
    c[14] = b02 * a30 + b12 * a31 + b22 * a32 + b32;
    c[3] = b03 * a00 + b13 * a01 + b23 * a02;
    // clang-format off
    c[7] = b03 * a10 + b13 * a11 + b23 * a12; c[11] = b03 * a20 + b13 * a21 + b23 * a22;
    // clang-format on
    c[15] = b03 * a30 + b13 * a31 + b23 * a32 + b33;
}

/// Transform a three-dimensional point.
/// \param mf Input affine matrix.
/// \param x Input point X coordinate.
/// \param y Input point Y coordinate.
/// \param z Input point Z coordinate.
/// \param ox Receives the transformed X coordinate.
/// \param oy Receives the transformed Y coordinate.
/// \param oz Receives the transformed Z coordinate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003953C (0x8003953C).
void matrix34TransformPoint(f32 mf[4][3], f32 x, f32 y, f32 z, f32 *ox, f32 *oy, f32 *oz) {
    *ox = mf[0][0] * x + mf[1][0] * y + mf[2][0] * z + mf[3][0];
    *oy = mf[0][1] * x + mf[1][1] * y + mf[2][1] * z + mf[3][1];
    *oz = mf[0][2] * x + mf[1][2] * y + mf[2][2] * z + mf[3][2];
}

/// Transform a three-dimensional direction vector.
/// \param matrix Input 3x3 matrix.
/// \param vector Input three-component vector.
/// \param output Output three-component vector; may alias vector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800395E0 (0x800395E0).
void matrix33TransformVector(f32 *matrix, f32 *vector, f32 *output) {
    f32 x = vector[0];
    f32 y = vector[1];
    f32 z = vector[2];

    output[0] = matrix[0] * x + matrix[3] * y + matrix[6] * z;
    output[1] = matrix[1] * x + matrix[4] * y + matrix[7] * z;
    output[2] = matrix[2] * x + matrix[5] * y + matrix[8] * z;
}

/// Transform a point with signed-byte coordinates.
/// \param mf Input affine matrix.
/// \param in Input three-component signed-byte point.
/// \param out Output three-component point; transformed coordinates must fit in signed bytes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039664 (0x80039664).
void matrix34TransformBytePoint(f32 mf[4][3], s8 *in, s8 *out) {
    s32 x = in[0];
    s32 y = in[1];
    s32 z = in[2];

    out[0] = mf[0][0] * x + mf[1][0] * y + mf[2][0] * z + mf[3][0];
    out[1] = mf[0][1] * x + mf[1][1] * y + mf[2][1] * z + mf[3][1];
    out[2] = mf[0][2] * x + mf[1][2] * y + mf[2][2] * z + mf[3][2];
}

/// Apply a translation in the local coordinate system.
/// \param mf Affine matrix to update.
/// \param x Local X displacement.
/// \param y Local Y displacement.
/// \param z Local Z displacement.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003973C (0x8003973C).
void matrix34TranslateLocal(f32 mf[4][3], f32 x, f32 y, f32 z) {
    if (x != 0.0f || y != 0.0f || z != 0.0f) {
        mf[3][0] += mf[0][0] * x + mf[1][0] * y + mf[2][0] * z;
        mf[3][1] += mf[0][1] * x + mf[1][1] * y + mf[2][1] * z;
        mf[3][2] += mf[0][2] * x + mf[1][2] * y + mf[2][2] * z;
    }
}


/// Apply a local Euler rotation in X, then Y, then Z order.
/// \param matrixAddress Matrix whose linear transform is updated in place.
/// \param xDegrees Rotation about the X axis in degrees.
/// \param yDegrees Rotation about the Y axis in degrees.
/// \param zDegrees Rotation about the Z axis in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039814 (0x80039814).
void matrix33ApplyRotationXYZDegrees(f32 *matrixAddress, f32 xDegrees, f32 yDegrees, f32 zDegrees) {
    f32 sp18[12];

    if ((xDegrees != 0.0f) || (yDegrees != 0.0f) || (zDegrees != 0.0f)) {
        matrix34MakeRotationXYZDegrees(sp18, xDegrees, yDegrees, zDegrees);
        matrix33Multiply(sp18, matrixAddress, matrixAddress);
    }
}

void matrix34MakeRotationZYXDegrees(f32 *, f32, f32, f32);

/// Apply a local Euler rotation in Z, then Y, then X order.
/// \param matrixAddress Matrix whose linear transform is updated in place.
/// \param xDegrees Rotation about the X axis in degrees.
/// \param yDegrees Rotation about the Y axis in degrees.
/// \param zDegrees Rotation about the Z axis in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003988C (0x8003988C).
void matrix33ApplyRotationZYXDegrees(f32 *matrixAddress, f32 xDegrees, f32 yDegrees, f32 zDegrees) {
    f32 sp18[12];

    if ((xDegrees != 0.0f) || (yDegrees != 0.0f) || (zDegrees != 0.0f)) {
        matrix34MakeRotationZYXDegrees(sp18, xDegrees, yDegrees, zDegrees);
        matrix33Multiply(sp18, matrixAddress, matrixAddress);
    }
}

/// Apply scale factors along the local axes.
/// \param matrix Matrix whose linear transform is to be updated.
/// \param xScale Scale along the local X axis.
/// \param yScale Scale along the local Y axis.
/// \param zScale Scale along the local Z axis.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039904 (0x80039904).
void matrix33ScaleLocal(f32 *matrix, f32 xScale, f32 yScale, f32 zScale) {
    matrix[0] = (f32)(matrix[0] * xScale);
    matrix[3] = (f32)(matrix[3] * yScale);
    matrix[6] = (f32)(matrix[6] * zScale);
    matrix[1] = (f32)(matrix[1] * xScale);
    matrix[4] = (f32)(matrix[4] * yScale);
    matrix[7] = (f32)(matrix[7] * zScale);
    matrix[2] = (f32)(matrix[2] * xScale);
    matrix[5] = (f32)(matrix[5] * yScale);
    matrix[8] = (f32)(matrix[8] * zScale);
}

f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
void matrix34Identity();

/// Build an affine rotation applying X, then Y, then Z rotations.
/// \param m Output affine matrix.
/// \param x Rotation about the X axis in degrees.
/// \param y Rotation about the Y axis in degrees.
/// \param z Rotation about the Z axis in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039994 (0x80039994).
void matrix34MakeRotationXYZDegrees(f32 *m, f32 x, f32 y, f32 z) {
    f32 sx;
    f32 sy;
    f32 sz;
    f32 cx;
    f32 cy;
    f32 cz;

    if (x == 0.0f && y == 0.0f && z == 0.0f) {
        matrix34Identity(m);
        return;
    }
    sx = mathSinDegrees(x);
    cx = mathCosDegrees(x);
    sy = mathSinDegrees(y);
    cy = mathCosDegrees(y);
    sz = mathSinDegrees(z);
    cz = mathCosDegrees(z);
    m[9] = 0.0f;
    m[10] = 0.0f;
    m[11] = 0.0f;
    m[0] = cy * cz;
    m[1] = cy * sz;
    m[2] = -sy;
    m[3] = sx * sy * cz - cx * sz;
    m[4] = sx * sy * sz + cx * cz;
    m[5] = sx * cy;
    m[6] = cx * sy * cz + sx * sz;
    m[7] = cx * sy * sz - sx * cz;
    m[8] = cx * cy;
}

/// Build an affine rotation applying Z, then Y, then X rotations.
/// \param m Output affine matrix.
/// \param x Rotation about the X axis in degrees.
/// \param y Rotation about the Y axis in degrees.
/// \param z Rotation about the Z axis in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039B04 (0x80039B04).
void matrix34MakeRotationZYXDegrees(f32 *m, f32 x, f32 y, f32 z) {
    f32 sx;
    f32 sy;
    f32 sz;
    f32 cx;
    f32 cy;
    f32 cz;

    if (x == 0.0f && y == 0.0f && z == 0.0f) {
        matrix34Identity(m);
        return;
    }
    sx = mathSinDegrees(x);
    cx = mathCosDegrees(x);
    sy = mathSinDegrees(y);
    cy = mathCosDegrees(y);
    sz = mathSinDegrees(z);
    cz = mathCosDegrees(z);
    m[9] = 0.0f;
    m[10] = 0.0f;
    m[11] = 0.0f;
    m[6] = sy;
    m[0] = cy * cz;
    m[1] = sx * sy * cz + cx * sz;
    m[2] = -cx * sy * cz + sx * sz;
    m[3] = -cy * sz;
    m[4] = -sx * sy * sz + cx * cz;
    m[5] = cx * sy * sz + sx * cz;
    m[7] = -sx * cy;
    m[8] = cx * cy;
}

/// Extract the affine components of a 4x4 matrix.
/// \param source Input 4x4 matrix.
/// \param destination Output affine matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039C84 (0x80039C84).
void matrix44To34(f32 *source, f32 *destination) {
    f32 *src = source;
    f32 *dst = destination;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            *dst++ = *src++;
        }
        src++;
    }
}

/// Expand an affine matrix to a 4x4 matrix.
/// \param source Input affine matrix.
/// \param destination Output sixteen-float matrix; must not overlap source.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039CC8 (0x80039CC8).
void matrix34To44(f32 *source, f32 *destination) {
    f32 *src = source;
    f32 *dst = destination;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            *dst++ = *src++;
        }
        dst++;
    }
    destination[0xB] = 0.0f;
    destination[7] = 0.0f;
    destination[3] = 0.0f;
    destination[0xF] = 1.0f;
}

/// Convert the affine components of a fixed-point matrix to floating point.
/// \param m Input fixed-point matrix.
/// \param out Output floating-point affine matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039D24 (0x80039D24).
void matrixFixedTo34(Mtx m, f32 *out) {
    s16 *ip = (s16 *)&m;
    u16 *fp = (u16 *)&m + 16;
    f32 *dst = out;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            *dst++ = *ip++ + (u32)*fp++ * (1.0f / 65536.0f);
        }
        ip++;
        fp++;
    }
}

/// Convert a floating-point 4x4 matrix to fixed point.
/// \param mf Input sixteen-float matrix with values representable in signed 16.16 fixed point.
/// \param m Output fixed-point matrix; must not overlap mf.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039DBC (0x80039DBC).
void matrix44ToFixed(f32 *mf, Mtx *m) {
    s16 *ip = (s16 *)m;
    u16 *fp = (u16 *)m + 16;
    f32 *src = mf;
    s32 i;
    s32 j;
    s32 e;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            e = *src++ * 65536.0f;
            *ip++ = e >> 16;
            *fp++ = e & 0xFFFF;
        }
    }
}

typedef struct FixedMatrixParts {
    s16 integer[16];
    u16 fraction[16];
} FixedMatrixParts;
/// Convert a fixed-point matrix to a floating-point 4x4 matrix.
/// \param matrix Input fixed-point matrix.
/// \param out Output floating-point 4x4 matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039F0C (0x80039F0C).
void matrixFixedPartsTo44(FixedMatrixParts matrix, f32 *out) {
    s16 *integer = matrix.integer;
    u16 *fraction = matrix.fraction;
    f32 *dest = out;
    s32 i;
    s32 j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            *dest++ = *integer++ + (u32)*fraction++ * (1.0f / 65536.0f);
        }
    }
}

/// Set an affine matrix to the identity transform.
/// \param m Output affine matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003A058 (0x8003A058).
void matrix34Identity(Matrix34 *m) {
    *m = gMatrix34Identity;
}

/// Replace a linear transform with its nonnegative axis scales.
/// \param m 3x3 matrix to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003A094 (0x8003A094).
void matrix33KeepScaleOnly(f32 *m) {
    f32 pad;
    f32 x0 = m[0];
    f32 x1 = m[3];
    f32 x2 = m[6];
    f32 y0 = m[1];
    f32 y1 = m[4];
    f32 y2 = m[7];
    f32 pad2;
    f32 z0 = m[2];
    f32 z1 = m[5];
    f32 z2 = m[8];

    m[0] = sqrtf(x0 * x0 + y0 * y0 + z0 * z0);
    m[4] = sqrtf(x1 * x1 + y1 * y1 + z1 * z1);
    m[8] = sqrtf(x2 * x2 + y2 * y2 + z2 * z2);
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[5] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
}

/// Transpose a 3x3 matrix in place.
/// \param p 3x3 matrix to transpose.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003A158 (0x8003A158).
void matrix33Transpose(f32 *p) {
    f32 t0, t1, t2;

    // clang-format off
    t0 = p[1]; p[1] = p[3]; p[3] = t0;
    t1 = p[2]; p[2] = p[6]; p[6] = t1;
    t2 = p[5]; p[5] = p[7]; p[7] = t2;
    // clang-format on
}

/* Round t to the nearest integer and wrap it into [0, 0xFFFF). */
#define ROUND_WRAP(v, t)                                                                                               \
    do {                                                                                                               \
        if ((t) >= 0) {                                                                                                \
            (v) = (t) + 0.5;                                                                                           \
        } else {                                                                                                       \
            (v) = (t) - 0.5;                                                                                           \
        }                                                                                                              \
        while ((v) < 0) {                                                                                              \
            (v) += 0xFFFF;                                                                                             \
        }                                                                                                              \
        while ((v) >= 0xFFFF) {                                                                                        \
            (v) -= 0xFFFF;                                                                                             \
        }                                                                                                              \
    } while (0)

/// Approximate the cosine of a radian angle.
/// \param rad Angle in radians whose rounded value in 65535 units per turn fits a signed 32-bit integer.
/// \returns Approximate cosine of the angle.
/// \note Portability: The ROUND_WRAP block preserves the IDO scheduling required by the ROM.
/// \note Original decompilation identifier: \c func_8003A18C (0x8003A18C).
f32 mathCosRadiansQuantized(f32 rad) {
    f64 t = rad * 10430.060040584269;
    s32 v;

    ROUND_WRAP(v, t);
    return coss(v) * 3.0518509e-05f;
}

/// Approximate the sine of a radian angle.
/// \param rad Angle in radians whose rounded value in 65535 units per turn fits a signed 32-bit integer.
/// \returns Approximate sine of the angle.
/// \note Portability: The ROUND_WRAP block preserves the IDO scheduling required by the ROM.
/// \note Original decompilation identifier: \c func_8003A26C (0x8003A26C).
f32 mathSinRadiansQuantized(f32 rad) {
    f64 t = rad * 10430.060040584269;
    s32 v;

    ROUND_WRAP(v, t);
    return sins(v) * 3.0518509e-05f;
}

#undef ROUND_WRAP

/// Measure the overlap of two directed angular intervals.
/// \param start Start of the first interval in degrees.
/// \param end End of the first interval in degrees.
/// \param otherStart Start of the second interval in degrees.
/// \param otherEnd End of the second interval in degrees.
/// \returns Overlapping angular extent in degrees.
/// \note Original decompilation identifier: \c func_8003A34C (0x8003A34C).
f32 mathAngularIntervalOverlapDegrees(f32 start, f32 end, f32 otherStart, f32 otherEnd) {

    end = end - start;
    otherStart = otherStart - start;
    if (end < 0.0f) {
        do {
            end += 360.0f;
        } while (end < 0.0f);
    }
    otherEnd = otherEnd - start;
    if (end >= 360.0f) {
        do {
            end -= 360.0f;
        } while (end >= 360.0f);
    }
    if (otherStart < 0.0f) {
        do {
            otherStart += 360.0f;
        } while (otherStart < 0.0f);
    }
    if (otherStart >= 360.0f) {
        do {
            otherStart -= 360.0f;
        } while (otherStart >= 360.0f);
    }
    if (otherEnd < 0.0f) {
        do {
            otherEnd += 360.0f;
        } while (otherEnd < 0.0f);
    }
    if (otherEnd >= 360.0f) {
        do {
            otherEnd -= 360.0f;
        } while (otherEnd >= 360.0f);
    }
    if (otherStart <= otherEnd) {
        if (end <= otherStart) {
            return 0.0f;
        }
        if (end <= otherEnd) {
            return end - otherStart;
        }
        return otherEnd - otherStart;
    }
    if (end <= otherEnd) {
        return end;
    }
    if (end <= otherStart) {
        return otherEnd;
    }
    return (otherEnd + end) - otherStart;
}

/// Convert a normalized direction to signed-byte components.
/// \param source Input three-component direction with components in [-1, 1].
/// \param destination Output storage for three signed-byte components.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003A4E4 (0x8003A4E4).
void vector3PackNormalBytes(f32 *source, u8 *destination) {
    f32 *src = source;
    u8 *dst = destination;
    s32 i;

    for (i = 0; i < 3; i++) {
        *dst++ = (s8)(s32)(*src++ * 127.0f);
    }
}

/// Convert signed-byte normal components to a floating-point direction.
/// \param src Input three-component signed-byte normal.
/// \param dst Output three-component floating-point direction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003A55C (0x8003A55C).
void vector3UnpackNormalBytes(s8 *src, f32 *dst) {
    f32 *d;
    s8 *s;
    s32 i;

    d = dst;
    s = src;
    for (i = 0; i < 3; i++) {
        *d = *s * 0.007874016f;
        d++;
        s++;
    }
}

/// Compute the length of a three-dimensional vector.
/// \param vector Input three-component vector.
/// \returns Vector length.
/// \note Original decompilation identifier: \c func_8003A5D0 (0x8003A5D0).
f32 vector3Length(f32 *vector) {
    f32 y;
    f32 z;
    f32 x;

    x = vector[0];
    y = vector[1];
    z = vector[2];
    return sqrtf((x * x) + (y * y) + (z * z));
}

f32 mathHypotApprox(f32, f32);

/// Approximate the length of a three-dimensional vector.
/// \param vector Input three-component vector.
/// \returns Approximate vector length.
/// \note Original decompilation identifier: \c func_8003A5FC (0x8003A5FC).
f32 vector3LengthApprox(u8 *vector) {
    return mathHypotApprox(mathHypotApprox(*(f32 *)(vector + 0), *(f32 *)(vector + 4)), *(f32 *)(vector + 8));
}

/// Compute the length of a planar vector.
/// \param x First vector component.
/// \param y Second vector component.
/// \returns Vector length.
/// \note Original decompilation identifier: \c func_8003A634 (0x8003A634).
f32 mathHypot(f32 x, f32 y) {
    return sqrtf((x * x) + (y * y));
}

/// Compute a planar vector length with improved numeric range.
/// \param x First vector component.
/// \param y Second vector component.
/// \returns Vector length.
/// \note Original decompilation identifier: \c func_8003A64C (0x8003A64C).
f32 mathHypotScaled(f32 x, f32 y) {
    f32 yOverX;
    f32 xOverY;
    if (x < 0.0f) {
        x = -x;
    }
    if (y < 0.0f) {
        y = -y;
    }
    if (x < y) {
        xOverY = x / y;
        return sqrtf((xOverY * xOverY) + 1.0f) * y;
    }
    if (x == 0.0f) {
        return 0.0f;
    }
    yOverX = y / x;
    return sqrtf((yOverX * yOverX) + 1.0f) * x;
}

/// Approximate the length of a planar vector.
/// \param a First vector component.
/// \param b Second vector component.
/// \returns Approximate vector length.
/// \note Original decompilation identifier: \c func_8003A6F0 (0x8003A6F0).
f32 mathHypotApprox(f32 a, f32 b) {
    f32 swap;
    f32 ratio;
    f32 square;
    f32 correction;
    if (a < 0.0f)
        a = -a;
    if (b < 0.0f)
        b = -b;
    if (a < b) {
        swap = a;
        a = b;
        b = swap;
    }
    if (a == 0.0f)
        return 0.0f;
    ratio = b / a;
    square = ratio * ratio;
    correction = square / (4.0f + square);
    a += 2 * a * correction;
    b *= correction;
    ratio = b / a;
    square = ratio * ratio;
    correction = square / (4.0f + square);
    a += 2 * a * correction;
    b *= correction;
    return a;
}

/// Normalize a direction, clearing vectors too small to normalize.
/// \param vector Three-component vector to update.
/// \returns Original length, or zero when the vector length is at most 0.001.
/// \note Original decompilation identifier: \c func_8003A7C0 (0x8003A7C0).
f32 vector3Normalize(f32 *vector) {
    f32 length;
    f32 inverseLength;

    length = vector3Length(vector);
    if (length <= 0.001f) {
        vector[2] = 0.0f;
        vector[1] = 0.0f;
        vector[0] = 0.0f;
        return 0.0f;
    }
    inverseLength = 1.0f / length;
    vector[0] = (f32)(vector[0] * inverseLength);
    vector[1] = (f32)(vector[1] * inverseLength);
    vector[2] = (f32)(vector[2] * inverseLength);
    return length;
}

/// Compute an oriented unit normal from three triangle vertices.
/// \param a X coordinates of the three vertices, in triangle order.
/// \param b Y coordinates of the three vertices, in triangle order.
/// \param c Z coordinates of the three vertices, in triangle order.
/// \param n Output unit normal, or a zero vector for a degenerate triangle.
/// \returns 1 when a usable normal was produced, or 0 for a degenerate triangle.
/// \note Original decompilation identifier: \c func_8003A854 (0x8003A854).
s32 triangleGetUnitNormal(f32 *a, f32 *b, f32 *c, f32 *n) {
    f32 az;
    f32 bz;
    f32 cz;
    f32 ax;
    f32 bx;
    f32 cx;

    az = a[2] - a[1];
    bz = b[2] - b[1];
    cz = c[2] - c[1];
    ax = a[0] - a[1];
    bx = b[0] - b[1];
    cx = c[0] - c[1];
    n[0] = bz * cx - cz * bx;
    n[1] = cz * ax - az * cx;
    n[2] = az * bx - bz * ax;
    return vector3Normalize(n) > 0.0f;
}

/// Compute the dot product of two three-dimensional vectors.
/// \param a First three-component vector.
/// \param b Second three-component vector.
/// \returns Dot product.
/// \note Original decompilation identifier: \c func_8003A928 (0x8003A928).
f32 vector3Dot(f32 *a, f32 *b) {
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

/// Find the X coordinate of a line segment at a requested Y coordinate.
/// \param x0 X coordinate of the first endpoint; must fit a signed 32-bit integer for a horizontal match.
/// \param y0 Y coordinate of the first endpoint.
/// \param x1 X coordinate of the second endpoint.
/// \param y1 Y coordinate of the second endpoint.
/// \param y Requested Y coordinate.
/// \returns Interpolated X, or approximately 2147483600 when Y is outside the segment range. A horizontal match returns x0 truncated to an integer.
/// \note Original decompilation identifier: \c func_8003A958 (0x8003A958).
f32 mathLineXAtY(f32 x0, f32 y0, f32 x1, f32 y1, f32 y) {
    f32 var_f0;
    f32 var_f16;

    if ((y1 == y0) && (y1 == y)) {
        return (f32)(s32)x0;
    }
    var_f0 = y0;
    var_f16 = y1;
    if (y1 < y0) {
        var_f0 = y1;
        var_f16 = y0;
    }
    if ((y < var_f0) || (var_f16 < y)) {
        return 2.1474836e9f;
    }
    return (((x1 - x0) * (y - y0)) / (y1 - y0)) + x0;
}

f32 mathLineXAtY(f32, f32, f32, f32, f32); /* extern */

/// Map a value between ranges while clamping to the input endpoints.
/// \param inputStart Lower input endpoint.
/// \param outputStart Output at the lower endpoint.
/// \param inputEnd Upper input endpoint, greater than inputStart.
/// \param outputEnd Output at the upper endpoint.
/// \param value Input value to map.
/// \returns Mapped value, clamped to the corresponding endpoint outside the input range.
/// \note Original decompilation identifier: \c func_8003AA14 (0x8003AA14).
f32 mathMapRangeClamped(f32 inputStart, f32 outputStart, f32 inputEnd, f32 outputEnd, f32 value) {
    if (value <= inputStart) {
        return outputStart;
    }
    if (inputEnd <= value) {
        return outputEnd;
    }
    return mathLineXAtY(outputStart, inputStart, outputEnd, inputEnd, value);
}

/* Four-word RNG state, seeded from the RDP counters and CPU count. */
typedef struct CounterState3AA88 {
    u32 word0;
    u32 word1;
    u32 word2;
    u32 word3;
} CounterState3AA88;
typedef union CounterViews3AA88 {
    u32 words[4];
    CounterState3AA88 named;
} CounterViews3AA88;
/// Four-word state of the shared pseudorandom generator.
/// \note Original identifier: \c D_800F2970 (0x800F2970).
extern CounterViews3AA88 gRandomGeneratorState;

/// Seed the global pseudorandom generator from current system state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003AA88 (0x8003AA88).
void randomSeedFromSystem(void) {
    u32 word2;
    u32 word1;
    osDpGetCounters(gRandomGeneratorState.words);
    word2 = gRandomGeneratorState.named.word2;
    gRandomGeneratorState.named.word3 = (gRandomGeneratorState.named.word3 & 0xFFFFFF) | ((word2 & 0xFF) << 24);
    word1 = gRandomGeneratorState.named.word1;
    gRandomGeneratorState.named.word2 = (word2 = (word2 & 0xFFFF00) >> 8) | ((word1 & 0xFFFF) << 16);
    gRandomGeneratorState.named.word1 =
        (word1 = (word1 & 0xFF0000) >> 16) | ((gRandomGeneratorState.named.word0 & 0xFFFFFF) << 8);
    gRandomGeneratorState.named.word0 = osGetCount();
}

u32 randomNext(); /* extern */

/// Get a bounded value from the global pseudorandom generator.
/// \param limit Exclusive upper bound; nonpositive values produce zero.
/// \returns Value in [0, limit) for a positive limit, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003AB20 (0x8003AB20).
u32 randomBelow(s32 limit) {
    if (limit > 0) {
        return randomNext() % (u32)limit;
    }
    return 0U;
}

/// Advance the global pseudorandom generator.
/// \returns Next unsigned 32-bit pseudorandom value.
/// \note Original decompilation identifier: \c func_8003AB68 (0x8003AB68).
u32 randomNext(void) {
    u32 combo = (gRandomGeneratorState.words[0] << 1) | (gRandomGeneratorState.words[1] >> 31);
    u32 result;
    gRandomGeneratorState.words[0] = gRandomGeneratorState.words[1];
    gRandomGeneratorState.words[1] = gRandomGeneratorState.words[2];
    gRandomGeneratorState.words[2] = gRandomGeneratorState.words[3];
    result = gRandomGeneratorState.words[1] ^ combo;
    gRandomGeneratorState.words[3] = result;
    return result;
}

/// Convert an HSV color to an RGB color.
/// \param hsv Finite hue in degrees, followed by saturation and value in [0, 100] percent.
/// \param rgb Output red, green, and blue components in the range 0 through 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003ABA4 (0x8003ABA4).
void colorHsvToRgb(f32 *hsv, u8 *rgb) {
    f32 h = *hsv++;
    f32 s = *hsv++;
    f32 v = *hsv;
    f32 r;
    f32 g;
    f32 b;
    f32 chroma;
    f32 ramp;
    s32 degrees;
    s32 sectorSize = 60;
    if (s == 0.0f) {
        b = v;
        g = v;
        r = v;
    } else {
        chroma = v * s / 100.0f;
        ramp = chroma;
        while (h < 0.0f)
            h += 360.0f;
        while (h >= 360.0f)
            h -= 360.0f;
        degrees = h;
        switch ((u32)degrees / sectorSize) {
        case 0:
            r = v;
            g = (v - chroma) + (degrees % sectorSize) * ramp / 60.0f;
            b = v - chroma;
            break;
        case 1:
            r = v - (degrees % sectorSize) * ramp / 60.0f;
            g = v;
            b = v - chroma;
            break;
        case 2:
            g = v;
            r = v - chroma;
            b = (v - chroma) + (degrees % sectorSize) * ramp / 60.0f;
            break;
        case 3:
            b = v;
            r = v - chroma;
            g = v - (degrees % sectorSize) * ramp / 60.0f;
            break;
        case 4:
            r = (v - chroma) + (degrees % sectorSize) * ramp / 60.0f;
            g = v - chroma;
            b = v;
            break;
        case 5:
            r = v;
            g = v - chroma;
            b = v - (degrees % sectorSize) * ramp / 60.0f;
            break;
        }
    }
    *rgb++ = (u32)(r * 2.55f + 0.005f);
    *rgb++ = (u32)(g * 2.55f + 0.005f);
    *rgb = (u32)(b * 2.55f + 0.005f);
}
/// Convert an RGB color to an HSV color.
/// \param rgb Input red, green, and blue components in the range 0 through 255.
/// \param hsv Output hue in degrees, followed by saturation and value in percent.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003B094 (0x8003B094).
void colorRgbToHsv(u8 *rgb, f32 *hsv) {
    f32 r = (u32)*rgb++ / 2.55f;
    f32 g = (u32)*rgb++ / 2.55f;
    f32 b = (u32)*rgb / 2.55f;
    f32 max;
    f32 min;
    if (r == g && r == b) {
        hsv += 2;
        *hsv = r;
        hsv[-2] = 0.0f;
        hsv[-1] = 0.0f;
        return;
    }
    max = r > (g > b ? g : b) ? r : (g > b ? g : b);
    min = r < (g < b ? g : b) ? r : (g < b ? g : b);
    if (r == max)
        hsv[0] = (g - b) * 60.0f / (max - min);
    else if (g == max)
        hsv[0] = (b - r) * 60.0f / (max - min) + 120.0f;
    else if (b == max)
        hsv[0] = (r - g) * 60.0f / (max - min) + 240.0f;
    while (hsv[0] < 0.0f)
        hsv[0] += 360.0f;
    while (hsv[0] >= 360.0f)
        hsv[0] -= 360.0f;
    hsv[1] = (max - min) * 100.0f / max;
    hsv += 2;
    *hsv = max;
}

/// Inverse-sine angle samples used to recover unit-vector directions.
/// \note Original identifier: \c D_8009C400 (0x8009C400).
extern s16 gMathAsinLookupTable[];

/// Approximate the direction angle of a planar unit vector.
/// \param x Unit vector X component.
/// \param y Unit vector Y component.
/// \returns Direction in degrees from the positive X axis, between 0 and 360.
/// \note Original decompilation identifier: \c func_8003B318 (0x8003B318).
f32 mathUnitDirectionDegrees(f32 x, f32 y) {
    f32 ay = fabsf(y);
    f32 a;
    f32 ax;

    if (ay < 0.70711f) {
        a = gMathAsinLookupTable[(s32)(ay * 1448.1481f)] * 0.005493164f;
        if (y >= 0.0f) {
            if (x >= 0.0f) {
                return a;
            }
            return 180.0f - a;
        }
        if (x >= 0.0f) {
            return 360.0f - a;
        }
        return 180.0f + a;
    }
    ax = fabsf(x);
    if (ax > 0.70711f) {
        ax = 0.70711f;
    }
    a = gMathAsinLookupTable[(s32)(ax * 1448.1481f)] * 0.005493164f;
    if (y >= 0.0f) {
        if (x >= 0.0f) {
            return 90.0f - a;
        }
        return 90.0f + a;
    }
    if (x >= 0.0f) {
        return 270.0f + a;
    }
    return 270.0f - a;
}

/// Evaluate a clamped inverse sine without exposing the result.
/// \param value Input ratio, clamped to [-1, 1].
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003B49C (0x8003B49C).
void mathQueryAsinDegreesNoResult(f32 value) {
    if (value < -1.0f) {
        value = -1.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    mathUnitDirectionDegrees(sqrtf(1.0f - (value * value)), value);
}

/// Evaluate a clamped inverse cosine without exposing the result.
/// \param value Input ratio, clamped to [-1, 1].
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003B50C (0x8003B50C).
void mathQueryAcosDegreesNoResult(f32 value) {
    if (value < -1.0f) {
        value = -1.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    mathUnitDirectionDegrees(value, sqrtf(1.0f - (value * value)));
}

/// Determine the sign of the cosine of a degree angle.
/// \param angle Finite angle in degrees.
/// \returns -1 for negative cosine, 0 at a quarter or three-quarter turn, or 1 otherwise.
/// \note Original decompilation identifier: \c func_8003B578 (0x8003B578).
s32 mathCosSignDegrees(f32 angle) {
    if (angle < 0.0f) {
        do {
            angle += 360.0f;
        } while (angle < 0.0f);
    }
    if (angle >= 360.0f) {
        do {
            angle -= 360.0f;
        } while (angle >= 360.0f);
    }
    if ((angle > 90.0f) && (angle < 270.0f)) {
        return -1;
    }
    if ((angle == 90.0f) || (angle == 270.0f)) {
        return 0;
    }
    return 1;
}

/// Measure the shortest separation between two degree angles.
/// \param from First finite angle in degrees.
/// \param to Second finite angle in degrees.
/// \returns Angular distance from 0 through 180 degrees.
/// \note Original decompilation identifier: \c func_8003B648 (0x8003B648).
f32 mathAngleDistanceDegrees(f32 from, f32 to) {
    f32 d = to - from;
    f32 r;

    if (to < from) {
        do {
            d += 360.0f;
        } while (d < 0.0f);
    }
    if (d >= 360.0f) {
        do {
            d -= 360.0f;
        } while (d >= 360.0f);
    }
    if (d <= 180.0f) {
        r = d;
    } else {
        r = 360.0f - d;
    }
    return r;
}

/// Normalize an angle to a signed single-turn range.
/// \param angle Finite angle in degrees.
/// \returns Equivalent angle in (-180, 180].
/// \note Original decompilation identifier: \c func_8003B6D4 (0x8003B6D4).
f32 mathWrapSignedDegrees(f32 angle) {
    f32 r;

    if (angle < 0.0f) {
        do {
            angle += 360.0f;
        } while (angle < 0.0f);
    }
    if (angle >= 360.0f) {
        do {
            angle -= 360.0f;
        } while (angle >= 360.0f);
    }
    if (angle <= 180.0f) {
        r = angle;
    } else {
        r = angle - 360.0f;
    }
    return r;
}

void matrix33GetEulerDegrees(f32 *, f32 *, f32 *, f32 *);

/// Extract an equivalent Euler orientation with roll near zero.
/// \param matrix Input rotation matrix.
/// \param roll Receives roll in degrees, folded near zero and represented as a positive angle.
/// \param pitch Receives the corresponding pitch in degrees.
/// \param yaw Receives the corresponding yaw in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003B764 (0x8003B764).
void matrix33GetFoldedEulerDegrees(f32 *matrix, f32 *roll, f32 *pitch, f32 *yaw) {
    matrix33GetEulerDegrees(matrix, roll, pitch, yaw);
    if (*roll < 0.0f) {
        do {
            *roll += 360.0f;
        } while (*roll < 0.0f);
    }
    if (*roll >= 360.0f) {
        do {
            *roll -= 360.0f;
        } while (*roll >= 360.0f);
    }
    if ((*roll > 90.0f) && (*roll <= 270.0f)) {
        *roll += 180.0f;
        *pitch = 180.0f - *pitch;
        *yaw += 180.0f;
        if (*roll < 0.0f) {
            do {
                *roll += 360.0f;
            } while (*roll < 0.0f);
        }
        if (*roll >= 360.0f) {
            do {
                *roll -= 360.0f;
            } while (*roll >= 360.0f);
        }
        if (*pitch < 0.0f) {
            do {
                *pitch += 360.0f;
            } while (*pitch < 0.0f);
        }
        if (*pitch >= 360.0f) {
            do {
                *pitch -= 360.0f;
            } while (*pitch >= 360.0f);
        }
        if (*yaw < 0.0f) {
            do {
                *yaw += 360.0f;
            } while (*yaw < 0.0f);
        }
        if (*yaw >= 360.0f) {
            do {
                *yaw -= 360.0f;
            } while (*yaw >= 360.0f);
        }
    }
}

/// Extract an Euler orientation from a rotation matrix.
/// \param m Input rotation matrix.
/// \param roll Receives rotation about X in degrees.
/// \param pitch Receives rotation about Y in degrees.
/// \param yaw Receives rotation about Z in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003B978 (0x8003B978).
void matrix33GetEulerDegrees(f32 *m, f32 *roll, f32 *pitch, f32 *yaw) {
    f32 x = m[0];
    f32 y = m[1];
    f32 z = -m[2];
    f32 horizontal = sqrtf(x * x + y * y);
    f32 cosine;
    f32 sine;
    s32 verticalSign;
    if (horizontal < 0.001f && horizontal > -0.001f) {
        if (z <= 0.0f)
            *pitch = 270.0f;
        else
            *pitch = 90.0f;
        *roll = 0.0f;
        x = m[4];
        y = m[3];
        *yaw = mathUnitDirectionDegrees(x, -y);
        return;
    }
    x /= horizontal;
    y /= horizontal;
    if (z < 0.001f && z > -0.001f)
        cosine = m[8];
    else
        cosine = (-(z * y) * m[3] + m[4] * (z * x)) / z;
    if (cosine < -1.0f)
        cosine = -1.0f;
    else if (cosine > 1.0f)
        cosine = 1.0f;
    sine = sqrtf(1.0f - cosine * cosine);
    verticalSign = m[5] > 0.0f ? 1 : m[5] < 0.0f ? -1 : 0;
    if ((horizontal > 0.0f ? 1 : horizontal < 0.0f ? -1 : 0) != verticalSign)
        sine = -sine;
    *roll = mathUnitDirectionDegrees(cosine, sine);
    *pitch = mathUnitDirectionDegrees(horizontal, z);
    *yaw = mathUnitDirectionDegrees(x, y);
}

/// Get the game-convention orientation angles of a direction relative to the Z axis.
/// \param vector Input three-component direction.
/// \param yaw Receives the first orientation angle in degrees; zero for a near-zero vector.
/// \param pitch Receives the second orientation angle in degrees; zero for a near-zero vector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003BBA8 (0x8003BBA8).
void vector3GetZAxisAnglesDegrees(f32 *vector, f32 *yaw, f32 *pitch) {
    f32 x = vector[0];
    f32 y = vector[1];
    f32 z = vector[2];
    f32 length = x * x + y * y + z * z;
    f32 horizontal;
    f32 inverse;
    f32 nx;
    f32 ny;
    f32 negativeZ;
    if (length <= 0.001f) {
        *pitch = 0.0f;
        *yaw = 0.0f;
        return;
    }
    inverse = 1.0f / sqrtf(length);
    x *= inverse;
    y *= inverse;
    z *= inverse;
    negativeZ = -z;
    if (negativeZ >= 1.0f || negativeZ <= -1.0f) {
        if (negativeZ > 0.0f)
            *yaw = 90.0f;
        else
            *yaw = 270.0f;
        *pitch = 0.0f;
        return;
    }
    horizontal = sqrtf(1.0f - negativeZ * negativeZ);
    nx = x / horizontal;
    ny = y / horizontal;
    if (nx < 0.0f) {
        horizontal = -horizontal;
        nx = -nx;
        ny = -ny;
    }
    *yaw = mathUnitDirectionDegrees(horizontal, negativeZ);
    *pitch = mathUnitDirectionDegrees(nx, ny);
}

/// Get the game-convention Euler orientation of a direction.
/// \param vector Input three-component direction.
/// \param roll Receives roll in degrees; zero for a near-zero vector.
/// \param pitch Receives pitch in degrees; zero for a near-zero vector.
/// \param yaw Receives yaw in degrees; zero for a near-zero vector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003BD28 (0x8003BD28).
void vector3GetEulerDegrees(f32 *vector, f32 *roll, f32 *pitch, f32 *yaw) {
    f32 x = vector[0];
    f32 y = vector[1];
    f32 z = vector[2];
    f32 length = ((x * x) + (y * y)) + (z * z);
    f32 inverse;
    f32 axis;
    f32 horizontal;
    f32 negativeZ;
    f32 nx;
    f32 ny;
    f32 product;
    f32 sine;
    if (length <= 0.001f) {
        *yaw = 0.0f;
        *pitch = 0.0f;
        *roll = 0.0f;
        return;
    }
    inverse = 1.0f / sqrtf(length);
    x *= inverse;
    y *= inverse;
    z *= inverse;
    negativeZ = -z;
    axis = negativeZ;
    if ((negativeZ >= 0.999f) || (negativeZ <= (-0.999f))) {
        if (negativeZ > 0.0f) {
            *pitch = 90.0f;
        } else {
            *pitch = 270.0f;
        }
        *yaw = 0.0f;
        *roll = 0.0f;
        return;
    }
    if (negativeZ < (-1.f)) {
        axis = -1.0f;
    } else if (negativeZ > 1.0f) {
        axis = 1.0f;
    }
    horizontal = sqrtf(1.0f - (axis * axis));
    nx = x / horizontal;
    ny = y / horizontal;
    if (nx < 0.0f) {
        horizontal = -horizontal;
        nx = -nx;
        ny = -ny;
    }
    sine = sqrtf(1.0f - y * y);
    if ((sine < 0.001f) && (sine > (-0.001f))) {
        *roll = 0.0f;
    } else {
        product = axis * ny;
        sine = product / sine;
        if ((0.001f > product) && (product > (-0.001f))) {
            *roll = 0.0f;
        } else {
            *roll = mathUnitDirectionDegrees((sine * nx) / product, sine);
        }
    }
    *pitch = mathUnitDirectionDegrees(horizontal, axis);
    *yaw = mathUnitDirectionDegrees(nx, ny);
}

/// Get elevation and heading for a direction with Y as the vertical axis.
/// \param vector Input three-component direction.
/// \param pitch Receives elevation in degrees; zero for a near-zero vector.
/// \param yaw Receives heading from negative Z toward negative X in degrees; zero for a near-zero vector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003BFA0 (0x8003BFA0).
void vector3GetLookAnglesDegrees(f32 *vector, f32 *pitch, f32 *yaw) {
    f32 xSquared;
    f32 ySquared;
    f32 zSquared;
    f32 length;
    f32 horizontalSquared;
    f32 horizontal;
    f32 cosYaw;
    f32 sinYaw;
    f32 lengthSquared;
    f32 y;
    f32 x;
    f32 z;
    f32 inverseLength;
    f32 yawAngle;

    x = vector[0];
    y = vector[1];
    z = vector[2];
    xSquared = x * x;
    ySquared = y * y;
    zSquared = z * z;
    lengthSquared = xSquared + ySquared + zSquared;
    if (lengthSquared <= 0.001f) {
        *yaw = 0.0f;
        *pitch = 0.0f;
        return;
    }
    length = sqrtf(lengthSquared);
    inverseLength = 1.0f / length;
    x *= inverseLength;
    y *= inverseLength;
    z *= inverseLength;
    if ((y >= 1.0f) || (y <= -1.0f)) {
        if (y > 0.0f) {
            *pitch = 90.0f;
        } else {
            *pitch = 270.0f;
        }
        *yaw = 0.0f;
        return;
    } else {
        horizontalSquared = 1.0f - (y * y);
        horizontal = sqrtf(horizontalSquared);
        sinYaw = -x / horizontal;
        cosYaw = -z / horizontal;
        *pitch = mathUnitDirectionDegrees(horizontal, y);
        yawAngle = mathUnitDirectionDegrees(cosYaw, sinYaw);
    }
    *yaw = yawAngle;
}

/// Order two angle endpoints so their forward arc is at most half a turn.
/// \param start First finite angle in degrees; may be exchanged with end.
/// \param end Second finite angle in degrees; may be exchanged with start.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003C0E0 (0x8003C0E0).
void mathOrderShortestArcDegrees(f32 *start, f32 *end) {
    f32 d = *end - *start;
    f32 t;

    if (d < 0.0f) {
        do {
            d += 360.0f;
        } while (d < 0.0f);
    }
    if (d >= 360.0f) {
        do {
            d -= 360.0f;
        } while (d >= 360.0f);
    }
    if (!(d <= 180.0f)) {
        t = *start;
        *start = *end;
        *end = t;
    }
}

/// Interpolate between two angles along the shortest arc.
/// \param from Starting finite angle in degrees.
/// \param to Target finite angle in degrees.
/// \param t Interpolation fraction; 0 retains from and 1 reaches to.
/// \returns Interpolated angle in [0, 360) degrees.
/// \note Original decompilation identifier: \c func_8003C174 (0x8003C174).
f32 mathLerpAngleDegrees(f32 from, f32 to, f32 t) {
    f32 a;

    a = to - from;
    while (a < 0.0f) {
        a += 360.0f;
    }
    while (a >= 360.0f) {
        a -= 360.0f;
    }
    if (a <= 180.0f) {
        a = a * t + from;
    } else {
        a = from - (360.0f - a) * t;
    }
    while (a < 0.0f) {
        a += 360.0f;
    }
    while (a >= 360.0f) {
        a -= 360.0f;
    }
    return a;
}

/// Turn toward a target using a distance-dependent step.
/// \param from Starting finite angle in degrees.
/// \param to Target finite angle in degrees.
/// \param lo Nonnegative snap distance and minimum turn step in degrees.
/// \param stepDistance Angular separation at which the maximum turn step is reached; greater than lo.
/// \param hi Nonnegative maximum turn step in degrees.
/// \returns Updated angle in degrees; may lie outside a single-turn range.
/// \note Original decompilation identifier: \c func_8003C25C (0x8003C25C).
f32 mathApproachAngleDegrees(f32 from, f32 to, f32 lo, f32 stepDistance, f32 hi) {
    f32 d;
    f32 nlo;

    while (from < 0.0f) {
        from += 360.0f;
    }
    while (from >= 360.0f) {
        from -= 360.0f;
    }
    while (to < 0.0f) {
        to += 360.0f;
    }
    while (to >= 360.0f) {
        to -= 360.0f;
    }
    d = to - from;
    while (d < 0.0f) {
        d += 360.0f;
    }
    while (d >= 360.0f) {
        d -= 360.0f;
    }
    if (d > 180.0f) {
        d = 360.0f - d;
        hi = -hi;
        nlo = -lo;
    } else {
        nlo = lo;
    }
    if (d < lo) {
        return to;
    }
    d = mathMapRangeClamped(lo, nlo, stepDistance, hi, d);
    return from + d;
}

/// Sine samples from zero through a quarter turn used by degree-angle lookup.
/// \note Original identifier: \c D_8009CC04 (0x8009CC04).
extern f32 gMathQuarterSineLookupTable[];

/// Approximate the sine of a degree angle.
/// \param deg Angle in degrees whose rounded value in 2048 units per turn fits a signed 32-bit integer.
/// \returns Approximate sine of the angle.
/// \note Original decompilation identifier: \c func_8003C3DC (0x8003C3DC).
f32 mathSinDegrees(f32 deg) {
    f32 t = deg * 5.688889f;
    f32 r;
    s32 i;

    if (t >= 0) {
        r = t + 0.5f;
    } else {
        r = t - 0.5f;
    }
    i = (s32)r & 0x7FF;
    if (i < 0x400) {
        if (i < 0x200) {
            return gMathQuarterSineLookupTable[i & 0x1FF];
        }
        return gMathQuarterSineLookupTable[0x200 - (i & 0x1FF)];
    }
    if (i < 0x600) {
        return -gMathQuarterSineLookupTable[i & 0x1FF];
    }
    if (i < 0x800) {
        return -gMathQuarterSineLookupTable[0x200 - (i & 0x1FF)];
    }
    return 0.0f;
}

/// Approximate the cosine of a degree angle.
/// \param angle Angle in degrees whose value plus 90 satisfies mathSinDegrees input limits.
/// \returns Approximate cosine of the angle.
/// \note Original decompilation identifier: \c func_8003C4D0 (0x8003C4D0).
f32 mathCosDegrees(f32 angle) {
    return mathSinDegrees(angle + 90.0f);
}
