#include "interaction.h"
#include "landmark.h"
#include "object_types.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "common.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Hierarchy template used to create selectable landmarks.
/// \note Original identifier: \c D_80088E20 (0x80088E20).
extern u8 gLandmarkObjectTemplate[];

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
    s = 0.1F;
    ((RuntimeObjectPrefix *)obj)->scale[2] = s;
    ((RuntimeObjectPrefix *)obj)->scale[1] = s;
    ((RuntimeObjectPrefix *)obj)->scale[0] = s;
    ((RuntimeObjectPrefix *)obj)->position[0] = x;
    ((RuntimeObjectPrefix *)obj)->worldPosition[0] = x;
    ((RuntimeObjectPrefix *)obj)->position[1] = y;
    ((RuntimeObjectPrefix *)obj)->worldPosition[1] = y;
    ((RuntimeObjectPrefix *)obj)->position[2] = z;
    ((RuntimeObjectPrefix *)obj)->worldPosition[2] = z;
    ((RuntimeObjectPrefix *)obj)->kind = landmarkId;
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

/// Consider a landmark as a candidate for the current object selection.
/// \param object Landmark to evaluate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F1C8 (0x8001F1C8).
void landmarkConsiderForSelection(u8 *object) {
    interactionConsiderObject(object);
}
