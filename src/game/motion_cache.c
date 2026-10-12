/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "rom_cache.h"

/// Shared ROM cache owning loaded motion data and pending reads.
/// \note Original identifier: \c D_800B6930 (0x800B6930).
extern RomCache gMotionRomCache;
/// Motion archive offsets indexed by motion identifier, including the next-entry end offset.
/// \note Original identifier: \c D_80077778 (0x80077778).
extern s32 gMotionArchiveOffsets[];
/// Cartridge ROM start address of the motion archive.
/// \note Original identifier: \c D_B77EB0 (0xB77EB0).
extern u8 gMotionArchiveRomStart[];
s32 motionCacheRead(s32, s32, s32 *);
/// Borrowed display-name pointers indexed by motion identifier.
/// \note Original identifier: \c D_80076CB0 (0x80076CB0).
extern u8 *gMotionDisplayNames[];
/// Fallback display text for an unavailable motion identifier.
/// \note Original identifier: \c D_800A3624 (0x800A3624).
extern u8 gNoMotionDisplayName[];
u8 *motionGetName(s32);

/// Initialize the shared motion-data cache.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800171D0 (0x800171D0).
void motionCacheInitialize(void) {
    romCacheInitialize(&gMotionRomCache, 0x32000, 0x1F, 0x10);
}


/// Release the shared motion-data cache.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80017204 (0x80017204).
void motionCacheDestroy(void) {
    romCacheDestroy(&gMotionRomCache);
}

/// Request a motion and obtain its archive-relative memory base.
/// \param cache Cache receiving the request.
/// \param motionId Index of a valid motion in the archive.
/// \param archiveBase Receives a memory base to which archive-relative motion offsets can be added.
/// \returns 3 when the motion is ready, or 4 when the read must be awaited.
/// \note Original decompilation identifier: \c func_80017228 (0x80017228).
s32 motionCacheRead(s32 cache, s32 motionId, s32 *archiveBase) {
    s32 sp18;
    s32 archiveOffset;
    s32 ret;

    archiveOffset = gMotionArchiveOffsets[motionId];
    sp18 = archiveOffset;
    ret = romCacheRead(cache, archiveOffset + gMotionArchiveRomStart,
                       gMotionArchiveOffsets[motionId + 1] - archiveOffset, archiveBase);
    *archiveBase -= archiveOffset;
    return ret;
}

/// Load a motion and wait until its data is ready.
/// \param cache Cache receiving the request.
/// \param motionId Index of a valid motion in the archive.
/// \returns A memory base to which archive-relative motion offsets can be added.
/// \note Original decompilation identifier: \c func_80017290 (0x80017290).
s32 motionCacheReadBlocking(s32 cache, s32 motionId) {
    s32 sp1C;

    if (motionCacheRead(cache, motionId, &sp1C) == 4) {
        romCacheWaitForReads(cache);
    }
    return sp1C;
}

/// Get the display name of a motion.
/// \param motionId Motion index to look up.
/// \returns The motion name, or the no-motion label for an invalid index.
/// \note Original decompilation identifier: \c func_800172CC (0x800172CC).
u8 *motionGetName(s32 motionId) {
    if ((motionId < 0) || (motionId >= 0xE6)) {
        return gNoMotionDisplayName;
    }
    return gMotionDisplayNames[motionId];
}
