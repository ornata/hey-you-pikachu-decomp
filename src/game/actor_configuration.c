#include "contact_effect.h"
#include "object_response.h"
#include "render_state.h"
#include "game_heap.h"
#include "ground_shadow.h"
#include "texture_setup.h"
#include "pikachu_actor.h"
#include "vector_metrics.h"
#include "math_angles.h"
#include "terrain_height.h"
#include "linked_effect.h"
#include "script_vm_execution.h"
#include "script_vm_lifecycle.h"
#include "actor_assets.h"
#include "progress_fields.h"
#include "progress_value.h"
#include "actor_rotation.h"
#include "object_transform.h"
#include "object_bounds.h"
#include "object_matrix_buffers.h"
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
#include "scene_configuration.h"
#include "scene_types.h"
/// Integer animation program used before the contact effect is triggered.
/// \note Original identifier: \c D_8006B390 (0x8006B390).
extern s32 gContactEffectIdleAnimation;
/// Cached actor asset storage addresses indexed by resource identifier.
/// \note Original identifier: \c D_800B6BB8 (0x800B6BB8).
extern u8 *gActorAssetStorage[36];
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
#include "actor_model.h"
#include "object_update.h"
#include "orbit_follower.h"
#include "togepi.h"
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

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
    f32 unknown34;                 ///< Unresolved configuration value scaled into actor state at 0x208.
    f32 groundOffset;              ///< Unscaled ground-reference offset.
    s32 shadowSegmentCount;        ///< Number of ground-shadow segments, or nonpositive to omit the shadow.
    s32 defaultPartId;             ///< Default reference part identifier.
    s32 firstAttachmentPartId;     ///< First attachment reference part identifier.
    s32 secondAttachmentPartId;    ///< Second attachment reference part identifier.
    s32 initialAttachmentMode;      ///< Initial attachment-position selection mode.
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
ACTOR_KIND_OFFSET_CHECK(ActorKindAttachmentModeOffsetCheck, initialAttachmentMode, 0x4C);
ACTOR_KIND_OFFSET_CHECK(ActorKindResourceOffsetCheck, resourceIndex, 0x50);
#undef ACTOR_KIND_OFFSET_CHECK



/// Initial uniform scale of a contact-reactive effect.
/// \note Original identifier: \c D_800A3A68 (0x800A3A68).
extern f32 gContactEffectInitialScale;
/// Hierarchy template used to create contact-reactive effects.
/// \note Original identifier: \c D_80079260 (0x80079260).
extern u8 gContactEffectHierarchyTemplate[];

/// Owned contact-reaction state with a texture-animation cursor borrowing resident keys.
typedef struct ContactEffectState {
    s32 triggered;               ///< Nonzero after contact starts the reaction animation.
    ValueTrack textureAnimation; ///< Retained texture frame and playback position.
} ContactEffectState;

typedef char ContactEffectStateSizeCheck[(sizeof(ContactEffectState) == 0x10) ? 1 : -1];
typedef char ContactEffectAnimationOffsetCheck[((u32) & ((ContactEffectState *)0)->textureAnimation == 4) ? 1 : -1];
typedef char ContactEffectFrameOffsetCheck[((u32) & ((ContactEffectState *)0)->textureAnimation.value == 0xC) ? 1 : -1];

/// Create and register a contact-reactive effect at a world position.
/// \param x World-space X coordinate.
/// \param y World-space Y coordinate; negative values request ground placement using the corresponding positive search height.
/// \param z World-space Z coordinate.
/// \returns Newly initialized and registered effect.
/// \note Original decompilation identifier: \c func_8001D6C0 (0x8001D6C0).
u8 *contactEffectCreate(f32 x, f32 y, f32 z) {
    u8 *p = objectCreateHierarchyFromTemplate((ObjectTemplate *)&gContactEffectHierarchyTemplate);
    f32 s;

    if (y < 0.0F) {
        y = terrainGetWorldHeight(x, -y, z, 0) + 1.0F;
    }
    s = gContactEffectInitialScale;
    ((RuntimeObjectPrefix *)p)->position[0] = x;
    ((RuntimeObjectPrefix *)p)->position[1] = y;
    ((RuntimeObjectPrefix *)p)->position[2] = z;
    ((RuntimeObjectPrefix *)p)->worldPosition[0] = x;
    ((RuntimeObjectPrefix *)p)->worldPosition[1] = y;
    ((RuntimeObjectPrefix *)p)->worldPosition[2] = z;
    ((RuntimeObjectPrefix *)p)->scale[2] = s;
    ((RuntimeObjectPrefix *)p)->scale[1] = s;
    ((RuntimeObjectPrefix *)p)->scale[0] = s;
    ((RuntimeObjectPrefix *)p)->radius = 3.0F;
    objectRegisterActiveHierarchy(p);
    return p;
}

/// Initialize an animated effect that reacts to contact with the active actor.
/// \param effect Effect object receiving its initial animation state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001D784 (0x8001D784).
void contactEffectInitialize(u8 *effect) {
    ContactEffectState *savedEffectState;
    ContactEffectState *effectState;
    effectState = heapAllocLocal(&gGameHeap, 0x10, 0x46);
    savedEffectState = effectState;
    effectState->triggered = 0;
    animationIntegerTrackInitialize(&effectState->textureAnimation, (ValueKey *)(gContactEffectIdleAnimation));
    ((RuntimeObjectPrefix *)effect)->state.pointer = savedEffectState;
}

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
    ContactEffectState *state = (ContactEffectState *)((RuntimeObjectPrefix *)effect)->state.pointer;
    f32 delta[3];
    s32 frame;
    s32 result;
    f32 pull;
    if (state->triggered == 0) {
        delta[0] = ((RuntimeObjectPrefix *)gPikachuActor)->worldPosition[0] -
                   ((RuntimeObjectPrefix *)effect)->worldPosition[0];
        delta[1] = ((RuntimeObjectPrefix *)gPikachuActor)->worldPosition[1] -
                   ((RuntimeObjectPrefix *)effect)->worldPosition[1];
        delta[2] = ((RuntimeObjectPrefix *)gPikachuActor)->worldPosition[2] -
                   ((RuntimeObjectPrefix *)effect)->worldPosition[2];
        if ((((RuntimeObjectPrefix *)effect)->radius + ((RuntimeObjectPrefix *)gPikachuActor)->radius) >
            vector3Length(delta)) {
            state->triggered = 1;
            animationIntegerTrackInitialize(&state->textureAnimation, (ValueKey *)(gContactEffectTriggeredAnimation));
        }
    }
    result = animationIntegerTrackEvaluate(&state->textureAnimation, &frame, gRuntimeFrameSeconds);
    if (state->triggered != 0) {
        if (result == 1) {
            objectRequestHierarchyDestruction(effect);
        } else if (result == 2) {
            objectPlaySpatialSound(effect, 0x97, 20.0F, 100.0F);
            result = actorGetPartObjectHandle((u8 *)((s32)gPikachuActor), 0x79);
            pull = gContactEffectChainPull;
            linkedEffectCreateChain(LINKED_EFFECT_REBOUND, ((RuntimeObjectPrefix *)effect)->worldPosition[0],
                                    ((RuntimeObjectPrefix *)effect)->worldPosition[1],
                                    ((RuntimeObjectPrefix *)effect)->worldPosition[2], gContactEffectChainScale, result,
                                    1.0F, gContactEffectChainDamping, 2.0F, pull, pull, 0xC2, 1, 8);
        }
    }
    objectOrientAlongCameraView(effect);
}

/// Prepare the contact effect's current animation texture for drawing.
/// \param effect Initialized effect object to draw.
/// \returns Borrowed effect display list, valid with its resources through graphics-task completion.
/// \note Original decompilation identifier: \c func_8001D96C (0x8001D96C).
Gfx *contactEffectPrepareTextureForDraw(u8 *effect) {
    u8 *obj;

    obj = ((RuntimeObjectPrefix *)effect)->state.bytes;
    textureLoadForDraw(&gRenderDisplayListCursor, 5, ((ContactEffectState *)obj)->textureAnimation.value);
    return ((RuntimeObjectPrefix *)effect)->displayList;
}

/// Trigger an idle contact effect when a response identifies the active actor.
/// \param effect Initialized effect object receiving the response.
/// \param response Borrowed queued response; sourceHandle identifies the sending object.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001D9A4 (0x8001D9A4).
void contactEffectHandleActorResponse(u8 *effect, ObjectResponse *response) {
    ContactEffectState *effectState;
    ContactEffectState *savedEffectState;

    effectState = (ContactEffectState *)((RuntimeObjectPrefix *)effect)->state.pointer;
    if (effectState->triggered == 0) {
        savedEffectState = effectState;
        if (objectFindLiveByHandle(response->sourceHandle) == (u8 *)((s32)gPikachuActor)) {
            effectState->triggered = 1;
            animationIntegerTrackInitialize(&effectState->textureAnimation,
                                            (ValueKey *)(gContactEffectTriggeredAnimation));
        }
    }
}

/// Initialize an actor model and its part lookup storage.
/// \param actor Actor to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DA00 (0x8001DA00).
void actorInitializeModelAndPartTable(u8 *actor) {
    actorInitializeModelState(actor);
    actorAllocatePartTable(actor);
}


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

    sp24 = ((RuntimeObjectPrefix *)actor)->state.bytes;
    actorUpdateScriptVm(actor);
    actorUpdateModel(actor);
    if (actorResolveStoredTarget(actor) != 0) {
        ((RuntimeObjectPrefix *)actor)->position[0] = ((ActorScriptState *)sp24)->targetPosition[0];
        ((RuntimeObjectPrefix *)actor)->position[1] = ((ActorScriptState *)sp24)->targetPosition[1];
        ((RuntimeObjectPrefix *)actor)->position[2] = ((ActorScriptState *)sp24)->targetPosition[2];
        ((RuntimeObjectPrefix *)actor)->position[1] =
            ((RuntimeObjectPrefix *)actor)->radius + ((RuntimeObjectPrefix *)actor)->position[1];
        ((RuntimeObjectPrefix *)actor)->worldPosition[0] = ((RuntimeObjectPrefix *)actor)->position[0];
        ((RuntimeObjectPrefix *)actor)->worldPosition[1] = ((RuntimeObjectPrefix *)actor)->position[1];
        ((RuntimeObjectPrefix *)actor)->worldPosition[2] = ((RuntimeObjectPrefix *)actor)->position[2];
    }
}

/// Owned roll-motion state, separate from actor and child playback state.
typedef struct ObjectRollMotionState {
    f32 initialRoll; ///< Object roll in degrees captured during initialization.
    f32 velocity; ///< Roll change in degrees per second; reset before the current update.
} ObjectRollMotionState;

typedef char ObjectRollMotionStateSizeCheck[(sizeof(ObjectRollMotionState) == 8) ? 1 : -1];
typedef char ObjectRollVelocityOffsetCheck[((u32)&((ObjectRollMotionState *)0)->velocity == 4) ? 1 : -1];

/// Initialize roll motion state using the object's current roll.
/// \param object Object receiving roll motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DADC (0x8001DADC).
void objectInitializeRollMotionState(u8 *object) {
    ObjectRollMotionState *rollState = heapAllocLocal(&gGameHeap, sizeof(ObjectRollMotionState), 0x61);
    _bzero(rollState, sizeof(ObjectRollMotionState));
    rollState->initialRoll = ((RuntimeObjectPrefix *)object)->rotation[2];
    ((RuntimeObjectPrefix *)object)->state.pointer = rollState;
}

/// Clear the object's roll velocity.
/// \param object Object with initialized roll motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DB30 (0x8001DB30).
void objectResetRollVelocity(u8 *object) {
    ObjectRollMotionState *rollState = ((RuntimeObjectPrefix *)object)->state.pointer;

    rollState->velocity = 0.0F;
    if (fabsf(rollState->velocity) < 1.0F) {
        rollState->velocity = 0.0;
    } else {
        ((RuntimeObjectPrefix *)object)->rotation[2] += rollState->velocity * gRuntimeFrameSeconds;
    }
}


/// Advance child motion while spinning toward decreasing yaw.
/// \param child Child object whose motion and yaw are updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DB84 (0x8001DB84).
void actorUpdateChildWithNegativeYawSpin(u8 *child) {
    actorUpdateChildMotion(child);
    ((RuntimeObjectPrefix *)child)->rotation[1] =
        (f32)(((RuntimeObjectPrefix *)child)->rotation[1] - (240.0F * gRuntimeFrameSeconds));
    if (((RuntimeObjectPrefix *)child)->rotation[1] < 0.0F) {
        do {
            ((RuntimeObjectPrefix *)child)->rotation[1] = (f32)(((RuntimeObjectPrefix *)child)->rotation[1] + 360.0F);
        } while (((RuntimeObjectPrefix *)child)->rotation[1] < 0.0F);
    }
    if (((RuntimeObjectPrefix *)child)->rotation[1] >= 360.0F) {
        do {
            ((RuntimeObjectPrefix *)child)->rotation[1] = (f32)(((RuntimeObjectPrefix *)child)->rotation[1] - 360.0F);
        } while (((RuntimeObjectPrefix *)child)->rotation[1] >= 360.0F);
    }
}

/// Advance child motion while spinning toward increasing yaw.
/// \param child Child object whose motion and yaw are updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DC40 (0x8001DC40).
void actorUpdateChildWithPositiveYawSpin(u8 *child) {
    actorUpdateChildMotion(child);
    ((RuntimeObjectPrefix *)child)->rotation[1] =
        (f32)(((RuntimeObjectPrefix *)child)->rotation[1] + (240.0F * gRuntimeFrameSeconds));
    if (((RuntimeObjectPrefix *)child)->rotation[1] < 0.0F) {
        do {
            ((RuntimeObjectPrefix *)child)->rotation[1] = (f32)(((RuntimeObjectPrefix *)child)->rotation[1] + 360.0F);
        } while (((RuntimeObjectPrefix *)child)->rotation[1] < 0.0F);
    }
    if (((RuntimeObjectPrefix *)child)->rotation[1] >= 360.0F) {
        do {
            ((RuntimeObjectPrefix *)child)->rotation[1] = (f32)(((RuntimeObjectPrefix *)child)->rotation[1] - 360.0F);
        } while (((RuntimeObjectPrefix *)child)->rotation[1] >= 360.0F);
    }
}

/// Integer texture animation program used by animated billboards.
/// \note Original identifier: \c D_8006B384 (0x8006B384).
extern ValueKey *gBillboardTextureAnimation;

/// Initialize texture animation for a billboard object.
/// \param object Object receiving an owned integer-animation track; the borrowed key program must outlive playback.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DCFC (0x8001DCFC).
void objectInitializeBillboardTextureAnimation(u8 *object) {
    ValueTrack *track;
    ValueTrack *allocation;

    allocation = heapAllocLocal(&gGameHeap, sizeof(ValueTrack), 0x93);
    track = allocation;
    animationIntegerTrackInitialize(allocation, gBillboardTextureAnimation);
    ((RuntimeObjectPrefix *)object)->state.pointer = track;
}

/// Orient a billboard object toward the active camera.
/// \param object Billboard object whose facing is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DD4C (0x8001DD4C).
void objectUpdateCameraFacingBillboard(u8 *object) {
    objectOrientTowardCamera(object);
    ((RuntimeObjectPrefix *)object)->flags |= 0x1000;
}

/// Advance a billboard's texture animation and prepare its current frame for drawing.
/// \param object Billboard object with initialized texture-animation state.
/// \returns Borrowed object display list, valid with its resources through graphics-task completion.
/// \note Original decompilation identifier: \c func_8001DD7C (0x8001DD7C).
Gfx *objectDrawAnimatedBillboardTexture(u8 *object) {
    ValueTrack *track;
    s32 textureFrame;

    track = ((RuntimeObjectPrefix *)object)->state.pointer;
    animationIntegerTrackEvaluate(track, &textureFrame, gRuntimeFrameSeconds);
    gDPPipeSync(gRenderDisplayListCursor++);
    gSPSetOtherMode(gRenderDisplayListCursor++, G_SETOTHERMODE_L, 3, 29, 0x1049D8);
    gDPSetCombine(gRenderDisplayListCursor++, 0xFFFEAC, 0xF0FCF238);
    textureLoadForDraw(&gRenderDisplayListCursor, 5, textureFrame);
    return ((RuntimeObjectPrefix *)object)->displayList;
}

/// Yaw sway phase increment in degrees per child-motion update.
/// \note Original identifier: \c D_800A4774 (0x800A4774).
extern f32 gActorChildSwayYawPhaseStep;
/// Roll sway phase increment in degrees per child-motion update.
/// \note Original identifier: \c D_800A4778 (0x800A4778).
extern f32 gActorChildSwayRollPhaseStep;
/// Advance child motion, pass its orientation to the first child, and apply yaw and roll sway to the object.
/// \param object Object to update; must have a first child to receive the motion orientation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001DE24 (0x8001DE24).
void actorUpdateChildMotionWithSway(u8 *object) {
    static f32 yawPhase = 0.0F;
    static f32 rollPhase = 0.0F;
    RuntimeObjectPrefix *linked;

    actorUpdateChildMotion(object);
    linked = ((RuntimeObjectPrefix *)object)->child.object;
    linked->rotation[0] = ((RuntimeObjectPrefix *)object)->rotation[0];
    linked->rotation[1] = ((RuntimeObjectPrefix *)object)->rotation[1];
    linked->rotation[2] = ((RuntimeObjectPrefix *)object)->rotation[2];
    ((RuntimeObjectPrefix *)object)->rotation[0] = 0.0F;
    ((RuntimeObjectPrefix *)object)->rotation[1] = mathSinDegrees(yawPhase) * 20.0F;
    ((RuntimeObjectPrefix *)object)->rotation[2] = mathSinDegrees(rollPhase) * 5.0F;
    yawPhase += gActorChildSwayYawPhaseStep;
    rollPhase += gActorChildSwayRollPhaseStep;
    while (yawPhase < 0.0F) {
        yawPhase += 360.0F;
    }
    while (yawPhase >= 360.0F) {
        yawPhase -= 360.0F;
    }
    while (rollPhase < 0.0F) {
        rollPhase += 360.0F;
    }
    while (rollPhase >= 360.0F) {
        rollPhase -= 360.0F;
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

/// Actor cache entry metadata; corrected-asset ranges borrow the resident resource table.
typedef struct ActorAssetDescriptor {
    s32 index; ///< Expected slot in the actor asset resource table.
    s32 compressed; ///< Zero selects a direct ROM copy; one selects corrected decompression at creation.
    AssetDescriptor asset; ///< ROM, decoded and correction ranges with their common bias.
} ActorAssetDescriptor;

typedef char ActorAssetDescriptorSizeCheck[(sizeof(ActorAssetDescriptor) == 0x24) ? 1 : -1];
typedef char ActorAssetCompressionOffsetCheck[((u32)&((ActorAssetDescriptor *)0)->compressed == 4) ? 1 : -1];
typedef char ActorAssetRangeOffsetCheck[((u32)&((ActorAssetDescriptor *)0)->asset == 8) ? 1 : -1];
/// Actor resource descriptors recording compression, ROM ranges and relocated data ranges.
/// \note Original identifier: \c D_800792A0 (0x800792A0).
extern ActorAssetDescriptor gActorAssetResources[];

/// Initialize the actor asset cache and its read queue.
/// \returns Nothing.
///
/// \note NONPORTABLE: the descriptor consistency check has a compiled-out body retained for matching code generation.
/// \note Original decompilation identifier: \c func_8001DFD4 (0x8001DFD4).
void actorAssetCacheInitialize(void) {
    s32 i;

    _bzero(gActorAssetStorage, sizeof(gActorAssetStorage));
    romDmaInitializeQueue(&gActorAssetReadQueue, 0x24);
    for (i = 0; i < 36; i++) {
        if (gActorAssetResources[i].index != i) {
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
extern u8 *gActorCompressedAssetStaging;

/// Reserve storage and request a read for an actor kind's asset if it is not already cached.
/// Complete any outstanding compressed-asset load before requesting another uncached asset.
/// \param kind Valid actor kind whose asset is requested.
/// \returns Nothing; compressed assets still require decompression before use.
/// \note Original decompilation identifier: \c func_8001E04C (0x8001E04C).
void actorAssetQueueLoadForKind(s32 kind) {
    s32 index;
    s32 size;
    u8 *source;

    index = *(s32 *)(gActorKindResourceIndexEntries + (kind * sizeof(ActorKindConfig)));
    if (gActorAssetStorage[index] == 0) {
        size = gActorAssetResources[index].asset.romEnd - gActorAssetResources[index].asset.romAddress;
        if (gActorAssetResources[index].compressed == 0) {
            source = heapAllocLocal(&gGameHeap, size, 0x100);
            gActorCompressedAssetStaging = 0;
            gActorAssetStorage[index] = source;
        } else {
            source = heapAllocTailLocal(&gGameHeap, size, 0x10D);
            gActorCompressedAssetStaging = source;
            gActorAssetStorage[index] = heapAllocLocal(
                &gGameHeap, gActorAssetResources[index].asset.decodedEnd - gActorAssetResources[index].asset.rangeBase, 0x113);
        }
        romDmaReadQueued(&gActorAssetReadQueue, gActorAssetResources[index].asset.romAddress, source, size);
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
u8 *actorAssetGetCachedStorage(s32 kind) {
    s32 idx;
    u32 off;
    u8 **tbl;
    off = kind * sizeof(ActorKindConfig);
    do {
        tbl = gActorAssetStorage;
        idx = *(s32 *)(gActorKindResourceIndexEntries + off);
        off = (u32)tbl[idx];
    } while (0);
    return (u8 *)off;
}

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
    ActorAssetDescriptor *descriptor;
    u8 *part;

    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorCreationStartHeapLabel);
    actorAssetQueueLoadForKind(kind);
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorResourcesRequestedHeapLabel);
    kindConfig = &((ActorKindConfig *)gActorKindConfigurationsFromOne)[kind - 1];
    actor = objectCloneTemplateHierarchy(kindConfig->hierarchyTemplate, 0);
    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorHierarchyAllocatedHeapLabel);
    ((ActorScriptState *)actorState)->attachmentMode = kindConfig->initialAttachmentMode;
    ((ActorScriptState *)actorState)->creationScriptArgument = scriptArgument;
    sp50 = objectCreateDefault();
    ((RuntimeObjectPrefix *)sp50)->root.bytes = actor;
    ((RuntimeObjectPrefix *)objectGetLastSibling(actor))->sibling.bytes = sp50;
    ((ActorScriptState *)actorState)->groundReference = sp50;
    if (kindConfig->shadowSegmentCount > 0) {
        sp48 = objectGetLastSibling(actor);
        shadowSegments = kindConfig->shadowSegmentCount;
        part = groundShadowCreate(shadowSegments,
                                  (kindConfig->visibilityRadius / (f32)shadowSegments) * gActorShadowSegmentRadiusScale);
        ((RuntimeObjectPrefix *)part)->root.bytes = actor;
        ((RuntimeObjectPrefix *)sp48)->sibling.bytes = part;
    }
    objectAllocateHierarchyMatrixBuffers(actor);
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorMatricesAllocatedHeapLabel);
    actorPopulatePartTable(actor, actor);
    heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorPartTablePreparedHeapLabel);
    ((RuntimeObjectPrefix *)actor)->position[0] = x;
    ((RuntimeObjectPrefix *)actor)->position[1] = y;
    ((RuntimeObjectPrefix *)actor)->position[2] = z;
    ((RuntimeObjectPrefix *)actor)->scale[2] = scale;
    ((RuntimeObjectPrefix *)actor)->scale[1] = scale;
    ((RuntimeObjectPrefix *)actor)->scale[0] = scale;
    actorAddRotationAxisDegrees(actor, pitch, 0);
    actorAddRotationAxisDegrees(actor, yaw, 1);
    ((RuntimeObjectPrefix *)actor)->radius = (f32)kindConfig->visibilityRadius;
    if (kindConfig == (ActorKindConfig *)gPikachuKindConfiguration) {
        part = actorFindPart(actor, 0x2F);
        if (part != NULL) {
            ((RuntimeObjectPrefix *)part)->radius = 80.0F;
        }
        part = actorFindPart(actor, 0x2D);
        if (part != NULL) {
            ((RuntimeObjectPrefix *)part)->radius = 80.0F;
        }
    }
    ((ActorScriptState *)actorState)->collisionDepth = (f32)(kindConfig->collisionDepth * scale);
    ((ActorScriptState *)actorState)->clearanceHeight = (f32)(kindConfig->clearanceHeight * scale);
    ((ActorScriptState *)actorState)->collisionWidth = (f32)(kindConfig->collisionWidth * scale);
    ((ActorScriptState *)actorState)->unknown208 = (f32)(kindConfig->unknown34 * scale);
    ((ActorScriptState *)actorState)->groundOffset = (f32)(kindConfig->groundOffset * scale);
    objectScaleHierarchyBounds(actor, 1.0F);
    objectRefreshHierarchyWorldPositions(actor);
    if (kindConfig->animatedTextureParts != NULL) {
        actorInitializeAnimatedTextures(actor, kindConfig->animatedTextureParts);
    }
    objectRegisterActiveHierarchy(actor);
    objectPrepareDistributedKindSearches(0x64, 4);
    ((ActorScriptState *)actorState)->fallbackObjectHandle =
        ((RuntimeObjectPrefix *)orbitFollowerCreate(actor, ((ActorScriptState *)actorState)->collisionWidth,
                                                   ((ActorScriptState *)actorState)->collisionDepth,
                                                   ((ActorScriptState *)actorState)->clearanceHeight))->handle;
    if (script != 0) {
        part = (u8 *)scriptVmCreate((ScriptVmConfiguration *)script);
        ((ActorScriptState *)actorState)->vmAddress = (s32)part;
        ((ScriptVm *)part)->executionState = SCRIPT_VM_READY;
    }
    actorAssetWaitForQueuedReads();
    if (gActorCompressedAssetStaging != NULL) {
        descriptor = &gActorAssetResources[kindConfig->resourceIndex];
        if (descriptor->compressed == 1) {
            sp50 = (void *)gActorAssetResources[kindConfig->resourceIndex].asset.rangeBase;
            assetDecompressAndPatch(gActorAssetStorage[kindConfig->resourceIndex],
                                    gActorCompressedAssetStaging, descriptor->asset.encodedEnd - (s32)sp50,
                                    descriptor->asset.decodedEnd - (s32)sp50, descriptor->asset.patchStart - (s32)sp50,
                                    descriptor->asset.patchCountValue - (s32)sp50);
            heapDebugReportFreeSpace(&gGameHeap, (const char *)gActorStagingReleaseStartHeapLabel);
            heapFreeLocal(&gGameHeap, gActorCompressedAssetStaging);
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
    replacementActor = actorCreateForKind(kind, ((RuntimeObjectPrefix *)actor)->worldPosition[0], ((RuntimeObjectPrefix *)actor)->worldPosition[1], ((RuntimeObjectPrefix *)actor)->worldPosition[2],
                                          ((RuntimeObjectPrefix *)actor)->rotation[0], ((RuntimeObjectPrefix *)actor)->rotation[1], ((RuntimeObjectPrefix *)actor)->scale[0], 0,
                                          ((RuntimeObjectPrefix *)actor)->state.actor->creationScriptArgument);
    ((RuntimeObjectPrefix *)replacementActor)->state.actor->vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress = 0;
    objectRequestHierarchyDestruction(actor);
    return replacementActor;
}

/// Get the configured parts used to measure an actor's ground clearance.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Borrowed part-ID list; positive IDs end at a nonpositive value.
/// \note Original decompilation identifier: \c func_8001E5F0 (0x8001E5F0).
const s32 *actorConfigGetGroundReferencePartIds(s32 kind) {
    return *(const s32 **)(gActorKindGroundReferencePartEntries + (kind * sizeof(ActorKindConfig)));
}

/// Get the first configured part used for actor attachment positions.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns First attachment part ID.
/// \note Original decompilation identifier: \c func_8001E614 (0x8001E614).
s32 actorConfigGetFirstAttachmentPartId(s32 kind) {
    return *(s32 *)(gActorKindFirstAttachmentPartEntries + (kind * sizeof(ActorKindConfig)));
}

/// Get the second configured part used for actor attachment positions.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Second attachment part ID.
/// \note Original decompilation identifier: \c func_8001E638 (0x8001E638).
s32 actorConfigGetSecondAttachmentPartId(s32 kind) {
    return *(s32 *)(gActorKindSecondAttachmentPartEntries + (kind * sizeof(ActorKindConfig)));
}

/// Get the configured default part for an actor kind.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Default part ID.
/// \note Original decompilation identifier: \c func_8001E65C (0x8001E65C).
s32 actorConfigGetDefaultPartId(s32 kind) {
    return *(s32 *)(gActorKindDefaultPartEntries + (kind * sizeof(ActorKindConfig)));
}

/// Get an actor kind's display name for inspection text.
/// \param kind Valid actor kind whose text is requested.
/// \param useDefault Nonzero selects the default name; zero prefers the alternate name when available.
/// \returns Borrowed selected name string, or null when no name is configured.
/// \note Original decompilation identifier: \c func_8001E680 (0x8001E680).
char *actorConfigGetDisplayName(s32 kind, s32 useDefault) {
    char *alternateName;

    if ((useDefault != 0) ||
        (alternateName = *(char **)(gActorKindAlternateNameEntries + (kind * sizeof(ActorKindConfig))),
         (alternateName == NULL))) {
        return *(char **)(gActorKindDefaultNameEntries + (kind * sizeof(ActorKindConfig)));
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
        (alternateDescription = *(char **)(gActorKindAlternateDescriptionEntries + (kind * sizeof(ActorKindConfig))),
         (alternateDescription == NULL))) {
        return *(char **)(gActorKindDefaultDescriptionEntries + (kind * sizeof(ActorKindConfig)));
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
    dimensions[0] = 0.0F;
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
        scale = (((f32)sp30 / (f32)progressGetMaximum(0x18)) + 1.0F) * 0.5F;
    }
    actorCreateForKind(0x16, x, y, z, pitch, yaw, scale * gCurrentAreaConfiguration->objectScale, script,
                       scriptArgument);
}
