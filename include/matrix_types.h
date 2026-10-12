/// \file
/// Defines the affine transform layout shared by hierarchy and actor math.

#ifndef MATRIX_TYPES_H
#define MATRIX_TYPES_H

#include <ultra64.h>

/// Affine transform containing a linear basis followed by world translation.
typedef struct Matrix34 {
    f32 m[4][3]; ///< First three rows hold the linear basis; the last holds translation.
} Matrix34;

typedef char Matrix34SizeCheck[(sizeof(Matrix34) == 0x30) ? 1 : -1];
typedef char Matrix34TranslationOffsetCheck[(((u32) & (((Matrix34 *)0)->m[3])) == 0x24) ? 1 : -1];

#endif
