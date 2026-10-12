/// \file
/// Declares actor resource-cache lifetime, queued reads and borrowed storage lookup.

#ifndef ACTOR_ASSETS_H
#define ACTOR_ASSETS_H

#include "ultra64.h"

/// Initialize the actor asset cache and its read queue.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_8001DFD4 (0x8001DFD4).
void actorAssetCacheInitialize(void);

/// Release the actor asset read queue.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001E028 (0x8001E028).
void actorAssetReadQueueDestroy(void);

/// Reserve storage and request a read for an actor kind's asset if it is not already cached.
/// Complete any outstanding compressed-asset load before requesting another uncached asset.
/// \param kind Valid actor kind whose asset is requested.
/// \returns Nothing; compressed assets still require decompression before use.
/// \note Original decompilation identifier: \c func_8001E04C (0x8001E04C).
void actorAssetQueueLoadForKind(s32 kind);

/// Wait for pending actor asset reads to complete.
/// \returns Nothing; completion does not decompress compressed assets.
/// \note Original decompilation identifier: \c func_8001E16C (0x8001E16C).
void actorAssetWaitForQueuedReads(void);

/// Get the cached asset storage reserved for an actor kind.
/// \param kind Valid actor kind whose asset storage is requested.
/// \returns Borrowed storage address owned by the asset cache, or zero if no load was requested; contents may not yet be ready for use.
/// \note Original decompilation identifier: \c func_8001E190 (0x8001E190).
u8 *actorAssetGetCachedStorage(s32 kind);

#endif
