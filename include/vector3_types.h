/// \file
/// Defines a three-component floating-point vector shared by spatial queries.
#ifndef VECTOR3_TYPES_H
#define VECTOR3_TYPES_H
#include <ultra64.h>

/// Three spatial components in X, Y, Z order.
typedef struct Vec3f {
    f32 x; ///< X component.
    f32 y; ///< Y component.
    f32 z; ///< Z component.
} Vec3f;
typedef char Vec3fSizeCheck[(sizeof(Vec3f) == 0xC) ? 1 : -1];
#endif
