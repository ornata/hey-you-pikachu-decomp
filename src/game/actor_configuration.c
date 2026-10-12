#include "object_movement.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "heap_free.h"
#include "heap_tail.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "actor_create.h"
#include "actor_part_config.h"
#include "actor_text.h"
#include "actor_parts.h"
#include "object_audio.h"
#include "asset_loader.h"
#include "actor_target.h"
#include "heap_diagnostics.h"
#include "integer_animation.h"
#include "object_lookup.h"
#include "actor_script.h"
#include "heap_alloc.h"
#include "object_types.h"
#include "rom_dma.h"
extern float fabsf(float);
#pragma intrinsic(fabsf)
/// Actor-kind table viewed at default part identifiers, with eighty-four bytes per kind.
/// \note Original identifier: \c D_80088008 (0x80088008).
extern u8 gActorKindDefaultPartEntries[];
/// Actor-kind table viewed at second attachment part identifiers, with eighty-four bytes per kind.
/// \note Original identifier: \c D_80088010 (0x80088010).
extern u8 gActorKindSecondAttachmentPartEntries[];
/// Actor-kind table viewed at first attachment part identifiers, with eighty-four bytes per kind.
/// \note Original identifier: \c D_8008800C (0x8008800C).
extern u8 gActorKindFirstAttachmentPartEntries[];
/// Actor-kind table viewed at ground-reference part lists, with eighty-four bytes per kind.
/// \note Original identifier: \c D_80087FE4 (0x80087FE4).
extern u8 gActorKindGroundReferencePartEntries[];
s32 progressGetValue(u32);
s32 progressGetMaximum(s32);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
void objectOrientTowardCamera(u8 *);
/// Integer animation program used before the contact effect is triggered.
/// \note Original identifier: \c D_8006B390 (0x8006B390).
extern s32 gContactEffectIdleAnimation;
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
void textureLoadForDraw(Gfx **, s32, s32);
/// Cached actor asset storage addresses indexed by resource identifier.
/// \note Original identifier: \c D_800B6BB8 (0x800B6BB8).
extern s32 gActorAssetStorage[];
/// Actor-kind table viewed at resource indices, with eighty-four bytes per kind.
/// \note Original identifier: \c D_80088018 (0x80088018).
extern u8 gActorKindResourceIndexEntries[];
/// Actor-kind configuration base viewed at alternate display-name pointers.
/// \note Original identifier: \c D_80087FC8 (0x80087FC8).
extern u8 gActorKindAlternateNameEntries[];
/// Actor-kind table viewed at default display-name pointers, with eighty-four bytes per kind.
/// \note Original identifier: \c D_80087FD0 (0x80087FD0).
extern u8 gActorKindDefaultNameEntries[];
/// Actor-kind table viewed at alternate description pointers, with eighty-four bytes per kind.
/// \note Original identifier: \c D_80087FCC (0x80087FCC).
extern u8 gActorKindAlternateDescriptionEntries[];
/// Actor-kind table viewed at default description pointers, with eighty-four bytes per kind.
/// \note Original identifier: \c D_80087FD4 (0x80087FD4).
extern u8 gActorKindDefaultDescriptionEntries[];
/// Actor-kind configuration storage beginning at kind one; creation indexes it with kind minus one.
/// \note Original identifier: \c D_8008801C (0x8008801C).
extern u8 gActorKindConfigurationsFromOne[];
/// Shared asynchronous ROM-transfer queue for actor assets.
/// \note Original identifier: \c D_800B6B90 (0x800B6B90).
extern DmaQueue gActorAssetReadQueue;
void _bzero(void *, u32);
/// Integer animation program used after contact triggers the effect.
/// \note Original identifier: \c D_8006B394 (0x8006B394).
extern s32 gContactEffectTriggeredAnimation;
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
void actorUpdateModel(u8 *);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

f32 terrainGetWorldHeight(f32, f32, f32, s32);
/// Initial uniform scale of a contact-reactive effect.
/// \note Original identifier: \c D_800A3A68 (0x800A3A68).
extern f32 gContactEffectInitialScale;
/// Hierarchy template used to create contact-reactive effects.
/// \note Original identifier: \c D_80079260 (0x80079260).
extern u8 gContactEffectHierarchyTemplate[];

/// Create and register a contact-reactive effect at a world position.
/// \param x World-space X coordinate.
/// \param y World-space Y coordinate; negative values request ground placement using the corresponding positive search height.
/// \param z World-space Z coordinate.
/// \returns Newly initialized and registered effect.
/// \note Original decompilation identifier: \c func_8001D6C0 (0x8001D6C0).
u8 *contactEffectCreate(f32 x, f32 y, f32 z) {
    u8 *p = objectCreateHierarchyFromTemplate((ObjectTemplate *)&gContactEffectHierarchyTemplate);
    f32 s;

    if (y < 0.0f) {
        y = terrainGetWorldHeight(x, -y, z, 0) + 1.0f;
    }
    s = gContactEffectInitialScale;
    *(f32 *)(p + 0x18) = x;
    *(f32 *)(p + 0x1C) = y;
    *(f32 *)(p + 0x20) = z;
    *(f32 *)(p + 0x44) = x;
    *(f32 *)(p + 0x48) = y;
    *(f32 *)(p + 0x4C) = z;
    *(f32 *)(p + 0x8) = s;
    *(f32 *)(p + 0x4) = s;
    *(f32 *)(p + 0x0) = s;
    *(f32 *)(p + 0x24) = 3.0f;
    objectRegisterActiveHierarchy(p);
    return p;
}

/// Initialize an animated effect that reacts to contact with the active actor.
/// \param effect Effect object receiving its initial animation state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001D784 (0x8001D784).
void contactEffectInitialize(u8 *effect) {
    s32 *savedEffectState;
    s32 *effectState;
    effectState = heapAllocLocal(&gGameHeap, 0x10, 0x46);
    savedEffectState = effectState;
    *effectState = 0;
    animationIntegerTrackInitialize((ValueTrack *)((f32 *)(effectState + 1)), (ValueKey *)(gContactEffectIdleAnimation));
    *(s32 **)(effect + 0x54) = savedEffectState;
}

s32 actorGetPartObjectHandle(u8 *, s32);
u8 *linkedEffectCreateChain(s32, f32, f32, f32, f32, s32, f32, f32, f32, f32, f32, s32, s32, s32);
f32 vector3Length(f32 *);
/// Attraction strength between adjacent effects in the contact reaction chain.
/// \note Original identifier: \c D_800A3A6C (0x800A3A6C).
extern f32 gContactEffectChainPull;
/// Uniform scale of effects spawned by the contact reaction.
/// \note Original identifier: \c D_800A3A70 (0x800A3A70).
extern f32 gContactEffectChainScale;
/// Velocity retention factor of the contact reaction effect chain.
/// \note Original identifier: \c D_800A3A74 (0x800A3A74).
extern f32 gContactEffectChainDamping;

/// Advance a contact effect, react to the active actor, and retire the effect when its reaction finishes.
/// \param effect Initialized effect object to update; requires an active actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001D7D8 (0x8001D7D8).
void contactEffectUpdate(u8 *effect) {
    s32 *state = *(s32 **)(effect + 0x54);
    f32 delta[3];
    s32 frame;
    s32 result;
    f32 pull;
    if (*state == 0) {
        delta[0] = *(f32 *)((u8 *)gPikachuActor + 0x44) - *(f32 *)(effect + 0x44);
        delta[1] = *(f32 *)((u8 *)gPikachuActor + 0x48) - *(f32 *)(effect + 0x48);
        delta[2] = *(f32 *)((u8 *)gPikachuActor + 0x4C) - *(f32 *)(effect + 0x4C);
        if ((*(f32 *)(effect + 0x24) + *(f32 *)((u8 *)gPikachuActor + 0x24)) > vector3Length(delta)) {
            *state = 1;
            animationIntegerTrackInitialize((ValueTrack *)((f32 *)(state + 1)), (ValueKey *)(gContactEffectTriggeredAnimation));
        }
    }
    result = animationIntegerTrackEvaluate((ValueTrack *)(state + 1), &frame, gRuntimeFrameSeconds);
    if (*state != 0) {
        if (result == 1) {
            objectRequestHierarchyDestruction(effect);
        } else if (result == 2) {
            objectPlaySpatialSound(effect, 0x97, 20.0f, 100.0f);
            result = actorGetPartObjectHandle((u8 *)gPikachuActor, 0x79);
            pull = gContactEffectChainPull;
            linkedEffectCreateChain(2, *(f32 *)(effect + 0x44), *(f32 *)(effect + 0x48), *(f32 *)(effect + 0x4C),
                                    gContactEffectChainScale, result, 1.0f, gContactEffectChainDamping, 2.0f, pull,
                                    pull, 0xC2, 1, 8);
        }
    }
    objectOrientAlongCameraView(effect);
}

/// Prepare the contact effect's current animation texture for drawing.
/// \param effect Initialized effect object to draw.
/// \returns Borrowed effect model display-list address represented as an integer.
/// \note Original decompilation identifier: \c func_8001D96C (0x8001D96C).
s32 contactEffectPrepareTextureForDraw(u8 *effect) {
    u8 *obj;

    obj = *(u8 **)(effect + 0x54);
    textureLoadForDraw(&gRenderDisplayListCursor, 5, *(s16 *)(obj + 0xC));
    return *(s32 *)(effect + 0x30);
}

/// Trigger an idle contact effect when a response identifies the active actor.
/// \param effect Initialized effect object receiving the response.
/// \param response Response whose first value is the responding object's handle.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001D9A4 (0x8001D9A4).
void contactEffectHandleActorResponse(u8 *effect, s32 *response) {
    u8 *effectState;
    u8 *savedEffectState;

    effectState = *(u8 **)(effect + 0x54);
    if (*(s32 *)effectState == 0) {
        savedEffectState = effectState;
        if (objectFindLiveByHandle(*response) == (u8 *)gPikachuActor) {
            *(s32 *)effectState = 1;
            animationIntegerTrackInitialize((ValueTrack *)((f32 *)(effectState + 4)), (ValueKey *)(gContactEffectTriggeredAnimation));
        }
    }
}

void actorInitializeModelState(u8 *);
void actorAllocatePartTable(u8 *);

/// Initialize an actor model and its part lookup storage.
/// \param actor Actor to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DA00 (0x8001DA00).
void actorInitializeModelAndPartTable(u8 *actor) {
    actorInitializeModelState(actor);
    actorAllocatePartTable(actor);
}

void actorUpdateScriptVm(u8 *);

/// Advance an actor's script execution and model behavior.
/// \param actor Actor whose script and model are updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DA28 (0x8001DA28).
void actorUpdateScriptAndModel(u8 *actor) {
    actorUpdateScriptVm(actor);
    actorUpdateModel(actor);
}

/// Advance an actor's script and model while following its stored target position when a target is available.
/// \param actor Actor whose behavior and position are updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DA50 (0x8001DA50).
void actorUpdateScriptAndModelFollowingTarget(u8 *actor) {
    u8 *sp24;

    sp24 = *(u8 **)(actor + 0x54);
    actorUpdateScriptVm(actor);
    actorUpdateModel(actor);
    if (actorResolveStoredTarget(actor) != 0) {
        *(f32 *)(actor + 0x18) = *(f32 *)(sp24 + 0x1EC);
        *(f32 *)(actor + 0x1C) = *(f32 *)(sp24 + 0x1F0);
        *(f32 *)(actor + 0x20) = *(f32 *)(sp24 + 0x1F4);
        *(f32 *)(actor + 0x1C) = *(f32 *)(actor + 0x24) + *(f32 *)(actor + 0x1C);
        *(f32 *)(actor + 0x44) = *(f32 *)(actor + 0x18);
        *(f32 *)(actor + 0x48) = *(f32 *)(actor + 0x1C);
        *(f32 *)(actor + 0x4C) = *(f32 *)(actor + 0x20);
    }
}

/// Initialize roll motion state using the object's current roll.
/// \param object Object receiving roll motion state.
/// \returns The input object.
/// \note Original decompilation identifier: \c func_8001DADC (0x8001DADC).
u8 *objectInitializeRollMotionState(u8 *object) {
    f32 *rollState = heapAllocLocal(&gGameHeap, 8, 0x61);
    _bzero(rollState, 8);
    *rollState = *(f32 *)(object + 0x14);
    *(f32 **)(object + 0x54) = rollState;
    return object;
}

/// Clear the object's roll velocity.
/// \param object Object with initialized roll motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DB30 (0x8001DB30).
void objectResetRollVelocity(u8 *object) {
    u8 *v = *(u8 **)(object + 0x54);

    *(f32 *)(v + 4) = 0.0f;
    if (fabsf(*(f32 *)(v + 4)) < 1.0f) {
        *(f32 *)(v + 4) = 0.0;
    } else {
        *(f32 *)(object + 0x14) += *(f32 *)(v + 4) * gRuntimeFrameSeconds;
    }
}

void actorUpdateChildMotion(u8 *);

/// Advance child motion while spinning toward decreasing yaw.
/// \param child Child object whose motion and yaw are updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DB84 (0x8001DB84).
void actorUpdateChildWithNegativeYawSpin(u8 *child) {
    actorUpdateChildMotion(child);
    ((RuntimeObjectPrefix *)child)->rotation[1] =
        (f32)(((RuntimeObjectPrefix *)child)->rotation[1] - (240.0f * gRuntimeFrameSeconds));
    if (((RuntimeObjectPrefix *)child)->rotation[1] < 0.0f) {
        do {
            ((RuntimeObjectPrefix *)child)->rotation[1] = (f32)(((RuntimeObjectPrefix *)child)->rotation[1] + 360.0f);
        } while (((RuntimeObjectPrefix *)child)->rotation[1] < 0.0f);
    }
    if (((RuntimeObjectPrefix *)child)->rotation[1] >= 360.0f) {
        do {
            ((RuntimeObjectPrefix *)child)->rotation[1] = (f32)(((RuntimeObjectPrefix *)child)->rotation[1] - 360.0f);
        } while (((RuntimeObjectPrefix *)child)->rotation[1] >= 360.0f);
    }
}

/// Advance child motion while spinning toward increasing yaw.
/// \param child Child object whose motion and yaw are updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DC40 (0x8001DC40).
void actorUpdateChildWithPositiveYawSpin(u8 *child) {
    actorUpdateChildMotion(child);
    ((RuntimeObjectPrefix *)child)->rotation[1] =
        (f32)(((RuntimeObjectPrefix *)child)->rotation[1] + (240.0f * gRuntimeFrameSeconds));
    if (((RuntimeObjectPrefix *)child)->rotation[1] < 0.0f) {
        do {
            ((RuntimeObjectPrefix *)child)->rotation[1] = (f32)(((RuntimeObjectPrefix *)child)->rotation[1] + 360.0f);
        } while (((RuntimeObjectPrefix *)child)->rotation[1] < 0.0f);
    }
    if (((RuntimeObjectPrefix *)child)->rotation[1] >= 360.0f) {
        do {
            ((RuntimeObjectPrefix *)child)->rotation[1] = (f32)(((RuntimeObjectPrefix *)child)->rotation[1] - 360.0f);
        } while (((RuntimeObjectPrefix *)child)->rotation[1] >= 360.0f);
    }
}

/// Integer texture animation program used by animated billboards.
/// \note Original identifier: \c D_8006B384 (0x8006B384).
extern s32 gBillboardTextureAnimation;

/// Initialize texture animation for a billboard object.
/// \param object Object receiving texture-animation state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DCFC (0x8001DCFC).
void objectInitializeBillboardTextureAnimation(u8 *object) {
    s32 *sp1C;
    s32 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0xC, 0x93);
    sp1C = allocation;
    animationIntegerTrackInitialize((ValueTrack *)((f32 *)allocation), (ValueKey *)(gBillboardTextureAnimation));
    ((RuntimeObjectPrefix *)object)->state.words = sp1C;
}

/// Orient a billboard object toward the active camera.
/// \param object Billboard object whose facing is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DD4C (0x8001DD4C).
void objectUpdateCameraFacingBillboard(u8 *object) {
    objectOrientTowardCamera(object);
    *(u16 *)(object + 0x36) |= 0x1000;
}

/// Advance a billboard's texture animation and prepare its current frame for drawing.
/// \param object Billboard object with initialized texture-animation state.
/// \returns Borrowed object model display-list address represented as an integer.
/// \note Original decompilation identifier: \c func_8001DD7C (0x8001DD7C).
s32 objectDrawAnimatedBillboardTexture(u8 *object) {
    s32 t;
    s32 sp28;

    t = *(s32 *)(object + 0x54);
    animationIntegerTrackEvaluate((ValueTrack *)((void *)t), &sp28, gRuntimeFrameSeconds);
    gDPPipeSync(gRenderDisplayListCursor++);
    gSPSetOtherMode(gRenderDisplayListCursor++, G_SETOTHERMODE_L, 3, 29, 0x1049D8);
    gDPSetCombine(gRenderDisplayListCursor++, 0xFFFEAC, 0xF0FCF238);
    textureLoadForDraw(&gRenderDisplayListCursor, 5, sp28);
    return *(s32 *)(object + 0x30);
}

/// Yaw sway phase increment in degrees per child-motion update.
/// \note Original identifier: \c D_800A4774 (0x800A4774).
extern f32 gActorChildSwayYawPhaseStep;
/// Roll sway phase increment in degrees per child-motion update.
/// \note Original identifier: \c D_800A4778 (0x800A4778).
extern f32 gActorChildSwayRollPhaseStep;
f32 mathSinDegrees(f32);
typedef struct SwayObject {
    u8 pad0[0xC];
    struct {
        f32 x, y, z;
    } rotation;
    u8 pad18[0x14];
    struct SwayObject *linked;
} SwayObject;

/// Advance child motion, pass its orientation to the first child, and apply yaw and roll sway to the object.
/// \param object Object to update; must have a first child to receive the motion orientation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DE24 (0x8001DE24).
void actorUpdateChildMotionWithSway(SwayObject *object) {
    static f32 yawPhase = 0.0f;
    static f32 rollPhase = 0.0f;
    SwayObject *linked;

    actorUpdateChildMotion((u8 *)object);
    linked = object->linked;
    linked->rotation.x = object->rotation.x;
    linked->rotation.y = object->rotation.y;
    linked->rotation.z = object->rotation.z;
    object->rotation.x = 0.0f;
    object->rotation.y = mathSinDegrees(yawPhase) * 20.0f;
    object->rotation.z = mathSinDegrees(rollPhase) * 5.0f;
    yawPhase += gActorChildSwayYawPhaseStep;
    rollPhase += gActorChildSwayRollPhaseStep;
    while (yawPhase < 0.0f) {
        yawPhase += 360.0f;
    }
    while (yawPhase >= 360.0f) {
        yawPhase -= 360.0f;
    }
    while (rollPhase < 0.0f) {
        rollPhase += 360.0f;
    }
    while (rollPhase >= 360.0f) {
        rollPhase -= 360.0f;
    }
}

/// Advance child motion and apply its resulting orientation to its first child.
/// \param object Object to update; must have a first child to receive its orientation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DF94 (0x8001DF94).
void actorUpdateChildMotionAndCopyOrientation(u8 *object) {
    u8 *firstChild;

    actorUpdateChildMotion(object);
    firstChild = ((RuntimeObjectPrefix *)object)->child.bytes;
    ((RuntimeObjectPrefix *)firstChild)->rotation[0] = (f32)((RuntimeObjectPrefix *)object)->rotation[0];
    ((RuntimeObjectPrefix *)firstChild)->rotation[1] = (f32)((RuntimeObjectPrefix *)object)->rotation[1];
    ((RuntimeObjectPrefix *)firstChild)->rotation[2] = (f32)((RuntimeObjectPrefix *)object)->rotation[2];
}

typedef struct {
    s32 unknown;
    s32 compressed;
    s32 romStart;
    s32 romEnd;
    s32 dataStart;
    s32 dataEnd;
    s32 remaining[3];
} ObjectAsset;
/// Actor resource descriptors recording compression, ROM ranges and relocated data ranges.
/// \note Original identifier: \c D_800792A0 (0x800792A0).
extern ObjectAsset gActorAssetResources[];

/// Initialize the actor asset cache and its read queue.
/// \returns Nothing.
///
/// \note NONPORTABLE: the descriptor consistency check has a compiled-out body retained for matching code generation.
/// \note Original decompilation identifier: \c func_8001DFD4 (0x8001DFD4).
void actorAssetCacheInitialize(void) {
    s32 i;

    _bzero(gActorAssetStorage, 0x90);
    romDmaInitializeQueue(&gActorAssetReadQueue, 0x24);
    for (i = 0; i < 36; i++) {
        if (gActorAssetResources[i].unknown != i) {
        }
    }
}


/// Release the actor asset read queue.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001E028 (0x8001E028).
void actorAssetReadQueueDestroy(void) {
    romDmaDestroyQueue(&gActorAssetReadQueue);
}

/// Pending compressed actor asset storage address, or zero when none is retained.
/// \note Original identifier: \c D_800B6C48 (0x800B6C48).
extern s32 gActorCompressedAssetStaging;

/// Reserve storage and request a read for an actor kind's asset if it is not already cached.
/// Complete any outstanding compressed-asset load before requesting another uncached asset.
/// \param kind Valid actor kind whose asset is requested.
/// \returns Nothing; compressed assets still require decompression before use.
/// \note Original decompilation identifier: \c func_8001E04C (0x8001E04C).
void actorAssetQueueLoadForKind(s32 kind) {
    s32 index;
    s32 size;
    s32 source;

    index = *(s32 *)(gActorKindResourceIndexEntries + kind * 0x54);
    if (gActorAssetStorage[index] == 0) {
        size = gActorAssetResources[index].romEnd - gActorAssetResources[index].romStart;
        if (gActorAssetResources[index].compressed == 0) {
            source = (s32)heapAllocLocal(&gGameHeap, size, 0x100);
            gActorCompressedAssetStaging = 0;
            gActorAssetStorage[index] = source;
        } else {
            source = (s32)heapAllocTailLocal(&gGameHeap, size, 0x10D);
            gActorCompressedAssetStaging = source;
            gActorAssetStorage[index] = (s32)heapAllocLocal(
                &gGameHeap, gActorAssetResources[index].dataEnd - gActorAssetResources[index].dataStart, 0x113);
        }
        romDmaReadQueued(&gActorAssetReadQueue, gActorAssetResources[index].romStart, (u8 *)source, size);
    }
}


/// Wait for pending actor asset reads to complete.
/// \returns Nothing; completion does not decompress compressed assets.
/// \note Original decompilation identifier: \c func_8001E16C (0x8001E16C).
void actorAssetWaitForQueuedReads(void) {
    romDmaWaitForQueue(&gActorAssetReadQueue);
}

/// Get the cached asset storage reserved for an actor kind.
/// \param kind Valid actor kind whose asset storage is requested.
/// \returns Borrowed storage address owned by the asset cache, or zero if no load was requested; contents may not yet be ready for use.
/// \note Original decompilation identifier: \c func_8001E190 (0x8001E190).
s32 actorAssetGetCachedStorage(s32 kind) {
    s32 idx;
    u32 off;
    s32 *tbl;
    off = kind * 0x54;
    do {
        tbl = gActorAssetStorage;
        idx = *(s32 *)(gActorKindResourceIndexEntries + off);
        off = tbl[idx];
    } while (0);
    return off;
}

void objectPrepareDistributedKindSearches(u16, s32);
u8 *objectGetLastSibling(u8 *);
void objectAllocateHierarchyMatrixBuffers(s32);
void objectScaleHierarchyBounds(u8 *, f32);
s32 objectRefreshHierarchyWorldPositions(s32);
void actorAddRotationAxisDegrees(u8 *, f32, s32);
void actorInitializeAnimatedTextures(u8 *, s32 *);
void actorPopulatePartTable(u8 *, u8 *);
void actorAssetQueueLoadForKind(s32);
u8 *orbitFollowerCreate(u8 *, f32, f32, f32);
u8 *groundShadowCreate(s32, f32);
u8 *scriptVmCreate(u8 *);
/// Separate symbol view of the actor-kind configuration for Pikachu, kind 0x25.
/// \note Original identifier: \c D_80088BEC (0x80088BEC).
extern u8 gPikachuKindConfiguration[];
/// Heap diagnostic label before actor resource loading.
/// \note Original identifier: \c D_800A465C (0x800A465C).
extern u8 gActorCreationStartHeapLabel[];
/// Heap diagnostic label after actor resource requests.
/// \note Original identifier: \c D_800A46D0 (0x800A46D0).
extern u8 gActorResourcesRequestedHeapLabel[];
/// Heap diagnostic label after actor hierarchy allocation.
/// \note Original identifier: \c D_800A46E8 (0x800A46E8).
extern u8 gActorHierarchyAllocatedHeapLabel[];
/// Heap diagnostic label after hierarchy matrix allocation.
/// \note Original identifier: \c D_800A46F8 (0x800A46F8).
extern u8 gActorMatricesAllocatedHeapLabel[];
/// Heap diagnostic label after actor part-table preparation.
/// \note Original identifier: \c D_800A470C (0x800A470C).
extern u8 gActorPartTablePreparedHeapLabel[];
/// Heap diagnostic label before releasing decompression staging storage.
/// \note Original identifier: \c D_800A4744 (0x800A4744).
extern u8 gActorStagingReleaseStartHeapLabel[];
/// Heap diagnostic label after releasing decompression staging storage.
/// \note Original identifier: \c D_800A4754 (0x800A4754).
extern u8 gActorStagingReleasedHeapLabel[];
/// Heap diagnostic label at actor creation completion.
/// \note Original identifier: \c D_800A4764 (0x800A4764).
extern u8 gActorCreationCompleteHeapLabel[];
/// Scale applied to per-segment actor ground-shadow radius.
/// \note Original identifier: \c D_800A477C (0x800A477C).
extern f32 gActorShadowSegmentRadiusScale;

/// Persistent actor-kind configuration used by actor creation.
/// Pointer fields borrow templates, text and part lists from resident resources.
/// Interior linker views elsewhere retain the original per-field relocations.
typedef struct ActorKindConfig {
    char *alternateName;           ///< Preferred inspection name when alternate text is requested.
    char *alternateDescription;    ///< Preferred inspection description when alternate text is requested.
    char *defaultName;             ///< Default inspection name.
    char *defaultDescription;      ///< Default inspection description.
    f32 markerDepth;               ///< Unscaled depth used to place an interaction marker.
    f32 markerHeight;              ///< Unscaled height used to place an interaction marker.
    ObjectTemplate *hierarchyTemplate;         ///< Borrowed actor hierarchy cloned during creation.
    const s32 *groundReferenceIds; ///< Positive part IDs terminated by a nonpositive value.
    s32 *animatedTextureParts;     ///< Borrowed part list for texture-animation initialization.
    f32 visibilityRadius;          ///< Unscaled actor visibility and segmented-shadow radius.
    f32 collisionDepth;            ///< Unscaled vertical span used by horizontal clearance queries.
    f32 clearanceHeight;           ///< Unscaled permitted clearance height.
    f32 collisionWidth;            ///< Unscaled horizontal collision width.
    f32 unknown34;                 ///< Unresolved dimension copied to actor state at 0x208.
    f32 groundOffset;              ///< Unscaled ground-reference offset.
    s32 shadowSegmentCount;        ///< Number of ground-shadow segments, or nonpositive to omit the shadow.
    s32 defaultPartId;             ///< Default reference part identifier.
    s32 firstAttachmentPartId;     ///< First attachment reference part identifier.
    s32 secondAttachmentPartId;    ///< Second attachment reference part identifier.
    s32 initialBehaviorFlags;      ///< Initial actor behavior flags.
    s32 resourceIndex;             ///< Actor resource-table index.
} ActorKindConfig;

typedef char ActorKindConfigSizeCheck[(sizeof(ActorKindConfig) == 0x54) ? 1 : -1];
#define ACTOR_KIND_OFFSET_CHECK(name, member, offset)                                                                  \
    typedef char name[(((u32) & ((ActorKindConfig *)0)->member) == (offset)) ? 1 : -1]
ACTOR_KIND_OFFSET_CHECK(ActorKindMarkerDepthOffsetCheck, markerDepth, 0x10);
ACTOR_KIND_OFFSET_CHECK(ActorKindMarkerHeightOffsetCheck, markerHeight, 0x14);
ACTOR_KIND_OFFSET_CHECK(ActorKindTemplateOffsetCheck, hierarchyTemplate, 0x18);
ACTOR_KIND_OFFSET_CHECK(ActorKindGroundReferencesOffsetCheck, groundReferenceIds, 0x1C);
ACTOR_KIND_OFFSET_CHECK(ActorKindTexturePartsOffsetCheck, animatedTextureParts, 0x20);
ACTOR_KIND_OFFSET_CHECK(ActorKindCollisionDepthOffsetCheck, collisionDepth, 0x28);
ACTOR_KIND_OFFSET_CHECK(ActorKindDefaultPartOffsetCheck, defaultPartId, 0x40);
ACTOR_KIND_OFFSET_CHECK(ActorKindFlagsOffsetCheck, initialBehaviorFlags, 0x4C);
ACTOR_KIND_OFFSET_CHECK(ActorKindResourceOffsetCheck, resourceIndex, 0x50);
#undef ACTOR_KIND_OFFSET_CHECK

/// Create and register an actor of the requested kind with its resources ready for use.
/// \param kind Valid actor kind to create.
/// \param x Initial world-space X coordinate.
/// \param y Initial world-space Y coordinate.
/// \param z Initial world-space Z coordinate.
/// \param pitch Initial pitch adjustment in degrees.
/// \param yaw Initial yaw adjustment in degrees.
/// \param scale Uniform actor scale.
/// \param script Script configuration whose referenced program and command tables remain valid for the script lifetime, or null to omit script state.
/// \param scriptArgument Argument retained for the actor's script.
/// \returns Newly initialized and registered actor.
/// \note Original decompilation identifier: \c func_8001E1C4 (0x8001E1C4).
/// \note An unused local word retains the target compiler stack layout required by the ROM.
u8 *actorCreateForKind(s32 kind, f32 x, f32 y, f32 z, f32 pitch, f32 yaw, f32 scale, u8 *script, s32 scriptArgument) {
    ActorKindConfig *kindConfig;
    s32 shadowSegments;
    s32 reservedCreationSlot;
    u8 *sp50;
    u8 *actor;
    u8 *sp48;
    u8 *actorState;
    ObjectAsset *asset;
    u8 *part;

    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorCreationStartHeapLabel);
    actorAssetQueueLoadForKind(kind);
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorResourcesRequestedHeapLabel);
    kindConfig = &((ActorKindConfig *)gActorKindConfigurationsFromOne)[kind - 1];
    actor = objectCloneTemplateHierarchy(kindConfig->hierarchyTemplate, 0);
    actorState = *(u8 **)(actor + 0x54);
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorHierarchyAllocatedHeapLabel);
    *(s32 *)(actorState + 0x194) = kindConfig->initialBehaviorFlags;
    *(s32 *)(actorState + 0x238) = scriptArgument;
    sp50 = objectCreateDefault();
    *(u8 **)(sp50 + 0x50) = actor;
    *(u8 **)(objectGetLastSibling(actor) + 0x28) = sp50;
    *(u8 **)(actorState + 0x0) = sp50;
    if (kindConfig->shadowSegmentCount > 0) {
        sp48 = objectGetLastSibling(actor);
        shadowSegments = kindConfig->shadowSegmentCount;
        part = groundShadowCreate(shadowSegments,
                                  (kindConfig->visibilityRadius / (f32)shadowSegments) * gActorShadowSegmentRadiusScale);
        *(u8 **)(part + 0x50) = actor;
        *(u8 **)(sp48 + 0x28) = part;
    }
    objectAllocateHierarchyMatrixBuffers((s32)actor);
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorMatricesAllocatedHeapLabel);
    actorPopulatePartTable(actor, actor);
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorPartTablePreparedHeapLabel);
    *(f32 *)(actor + 0x18) = x;
    *(f32 *)(actor + 0x1C) = y;
    *(f32 *)(actor + 0x20) = z;
    *(f32 *)(actor + 0x8) = scale;
    *(f32 *)(actor + 0x4) = scale;
    *(f32 *)(actor + 0x0) = scale;
    actorAddRotationAxisDegrees(actor, pitch, 0);
    actorAddRotationAxisDegrees(actor, yaw, 1);
    *(f32 *)(actor + 0x24) = (f32)kindConfig->visibilityRadius;
    if (kindConfig == (ActorKindConfig *)gPikachuKindConfiguration) {
        part = actorFindPart(actor, 0x2F);
        if (part != NULL) {
            *(f32 *)(part + 0x24) = 80.0f;
        }
        part = actorFindPart(actor, 0x2D);
        if (part != NULL) {
            *(f32 *)(part + 0x24) = 80.0f;
        }
    }
    *(f32 *)(actorState + 0x1FC) = (f32)(kindConfig->collisionDepth * scale);
    *(f32 *)(actorState + 0x200) = (f32)(kindConfig->clearanceHeight * scale);
    *(f32 *)(actorState + 0x204) = (f32)(kindConfig->collisionWidth * scale);
    *(f32 *)(actorState + 0x208) = (f32)(kindConfig->unknown34 * scale);
    *(f32 *)(actorState + 0x20C) = (f32)(kindConfig->groundOffset * scale);
    objectScaleHierarchyBounds(actor, 1.0f);
    objectRefreshHierarchyWorldPositions((s32)actor);
    if (kindConfig->animatedTextureParts != NULL) {
        actorInitializeAnimatedTextures(actor, kindConfig->animatedTextureParts);
    }
    objectRegisterActiveHierarchy(actor);
    objectPrepareDistributedKindSearches(0x64, 4);
    *(s32 *)(actorState + 0x234) =
        (s32) * (s32 *)(orbitFollowerCreate(actor, *(f32 *)(actorState + 0x204), *(f32 *)(actorState + 0x1FC),
                                            *(f32 *)(actorState + 0x200)) +
                        0x38);
    if (script != 0) {
        part = scriptVmCreate(script);
        *(u8 **)(actorState + 0x1B8) = part;
        *(s32 *)part = 0;
    }
    actorAssetWaitForQueuedReads();
    if ((u8 *)gActorCompressedAssetStaging != NULL) {
        asset = &gActorAssetResources[kindConfig->resourceIndex];
        if (asset->compressed == 1) {
            sp50 = (void *)gActorAssetResources[kindConfig->resourceIndex].dataStart;
            assetDecompressAndPatch((u8 *)gActorAssetStorage[kindConfig->resourceIndex],
                                    (u8 *)gActorCompressedAssetStaging, asset->remaining[0] - (s32)sp50,
                                    asset->dataEnd - (s32)sp50, asset->remaining[1] - (s32)sp50,
                                    asset->remaining[2] - (s32)sp50);
            heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorStagingReleaseStartHeapLabel);
            heapFreeLocal(&gGameHeap, (void *)gActorCompressedAssetStaging);
            gActorCompressedAssetStaging = 0;
            heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorStagingReleasedHeapLabel);
        }
    }
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorCreationCompleteHeapLabel);
    return actor;
}

/// Replace an actor with another kind while preserving its position, pitch, yaw, scale, and script ownership.
/// \param actor Actor whose script is transferred and whose hierarchy is scheduled for destruction.
/// \param kind Valid actor kind for the replacement.
/// \returns New replacement actor.
/// \note Original decompilation identifier: \c func_8001E568 (0x8001E568).
u8 *actorReplaceKindPreservingScript(u8 *actor, s32 kind) {
    u8 *replacementActor;
    replacementActor = actorCreateForKind(kind, *(f32 *)(actor + 0x44), *(f32 *)(actor + 0x48), *(f32 *)(actor + 0x4C),
                                          *(f32 *)(actor + 0xC), *(f32 *)(actor + 0x10), *(f32 *)(actor + 0), 0,
                                          *(s32 *)(*(u8 **)(actor + 0x54) + 0x238));
    *(s32 *)(*(u8 **)(replacementActor + 0x54) + 0x1B8) = *(s32 *)(*(u8 **)(actor + 0x54) + 0x1B8);
    *(s32 *)(*(u8 **)(actor + 0x54) + 0x1B8) = 0;
    objectRequestHierarchyDestruction(actor);
    return replacementActor;
}

/// Get the configured parts used to measure an actor's ground clearance.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Borrowed part-ID list; positive IDs end at a nonpositive value.
/// \note Original decompilation identifier: \c func_8001E5F0 (0x8001E5F0).
const s32 *actorConfigGetGroundReferencePartIds(s32 kind) {
    return *(const s32 **)(gActorKindGroundReferencePartEntries + (kind * 0x54));
}

/// Get the first configured part used for actor attachment positions.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns First attachment part ID.
/// \note Original decompilation identifier: \c func_8001E614 (0x8001E614).
s32 actorConfigGetFirstAttachmentPartId(s32 kind) {
    return *(s32 *)(gActorKindFirstAttachmentPartEntries + (kind * 0x54));
}

/// Get the second configured part used for actor attachment positions.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Second attachment part ID.
/// \note Original decompilation identifier: \c func_8001E638 (0x8001E638).
s32 actorConfigGetSecondAttachmentPartId(s32 kind) {
    return *(s32 *)(gActorKindSecondAttachmentPartEntries + (kind * 0x54));
}

/// Get the configured default part for an actor kind.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Default part ID.
/// \note Original decompilation identifier: \c func_8001E65C (0x8001E65C).
s32 actorConfigGetDefaultPartId(s32 kind) {
    return *(s32 *)(gActorKindDefaultPartEntries + (kind * 0x54));
}

/// Get an actor kind's display name for inspection text.
/// \param kind Valid actor kind whose text is requested.
/// \param useDefault Nonzero selects the default name; zero prefers the alternate name when available.
/// \returns Borrowed selected name string, or null when no name is configured.
/// \note Original decompilation identifier: \c func_8001E680 (0x8001E680).
char *actorConfigGetDisplayName(s32 kind, s32 useDefault) {
    char *alternateName;

    if ((useDefault != 0) ||
        (alternateName = *(char **)(gActorKindAlternateNameEntries + kind * 0x54), (alternateName == NULL))) {
        return *(char **)(gActorKindDefaultNameEntries + kind * 0x54);
    }
    return alternateName;
}

/// Get an actor kind's description for inspection text.
/// \param kind Valid actor kind whose text is requested.
/// \param useDefault Nonzero selects the default description; zero prefers the alternate description when available.
/// \returns Borrowed selected description string, or null when no description is configured.
/// \note Original decompilation identifier: \c func_8001E6D8 (0x8001E6D8).
char *actorConfigGetDescription(s32 kind, s32 useDefault) {
    char *alternateDescription;

    if ((useDefault != 0) ||
        (alternateDescription = *(char **)(gActorKindAlternateDescriptionEntries + kind * 0x54),
         (alternateDescription == NULL))) {
        return *(char **)(gActorKindDefaultDescriptionEntries + kind * 0x54);
    }
    return alternateDescription;
}

/// Get actor dimensions used to position an interaction marker.
/// \param kind Valid actor kind whose dimensions are requested.
/// \param dimensions Receives zero horizontal offset, unscaled height, and unscaled depth, in that order.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001E730 (0x8001E730).
void actorConfigGetInteractionMarkerDimensions(s32 kind, f32 *dimensions) {
    ActorKindConfig *kindConfig;

    kindConfig = &((ActorKindConfig *)gActorKindConfigurationsFromOne)[kind - 1];
    dimensions[0] = 0.0f;
    dimensions[1] = kindConfig->markerHeight;
    dimensions[2] = kindConfig->markerDepth;
}

/// Create and register Togepi with scale adjusted for the current scene and progress.
/// \param x Initial world-space X coordinate.
/// \param y Initial world-space Y coordinate.
/// \param z Initial world-space Z coordinate.
/// \param pitch Initial pitch adjustment in degrees.
/// \param yaw Initial yaw adjustment in degrees.
/// \param script Script configuration, or null to omit script state.
/// \param scriptArgument Argument retained for Togepi's script.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001E76C (0x8001E76C).
void togepiCreate(f32 x, f32 y, f32 z, f32 pitch, f32 yaw, u8 *script, s32 scriptArgument) {
    f32 scale;
    {
        s32 sp30;
        sp30 = progressGetValue(0x18);
        scale = (((f32)sp30 / (f32)progressGetMaximum(0x18)) + 1.0f) * 0.5f;
    }
    actorCreateForKind(0x16, x, y, z, pitch, yaw, scale * *(f32 *)(gCurrentAreaConfiguration + 0xC), script,
                       scriptArgument);
}
