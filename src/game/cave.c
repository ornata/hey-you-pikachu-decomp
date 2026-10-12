#include "object_hierarchy_create.h"
#include "object_template.h"
#include "object_lifecycle.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "heap_alloc.h"
#include "game_heap.h"
/// Description text format for the supported cave interaction kind.
/// \note Original identifier: \c D_800A4860 (0x800A4860).
extern s8 gCaveDescriptionFormat[];
/// Shared text storage returned by cave description requests.
/// \note Original identifier: \c D_800B6D30 (0x800B6D30).
extern s8 gCaveDescriptionBuffer[];

/// Hierarchy template used to create cave interaction objects.
/// \note Original identifier: \c D_80088EA0 (0x80088EA0).
extern ObjectTemplate gCaveObjectTemplate;

/// Create and register a cave interaction object with its scene anchors.
/// \param position Three-component world position of the cave interaction point.
/// \param anchorA First three-component scene anchor associated with the cave.
/// \param anchorB Second three-component scene anchor associated with the cave.
/// \param kind Cave description kind; zero selects the supported cave description.
/// \returns The registered cave object.
/// \note Original decompilation identifier: \c func_8001F4F0 (0x8001F4F0).
u8 *caveCreate(f32 *position, f32 *anchorA, f32 *anchorB, u8 kind) {
    ObjectTemplate tmpl = gCaveObjectTemplate;
    u8 *obj;
    u8 *v;

    obj = objectCreateHierarchyFromTemplate((ObjectTemplate *)&tmpl);
    *(f32 *)(obj + 0x18) = position[0];
    *(f32 *)(obj + 0x1C) = position[1];
    *(f32 *)(obj + 0x20) = position[2];
    *(f32 *)(obj + 0x44) = position[0];
    *(f32 *)(obj + 0x48) = position[1];
    *(f32 *)(obj + 0x4C) = position[2];
    v = heapAllocLocal(&gGameHeap, 0x20, 0x32);
    *(f32 *)(v + 0x0) = anchorA[0];
    *(f32 *)(v + 0x4) = anchorA[1];
    *(f32 *)(v + 0x8) = anchorA[2];
    *(f32 *)(v + 0xC) = anchorB[0];
    *(f32 *)(v + 0x10) = anchorB[1];
    *(f32 *)(v + 0x14) = anchorB[2];
    *(s32 *)(v + 0x18) = 0;
    v[0x1C] = kind;
    *(u8 **)(obj + 0x54) = v;
    objectRegisterActiveHierarchy(obj);
    return obj;
}

/// Associate a cave interaction object with a landmark.
/// \param object Initialized cave object.
/// \param landmark Linked landmark object address.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F608 (0x8001F608).
void caveSetLinkedLandmark(u8 *object, s32 landmark) {
    u8 *p = *(u8 **)(object + 0x54);

    *(s32 *)(p + 0x18) = landmark;
}

/// Get the cave description in shared text storage.
/// \param object Cave object whose description kind is queried.
/// \returns Shared description text, refreshed for kind zero and otherwise left unchanged; later requests may overwrite it.
/// \note Original decompilation identifier: \c func_8001F614 (0x8001F614).
s8 *caveGetDescription(u8 *object) {
    u8 *caveState = *(u8 **)(object + 0x54);
    if (*(u8 *)(caveState + 0x1C) == 0) {
        sprintf(gCaveDescriptionBuffer, gCaveDescriptionFormat);
    }
    return gCaveDescriptionBuffer;
}

s32 interactionConsiderObject(u8 *);

/// Consider a cave object for the current interaction selection.
/// \param object Cave interaction object to evaluate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F654 (0x8001F654).
void caveUpdateInteraction(u8 *object) {
    interactionConsiderObject(object);
}
