#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Hierarchy template used to create selectable landmarks.
/// \note Original identifier: \c D_80088E20 (0x80088E20).
extern u8 gLandmarkObjectTemplate[];
u8 *landmarkCreate(f32, f32, f32, s32);

/// Create and activate a landmark at a world position.
/// \param x World-space horizontal coordinate.
/// \param y World-space height.
/// \param z World-space depth coordinate.
/// \param landmarkId Identifier assigned to the landmark.
/// \returns Newly created landmark object.
/// \note Original decompilation identifier: \c func_8001F110 (0x8001F110).
u8 *landmarkCreate(f32 x, f32 y, f32 z, s32 landmarkId) {
    u8 *obj;
    f32 s;

    obj = objectCreateHierarchyFromTemplate((ObjectTemplate *)gLandmarkObjectTemplate);
    s = 0.1f;
    *(f32 *)(obj + 8) = s;
    *(f32 *)(obj + 4) = s;
    *(f32 *)(obj + 0) = s;
    *(f32 *)(obj + 0x18) = x;
    *(f32 *)(obj + 0x44) = x;
    *(f32 *)(obj + 0x1C) = y;
    *(f32 *)(obj + 0x48) = y;
    *(f32 *)(obj + 0x20) = z;
    *(f32 *)(obj + 0x4C) = z;
    *(s16 *)(obj + 0x34) = landmarkId;
    objectRegisterActiveHierarchy(obj);
    TRACE("landMarkNew(%f,%f,%f)\n", x, y, z);
    return obj;
}

/// Shared text storage for the pirate cannon landmark description.
/// \note Original identifier: \c D_800B6C60 (0x800B6C60).
extern s8 gLandmarkCannonDescriptionBuffer;

/// Get the formatted description of the pirate cannon landmark.
/// \param unused Unused callback argument.
/// \returns Description text in shared storage, overwritten by later calls.
/// \note Original decompilation identifier: \c func_8001F194 (0x8001F194).
s8 *landmarkGetCannonDescription(s32 unused) {
    sprintf(&gLandmarkCannonDescriptionBuffer, "^4Cannon^^\nA weapon once used by pirates.");
    return &gLandmarkCannonDescriptionBuffer;
}

s32 interactionConsiderObject(u8 *);

/// Consider a landmark as a candidate for the current object selection.
/// \param object Landmark to evaluate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F1C8 (0x8001F1C8).
void landmarkConsiderForSelection(u8 *object) {
    interactionConsiderObject(object);
}
