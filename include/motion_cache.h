/// \file
/// Provides cached motion archives and archive-relative address bases.

#ifndef MOTION_CACHE_H
#define MOTION_CACHE_H

#include "rom_cache.h"

/// Shared cache owning loaded motion data and pending reads.
/// \note Original identifier: \c D_800B6930 (0x800B6930).
extern RomCache gMotionRomCache;

/// Initialize the shared motion-data cache.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800171D0 (0x800171D0).
void motionCacheInitialize(void);

/// Release the shared motion-data cache.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80017204 (0x80017204).
void motionCacheDestroy(void);

/// Request a motion and obtain its archive-relative memory base.
/// \param cache Cache receiving the request.
/// \param motionId Index of a valid motion in the archive.
/// \param archiveBase Receives a memory base to which archive-relative motion offsets can be added.
/// \returns 3 when the motion is ready, or 4 when the read must be awaited.
/// \note Original decompilation identifier: \c func_80017228 (0x80017228).
s32 motionCacheRead(RomCache *cache, s32 motionId, s32 *archiveBase);

/// Load a motion and wait until its data is ready.
/// \param cache Cache receiving the request.
/// \param motionId Index of a valid motion in the archive.
/// \returns A memory base to which archive-relative motion offsets can be added.
/// \note Original decompilation identifier: \c func_80017290 (0x80017290).
s32 motionCacheReadBlocking(RomCache *cache, s32 motionId);

/// Get the display name of a motion.
/// \param motionId Motion index to look up.
/// \returns The motion name, or the no-motion label for an invalid index.
/// \note Original decompilation identifier: \c func_800172CC (0x800172CC).
u8 *motionGetName(s32 motionId);

#endif
