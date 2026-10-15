#include "interaction.h"
#include "cave.h"
#include "scene_types.h"
#include "object_types.h"
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
    CaveState *v;

    obj = objectCreateHierarchyFromTemplate(&tmpl);
    ((RuntimeObjectPrefix *)obj)->position[0] = position[0];
    ((RuntimeObjectPrefix *)obj)->position[1] = position[1];
    ((RuntimeObjectPrefix *)obj)->position[2] = position[2];
    ((RuntimeObjectPrefix *)obj)->worldPosition[0] = position[0];
    ((RuntimeObjectPrefix *)obj)->worldPosition[1] = position[1];
    ((RuntimeObjectPrefix *)obj)->worldPosition[2] = position[2];
    v = heapAllocLocal(&gGameHeap, 0x20, 0x32);
    v->firstAnchor[0] = anchorA[0];
    v->firstAnchor[1] = anchorA[1];
    v->firstAnchor[2] = anchorA[2];
    v->secondAnchor[0] = anchorB[0];
    v->secondAnchor[1] = anchorB[1];
    v->secondAnchor[2] = anchorB[2];
    v->landmark = NULL;
    v->descriptionKind = kind;
    ((RuntimeObjectPrefix *)obj)->state.pointer = v;
    objectRegisterActiveHierarchy(obj);
    return obj;
}

/// Associate a cave interaction object with a landmark.
/// \param object Initialized cave object.
/// \param landmark Linked landmark object address.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F608 (0x8001F608).
void caveSetLinkedLandmark(u8 *object, u8 *landmark) {
    CaveState *p = ((RuntimeObjectPrefix *)object)->state.pointer;

    p->landmark = landmark;
}

/// Get the cave description in shared text storage.
/// \param object Cave object whose description kind is queried.
/// \returns Shared description text, refreshed for kind zero and otherwise left unchanged; later requests may overwrite it.
/// \note Original decompilation identifier: \c func_8001F614 (0x8001F614).
s8 *caveGetDescription(u8 *object) {
    CaveState *caveState = ((RuntimeObjectPrefix *)object)->state.pointer;
    if (caveState->descriptionKind == 0) {
        sprintf(gCaveDescriptionBuffer, gCaveDescriptionFormat);
    }
    return gCaveDescriptionBuffer;
}

/// Consider a cave object for the current interaction selection.
/// \param object Cave interaction object to evaluate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F654 (0x8001F654).
void caveUpdateInteraction(u8 *object) {
    interactionConsiderObject(object);
}
