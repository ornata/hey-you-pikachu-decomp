/// \file
/// Defines saved reference poses for relative actor model-part orientation.

#ifndef ACTOR_ORIENTATION_CACHE_H
#define ACTOR_ORIENTATION_CACHE_H

#include "matrix_types.h"

/// Reference pose retained across relative-orientation evaluations.
/// A cache belongs to its containing actor state or owned model-part allocation.
/// Set captureReference before first use or to adopt a new reference pose.
typedef struct ActorOrientationCache {
    s32 captureReference;      ///< Nonzero requests a new reference on the next evaluation.
    Matrix34 inverseReference; ///< Inverse saved orientation, valid after reference capture.
} ActorOrientationCache;

typedef char ActorOrientationCacheSizeCheck[(sizeof(ActorOrientationCache) == 0x34) ? 1 : -1];
typedef char
    ActorOrientationCacheMatrixOffsetCheck[(((u32) & ((ActorOrientationCache *)0)->inverseReference) == 0x4) ? 1 : -1];

/// Evaluate a model-part sequence relative to a retained reference pose.
/// \param cache Writable cache initialized with captureReference nonzero, or a
/// previously captured reference; requesting capture produces identity this call.
/// \param matrix Output storage for a complete 3-by-4 relative orientation matrix.
/// \param actor Actor address in the original 32-bit ABI, containing the listed parts.
/// \param parts Address of a zero-terminated signed 32-bit part-identifier sequence;
/// negative identifiers request inverse rotations and the minimum integer is invalid.
/// \returns Nothing.
/// \note Original identifier: \c func_800142E0 (0x800142E0).
void actorEvaluateRelativePartOrientation(ActorOrientationCache *cache, u8 *matrix, s32 actor, s32 parts);

#endif
