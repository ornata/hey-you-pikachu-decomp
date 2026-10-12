#include "heap_free.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_lookup.h"
#include "keyed_pool_types.h"
#include "scene_types.h"
#include "heap_alloc.h"
#include "object_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Handles of registered door and sign objects.
/// \note Original identifier: \c D_800B6990 (0x800B6990).
s32 gDoorObjectHandles[10];
/// Number of registered door and sign objects.
/// \note Original identifier: \c D_800B69B8 (0x800B69B8).
s32 gDoorObjectCount;
/// Shared destination-description text overwritten by the next door description request.
/// \note Original identifier: \c D_800B69C0 (0x800B69C0).
s8 gDoorDescriptionBuffer[0xC8];
/// Shared passage-access text overwritten by the next access-description request.
/// \note Original identifier: \c D_800B6A88 (0x800B6A88).
char gDoorAccessDescriptionBuffer[0xC8];
/// Owned storage backing the object-response heap.
/// \note Original identifier: \c D_800B6B50 (0x800B6B50).
u8 *gObjectResponseHeapStorage;
/// Heap header for queued response records and their owned payloads.
/// \note Original identifier: \c D_800B6B58 (0x800B6B58).
HpHdr gObjectResponseHeap;
/// Keyed pool indexing pending responses by target object handle.
/// \note Original identifier: \c D_800B6B68 (0x800B6B68).
u8 gObjectResponseRegistry[0x28];
s32 objectResponsesGetFreeCapacity(void);
/// Per-kind response callback view of the object callback table, with a twenty-four-byte stride.
/// \note Original identifier: \c D_80078154 (0x80078154).
extern u8 gObjectKindResponseEntries[];
s32 keyedPoolInsert(KeyedPool *, s32, u32, u8 *);
void objectResponsesClearWithCallback(s32, s32);
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
void heapInitializeRegion(void *, s32, s32);
void keyedPoolInitialize(KeyedPool *, u32);

/// Reset the registry of created door objects.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C110 (0x8001C110).
void doorRegistryReset(void) {
    s32 i;

    for (i = 0; i < 10; i++) {
        gDoorObjectHandles[i] = 0;
    }
    gDoorObjectCount = 0;
}

u8 *objectCreateDefault(void);
u8 *objectMakeHierarchyRoot(u8 *);
u8 *objectRegisterActiveHierarchy(u8 *);

/// Create and register a destination door or sign.
/// \param position Three-component world position.
/// \param kind Door or sign description type, from 0 through 3.
/// \param destination Destination area or exit category, according to kind.
/// \param entrance Destination entrance identifier.
/// \param accessState Initial access state used to describe whether passage is allowed.
/// \returns The created door object; the door registry must have room for another entry.
/// \note Original decompilation identifier: \c func_8001C150 (0x8001C150).
u8 *doorCreate(f32 *position, s32 kind, s32 destination, s32 entrance, s32 accessState) {
    u8 *a;
    u8 *p;
    u8 *r;

    TRACE("door object was made, (%3.2f, %3.2f, %3.2f), type = %d, area = %d, entrance = %d\n, lock = %s \n",
          position[0], position[1], position[2], kind, destination, entrance, accessState ? "TRUE" : "FALSE");
    a = objectCreateDefault();
    p = (u8 *)heapAllocLocal(&gGameHeap, 0x10, 0x40);
    _bzero((s32)p, 0x10);
    *(s32 *)(p + 4) = destination;
    *(s32 *)(p + 8) = entrance;
    *(s32 *)(p + 0) = kind;
    *(s32 *)(p + 0xC) = accessState;
    *(u16 *)(a + 0x34) = 0x87;
    *(u8 **)(a + 0x54) = p;
    r = objectRegisterActiveHierarchy(objectMakeHierarchyRoot(a));
    *(f32 *)(a + 0x18) = position[0];
    *(f32 *)(a + 0x1C) = position[1];
    *(f32 *)(a + 0x20) = position[2];
    *(f32 *)(a + 0x44) = position[0];
    *(f32 *)(a + 0x48) = position[1];
    *(f32 *)(a + 0x4C) = position[2];
    gDoorObjectHandles[gDoorObjectCount] = *(s32 *)(a + 0x38);
    gDoorObjectCount += 1;
    return r;
}

/// Change the passage-access state of a registered door.
/// \param doorIndex Valid index in the created-door registry.
/// \param accessState Passage state: 0 allowed, 1 locked, 2 unavailable,
/// 3 requiring Pikachu nearby, or 4 blocked by Squirtle.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C248 (0x8001C248).
void doorSetAccessState(s32 doorIndex, s32 accessState) {
    u8 *obj;

    obj = *(u8 **)(objectFindLiveByHandle(gDoorObjectHandles[doorIndex]) + 0x54);
    *(s32 *)(obj + 0xC) = accessState;
}

char *areaGetDisplayName(s32); /* extern */

char *gDoorExitDescriptions[4] = {
    "Sliding Door\nThis leads to\n^6PIKACHU^^'s Discovery Days\nwhere you can play.",
    "Front Gate\nThis leads to\n^6PIKACHU^^'s Play Days\nwhere you can play.",
    "Side Gate\nThis leads to\n^6PIKACHU^^'s Daring Days\nwhere you can play.",
};

/// Format a door or sign destination description.
/// \param object Door object with a supported description type and destination.
/// \returns Shared description text, overwritten by the next description request.
/// \note Original decompilation identifier: \c func_8001C284 (0x8001C284).
s8 *doorGetDescription(u8 *object) {
    s32 passageKind;
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    passageKind = ((PassageState *)objectState)->kind;
    switch (passageKind) { /* irregular */
    case 0:
        sprintf(gDoorDescriptionBuffer, "Bedroom Door\nThis leads to \n%s.",
                areaGetDisplayName(((PassageState *)objectState)->destination));
        break;
    case 1:
        sprintf(gDoorDescriptionBuffer, "Sign\nIt says,\n\"This way to %s.\"",
                areaGetDisplayName(((PassageState *)objectState)->destination));
        break;
    case 2:
        sprintf(gDoorDescriptionBuffer, gDoorExitDescriptions[((PassageState *)objectState)->destination]);
        break;
    case 3:
        sprintf(gDoorDescriptionBuffer, gDoorExitDescriptions[0]);
        break;
    }
    return gDoorDescriptionBuffer;
}

/// Format a description of the door passage-access state.
/// \param object Door object whose passage state is described.
/// \returns Shared access-description text, overwritten by the next access-description request.
/// \note Original decompilation identifier: \c func_8001C35C (0x8001C35C).
u8 *doorGetAccessDescription(u8 *object) {
    PassageState *state = ((RuntimeObjectPrefix *)object)->state.pointer;

    switch ((u32)state->accessState) {
    case 0:
        sprintf(gDoorAccessDescriptionBuffer, "\nPress @I@J to go on.");
        break;
    case 1:
        sprintf(gDoorAccessDescriptionBuffer, "\nIt's locked.");
        break;
    case 2:
        sprintf(gDoorAccessDescriptionBuffer, "\nYou can't go through here yet.");
        break;
    case 3:
        sprintf(gDoorAccessDescriptionBuffer, "\nYou can go through\nwhen ^6PIKACHU^^ is near.");
        break;
    case 4:
        sprintf(gDoorAccessDescriptionBuffer, "\n^4SQUIRTLE^^ is blocking the way.");
        break;
    default:
        sprintf(gDoorAccessDescriptionBuffer, "\nYou can't get by for some reason.");
        break;
    }
    return (u8 *)gDoorAccessDescriptionBuffer;
}

s32 interactionConsiderObject(u8 *);

/// Update a door object's interaction eligibility.
/// \param object Door object to consider for interaction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C440 (0x8001C440).
void doorUpdateInteraction(u8 *object) {
    interactionConsiderObject(object);
}

/// Initialize storage for queued object responses.
/// \param capacity Positive maximum number of registered response records.
/// \param heapBytes Number of bytes available for response records and owned payloads.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C460 (0x8001C460).
void objectResponsesInitialize(s32 capacity, s32 heapBytes) {
    heapBytes = (heapBytes + 0xF) & ~0xF;
    gObjectResponseHeapStorage = heapAllocLocal(&gGameHeap, heapBytes, 0x33);
    heapInitializeRegion(&gObjectResponseHeap, (s32)gObjectResponseHeapStorage, heapBytes);
    keyedPoolInitialize((KeyedPool *)&gObjectResponseRegistry, capacity);
}

void keyedPoolDestroy(KeyedPool *); /* extern */

/// Release the queued object-response registry and its storage.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C4CC (0x8001C4CC).
void objectResponsesDestroy(void) {
    keyedPoolDestroy((KeyedPool *)gObjectResponseRegistry);
    heapFreeLocal(&gGameHeap, gObjectResponseHeapStorage);
}

void vector3UnpackNormalBytes(u8 *, f32 *);
void itemAddVelocity(s32, f32 *);
void itemStartDirectionMotion(s32, f32, f32, f32);

/// Apply a queued item impulse and its visual reaction.
/// \param object Target item object address.
/// \param response Impulse response containing a packed direction and strength.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C504 (0x8001C504).
void objectResponseApplyImpulse(s32 object, u8 *response) {
    f32 v[3];

    vector3UnpackNormalBytes(response + 8, v);
    v[0] = (f32) * (s8 *)(response + 0xB) * v[0];
    v[1] = (f32) * (s8 *)(response + 0xB) * v[1];
    v[2] = (f32) * (s8 *)(response + 0xB) * v[2];
    itemAddVelocity(*(s32 *)(object + 0x38), v);
    itemStartDirectionMotion(object, 0.5f, 1000.0f, 0.0f);
}

/// Response callbacks indexed by response kind.
/// \note Original identifier: \c D_80078130 (0x80078130).
void (*gObjectResponseHandlers[])(s32, u8 *) = {objectResponseApplyImpulse};

/// Dispatch a queued response according to its response kind.
/// \param object Target object address.
/// \param response Response record with a supported kind.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C5B0 (0x8001C5B0).
void objectResponseDispatch(s32 object, u8 *response) {
    gObjectResponseHandlers[response[6]](object, response);
}

s32 keyedPoolIterateFirstForKey(KeyedPool *, s32);
s32 keyedPoolIterateNextForKey(KeyedPool *, s32);
void keyedPoolRemoveAllForKey(KeyedPool *, s32);

/// Consume an object's pending responses, optionally invoking a callback for each.
/// \param object Target object address whose pending responses are cleared.
/// \param callback Optional callback address receiving the target object and each response before it is released; zero skips dispatch.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C5E4 (0x8001C5E4).
void objectResponsesClearWithCallback(s32 object, s32 callback) {
    s32 key;
    u8 *n;

    key = *(s32 *)(object + 0x38);
    n = (u8 *)keyedPoolIterateFirstForKey((KeyedPool *)gObjectResponseRegistry, key);
    if (n != NULL) {
        do {
            if (callback != 0) {
                ((void (*)(s32, u8 *))callback)(object, n);
            }
            if (*(u16 *)(n + 4) & 1) {
                heapFreeLocal(&gObjectResponseHeap, *(void **)(n + 8));
            }
            heapFreeLocal(&gObjectResponseHeap, n);
        } while ((n = (u8 *)keyedPoolIterateNextForKey((KeyedPool *)gObjectResponseRegistry, key)) != NULL);
    }
    keyedPoolRemoveAllForKey((KeyedPool *)gObjectResponseRegistry, key);
    *(u16 *)(object + 0x36) = *(u16 *)(object + 0x36) & 0xF7FF;
}

/// Dispatch an object's pending responses through its type handler and release them.
/// \param object Target object address.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C6BC (0x8001C6BC).
void objectResponsesDispatchAndClear(s32 object) {
    objectResponsesClearWithCallback(object,
                                     *(s32 *)(gObjectKindResponseEntries + (*(u16 *)((u8 *)object + 0x34) * 0x18)));
}

/// Release an object's pending responses without dispatching them.
/// \param object Target object address.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C6F4 (0x8001C6F4).
void objectResponsesDiscard(s32 object) {
    objectResponsesClearWithCallback(object, 0);
}

/// Get the remaining response-registry capacity.
/// \returns The number of unused registry entries; response-heap space is a separate requirement.
/// \note Original decompilation identifier: \c func_8001C714 (0x8001C714).
s32 objectResponsesGetFreeCapacity(void) {
    return ((KeyedPool *)&gObjectResponseRegistry)->capacity - ((KeyedPool *)&gObjectResponseRegistry)->active;
}

/// Attach a prepared response to its target object.
/// \param object Target object with available response-registry capacity.
/// \param response Prepared record allocated by objectResponseCreate; ownership
/// transfers to the response system.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C72C (0x8001C72C).
void objectResponsesAttach(u8 *object, s32 response) {
    *(u16 *)(object + 0x36) |= 0x800;
    keyedPoolInsert((KeyedPool *)gObjectResponseRegistry, response, *(s32 *)(object + 0x38), object);
}

/// Allocate a response record for an eligible target object.
/// \param source Source object, or NULL for no source.
/// \param target Target object with a valid hierarchy root, used to check
/// response eligibility.
/// \param kind Response kind to assign.
/// \returns A newly allocated record whose payload the caller must prepare and attach, or NULL when ineligible or storage is unavailable.
/// \note Original decompilation identifier: \c func_8001C764 (0x8001C764).
u8 *objectResponseCreate(u8 *source, u8 *target, u8 kind) {
    u8 *allocation;
    if (*(u16 *)(*(u8 **)(target + 0x50) + 0x36) & 0x30) {
        return NULL;
    }
    if (objectResponsesGetFreeCapacity() <= 0) {
        return NULL;
    }
    allocation = heapAllocLocal(&gObjectResponseHeap, 0xC, 0xC6);
    if (allocation == NULL) {
        return NULL;
    }
    if (source != NULL) {
        *(s32 *)(allocation + 0) = *(s32 *)(source + 0x38);
    } else {
        *(s32 *)(allocation + 0) = 0;
    }
    allocation[6] = kind;
    *(s16 *)(allocation + 4) = 0;
    return allocation;
}

typedef struct {
    u8 b[4];
} Bytes4;
/// Queue a packed impulse response for an eligible target object.
/// \param source Source object, or NULL for no source.
/// \param target Target item object.
/// \param impulse Packed direction bytes and signed strength byte.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C804 (0x8001C804).
void objectResponseQueueImpulse(u8 *source, u8 *target, Bytes4 impulse) {
    u8 *r = objectResponseCreate(source, target, 0);

    if (r != NULL) {
        *(Bytes4 *)(r + 8) = impulse;
        objectResponsesAttach(target, (s32)r);
    }
}

/// Provide the inactive queued object-response diagnostic report hook.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C84C (0x8001C84C).
void objectResponsesDebugReportNoOp(void) {
}
