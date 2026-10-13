#include "opacity_animation_programs.h"
#include "texture_animation_programs.h"
#include "object_movement.h"
#include "camera_visibility.h"
#include "item_kind.h"
#include "item_types.h"
#include "actor_script.h"
#include "interaction.h"
#include "vector3_types.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "common.h"
#include "actor_part_config.h"
#include "integer_animation.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "object_types.h"
/// Model hierarchy template cloned to create an interaction marker.
/// \note Original identifier: \c D_80088C50 (0x80088C50).
extern u8 gInteractionMarkerModelTemplate[];

/// State owned by one interaction marker, including its six animation tracks.
/// Tracks select texture frame, opacity, depth offset, height offset, Z scale
/// and Y scale, in that order. Track values are available after evaluation.
typedef struct InteractionMarkerState {
    s32 mode;             ///< Selected marker presentation mode.
    s32 targetHandle;     ///< Handle of the selected interaction target.
    ValueTrack tracks[6]; ///< Retained integer animation-track records.
} InteractionMarkerState;
typedef char InteractionMarkerStateSizeCheck[(sizeof(InteractionMarkerState) == 0x50) ? 1 : -1];
typedef char InteractionMarkerTracksOffsetCheck[((u32) & ((InteractionMarkerState *)0)->tracks == 8) ? 1 : -1];

f32 fabsf(f32);
#pragma intrinsic(fabsf)
/// Primary and secondary view-space regions used to classify interaction candidates.
/// \note Original identifier: \c D_80088C88 (0x80088C88).
extern CameraViewRegion gInteractionRegions[2];
/// Classification bits associated with the two interaction regions.
/// \note Original identifier: \c D_800A4780 (0x800A4780).
extern s32 gInteractionRegionFlags[2];
/// Separate symbol view of the primary player record's held item handle.
/// \note Original identifier: \c D_800EF724 (0x800EF724).
extern s32 gPlayerHeldItemHandle;
/// Separate symbol view of the primary player record's position Y.
/// \note Original identifier: \c D_800EF6A4 (0x800EF6A4).
extern f32 gPlayerPositionY;
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;
/// Best candidate score in the current scan; lower scores take precedence.
/// \note Original identifier: \c D_800B6C50 (0x800B6C50).
extern f32 gInteractionCandidateScore;
/// Preferred object handle in the current interaction scan, or zero when absent.
/// \note Original identifier: \c D_800B6C54 (0x800B6C54).
extern s32 gInteractionCandidateHandle;
/// Region and visibility classification of the preferred interaction candidate.
/// \note Original identifier: \c D_800B6C58 (0x800B6C58).
extern s32 gInteractionCandidateFlags;

/// Classify an object for interaction and consider it as the preferred candidate.
/// \param object Object whose size, position, and identity are evaluated.
/// \returns A bitmask: 1 for the secondary interaction region, 2 for the primary
/// region, and 4 for camera visibility; zero means the object is ineligible.
/// \note Original decompilation identifier: \c func_8001E830 (0x8001E830).
s32 interactionConsiderObject(u8 *object) {
    s32 flags;
    f32 horizontal;
    f32 vertical;
    f32 projected[3];
    f32 score;
    s32 i;

    if (((RuntimeObjectPrefix *)object)->scale[0] == 0.0f || ((RuntimeObjectPrefix *)object)->scale[1] == 0.0f || ((RuntimeObjectPrefix *)object)->scale[2] == 0.0f) {
        return 0;
    }
    if (areaTestCameraVisibility(((RuntimeObjectPrefix *)object)->radius * 2, ((RuntimeObjectPrefix *)object)->worldPosition, projected)) {
        flags = 4;
    } else {
        flags = 0;
    }
    if (gPlayerHeldItemHandle == ((RuntimeObjectPrefix *)object)->handle) {
        flags |= 3;
    } else {
        if (flags == 0)
            return 0;
        for (i = 0; i < 2; i++) {
            if (cameraTestViewPoint(gInteractionRegions[i], projected)) {
                flags |= gInteractionRegionFlags[i];
            }
        }
        if (flags & 2) {
            if (((RuntimeObjectPrefix *)object)->worldPosition[1] - gPlayerPositionY > 12.0f) {
                flags = (flags & ~2) | 1;
            }
        }
    }
    if (object != gPikachuActor && (flags & 3)) {
        if (gPlayerHeldItemHandle == ((RuntimeObjectPrefix *)object)->handle) {
            score = -2147483648.0f;
        } else {
            score =
                ((horizontal = fabsf(projected[0])) * 50.0f - projected[2]) + (vertical = fabsf(projected[1])) * 40.0f;
        }
        if (score < gInteractionCandidateScore) {
            gInteractionCandidateFlags = flags;
            gInteractionCandidateHandle = ((RuntimeObjectPrefix *)object)->handle;
            gInteractionCandidateScore = score;
        }
    }
    return flags;
}

/// Create and register the interaction marker object.
/// \returns The created marker object address.
/// \note Original decompilation identifier: \c func_8001EA78 (0x8001EA78).
s32 interactionMarkerCreate(void) {
    s32 markerAddress = (s32)objectCreateHierarchyFromTemplate((ObjectTemplate *)gInteractionMarkerModelTemplate);
    objectRegisterActiveHierarchy((u8 *)markerAddress);
    return markerAddress;
}

/// Most recently evaluated interaction marker presentation mode.
/// \note Original identifier: \c D_800B6C5C (0x800B6C5C).
extern s32 gInteractionMarkerMode;
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Initialize an interaction marker's target and animation state.
/// \param object Marker object receiving its initialized state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001EAAC (0x8001EAAC).
void interactionMarkerInitialize(u8 *object) {
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x50, 0x142);
    ((InteractionMarkerState *)allocation)->mode = 0;
    gInteractionMarkerMode = 0;
    ((InteractionMarkerState *)allocation)->targetHandle = 0;
    ((RuntimeObjectPrefix *)object)->state.bytes = allocation;
}

/// Marker-mode animation programs, sound and target-relative placement factors.
typedef struct SelectionStyle {
    s32 tracks[6]; ///< Texture, opacity, depth, height, Z-scale and Y-scale program indices.
    s32 sound;     ///< Sound played when this mode becomes active.
    f32 depth;     ///< Fraction of target depth used for marker placement.
    f32 height;    ///< Fraction of target height used for marker placement.
} SelectionStyle;
typedef char SelectionStyleSizeCheck[(sizeof(SelectionStyle) == 0x24) ? 1 : -1];
/// Animation, sound and target-relative placement settings indexed by marker mode.
/// \note Original identifier: \c D_80088CB0 (0x80088CB0).
extern SelectionStyle gInteractionMarkerStyles[];
/// Integer animation programs controlling marker position offsets and scale axes.
/// \note Original identifier: \c D_8006B4EC (0x8006B4EC).
extern u8 *gInteractionMarkerTransformPrograms[];
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern u8 gPlayerState[];
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
/// Scale converting marker animation values from tenths to coordinate units.
/// \note Original identifier: \c D_800A47A8 (0x800A47A8).
extern f32 gInteractionMarkerTrackUnitScale;
/// Scale converting marker animation offsets for actor targets from tenths to coordinate units.
/// \note Original identifier: \c D_800A47AC (0x800A47AC).
extern f32 gInteractionMarkerActorTrackUnitScale;
s32 playerClassifyInteractionTarget(u8 *, u8 *, s32);
s32 audioPlaySoundEffect(s32);
/// Update the marker target, animation, and placement, then clear the current candidate scan.
/// \param object Initialized interaction marker object.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001EAF4 (0x8001EAF4).
void interactionMarkerUpdate(u8 *object) {
    f32 unit;
    u8 *species;
    s32 mode;
    union {
        Vec3f vector;
        f32 components[3];
    } offset;
    s32 event;
    u8 *state;
    u8 *target;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    target = objectResolveAvailableHandle(gInteractionCandidateHandle);
    mode = playerClassifyInteractionTarget(gPlayerState, target, gInteractionCandidateFlags);
    if (target == NULL && ((InteractionMarkerState *)state)->mode != 0) {
        target = objectResolveAvailableHandle(((InteractionMarkerState *)state)->targetHandle);
        if (target != NULL) {
            mode = 1;
            gInteractionCandidateHandle = ((InteractionMarkerState *)state)->targetHandle;
        } else
            mode = 0;
    }
    if (mode != 0 && target != NULL) {
        if (mode != ((InteractionMarkerState *)state)->mode ||
            gInteractionCandidateHandle != ((InteractionMarkerState *)state)->targetHandle) {
            animationIntegerTrackInitialize(
                &((InteractionMarkerState *)state)->tracks[0],
                (gTextureFrameAnimationPrograms[gInteractionMarkerStyles[mode].tracks[0]]));
            animationIntegerTrackInitialize(
                &((InteractionMarkerState *)state)->tracks[1],
                (gOpacityAnimationPrograms[gInteractionMarkerStyles[mode].tracks[1]]));
            animationIntegerTrackInitialize(
                &((InteractionMarkerState *)state)->tracks[2],
                (ValueKey *)(gInteractionMarkerTransformPrograms[gInteractionMarkerStyles[mode].tracks[2]]));
            animationIntegerTrackInitialize(
                &((InteractionMarkerState *)state)->tracks[3],
                (ValueKey *)(gInteractionMarkerTransformPrograms[gInteractionMarkerStyles[mode].tracks[3]]));
            animationIntegerTrackInitialize(
                &((InteractionMarkerState *)state)->tracks[4],
                (ValueKey *)(gInteractionMarkerTransformPrograms[gInteractionMarkerStyles[mode].tracks[4]]));
            animationIntegerTrackInitialize(
                &((InteractionMarkerState *)state)->tracks[5],
                (ValueKey *)(gInteractionMarkerTransformPrograms[gInteractionMarkerStyles[mode].tracks[5]]));
            audioPlaySoundEffect(gInteractionMarkerStyles[mode].sound);
            gInteractionMarkerStyles[1].tracks[0] = gInteractionMarkerStyles[mode].tracks[0];
            gInteractionMarkerStyles[1].depth = gInteractionMarkerStyles[mode].depth;
            gInteractionMarkerStyles[1].height = gInteractionMarkerStyles[mode].height;
        }
        animationIntegerTrackEvaluate(&((InteractionMarkerState *)state)->tracks[0], &event, gRuntimeFrameSeconds);
        animationIntegerTrackEvaluate(&((InteractionMarkerState *)state)->tracks[1], &event, gRuntimeFrameSeconds);
        animationIntegerTrackEvaluate(&((InteractionMarkerState *)state)->tracks[2], &event, gRuntimeFrameSeconds);
        animationIntegerTrackEvaluate(&((InteractionMarkerState *)state)->tracks[3], &event, gRuntimeFrameSeconds);
        animationIntegerTrackEvaluate(&((InteractionMarkerState *)state)->tracks[4], &event, gRuntimeFrameSeconds);
        animationIntegerTrackEvaluate(&((InteractionMarkerState *)state)->tracks[5], &event, gRuntimeFrameSeconds);
        ((RuntimeObjectPrefix *)object)->position[0] = ((RuntimeObjectPrefix *)target)->worldPosition[0];
        ((RuntimeObjectPrefix *)object)->position[1] = ((RuntimeObjectPrefix *)target)->worldPosition[1];
        ((RuntimeObjectPrefix *)object)->position[2] = ((RuntimeObjectPrefix *)target)->worldPosition[2];
        ((RuntimeObjectPrefix *)object)->worldPosition[0] = ((RuntimeObjectPrefix *)object)->position[0];
        ((RuntimeObjectPrefix *)object)->worldPosition[1] = ((RuntimeObjectPrefix *)object)->position[1];
        ((RuntimeObjectPrefix *)object)->worldPosition[2] = ((RuntimeObjectPrefix *)object)->position[2];
        objectOrientAlongCameraView(object);
        unit = gInteractionMarkerTrackUnitScale;
        ((RuntimeObjectPrefix *)object)->scale[1] =
            ((f32) * (s16 *)(state + 0x4C) * unit) * ((RuntimeObjectPrefix *)object)->scale[0];
        ((RuntimeObjectPrefix *)object)->scale[2] =
            ((f32) * (s16 *)(state + 0x40) * unit) * ((RuntimeObjectPrefix *)object)->scale[0];
        if (((RuntimeObjectPrefix *)target)->kind == 0x86) {
            species = ((RuntimeObjectPrefix *)target)->state.bytes;
            offset.vector.x = 0.0f;
            offset.vector.y = gItemKinds[((ItemStatePrefix *)species)->kind].radiusZ * ((RuntimeObjectPrefix *)target)->scale[1];
            offset.vector.z = gItemKinds[((ItemStatePrefix *)species)->kind].radiusX * ((RuntimeObjectPrefix *)target)->scale[2];
        } else if (((RuntimeObjectPrefix *)target)->kind > 0 && ((RuntimeObjectPrefix *)target)->kind < 0x26) {
            actorConfigGetInteractionMarkerDimensions(((RuntimeObjectPrefix *)target)->kind, offset.components);
            unit = gInteractionMarkerActorTrackUnitScale;
            offset.vector.y *= ((RuntimeObjectPrefix *)target)->scale[1];
            offset.vector.z *= ((RuntimeObjectPrefix *)target)->scale[2];
            offset.vector.x = 0.0f;
        } else {
            offset.vector.x = 0.0f;
            offset.vector.y = ((RuntimeObjectPrefix *)target)->radius;
            offset.vector.z = ((RuntimeObjectPrefix *)target)->radius;
        }
        offset.vector.y =
            (f32) * (s16 *)(state + 0x34) * unit + gInteractionMarkerStyles[mode].height * offset.vector.y;
        offset.vector.z = (f32) - ((InteractionMarkerState *)state)->tracks[2].value * unit -
                          gInteractionMarkerStyles[mode].depth * offset.vector.z;
        objectAddPendingLocalMovement(offset.components);
    }
    ((InteractionMarkerState *)state)->targetHandle = gInteractionCandidateHandle;
    ((InteractionMarkerState *)state)->mode = mode;
    gInteractionMarkerMode = mode;
    gInteractionCandidateFlags = 0;
    gInteractionCandidateHandle = 0;
    gInteractionCandidateScore = 2147483648.0f;
}

/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
void textureLoadForDraw(Gfx **, s32, s32, u8 *);

/// Prepare display settings for an active interaction marker.
/// \param object Interaction marker object to draw.
/// \returns The marker display-list address, or zero when the marker is inactive.
/// \note Original decompilation identifier: \c func_8001EF58 (0x8001EF58).
s32 interactionMarkerPrepareDraw(u8 *object) {
    u8 *d = ((RuntimeObjectPrefix *)object)->state.bytes;

    if (((InteractionMarkerState *)d)->mode == 0) {
        return 0;
    }
    textureLoadForDraw(&gRenderDisplayListCursor, 5, ((InteractionMarkerState *)d)->tracks[0].value, d);
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 0xFF, 0xFF, 0xFF, ((InteractionMarkerState *)d)->tracks[1].value);
    if (((InteractionMarkerState *)d)->targetHandle == ((ActorScriptState *)*(u8 **)(gPikachuActor + 0x54))->heldItemHandle) {
        gDPSetRenderMode(gRenderDisplayListCursor++, 0x504240, 0);
    } else {
        gDPSetRenderMode(gRenderDisplayListCursor++, 0x504A50, 0);
    }
    return *(s32 *)(object + 0x30);
}

/// Get the preferred object from the current interaction scan.
/// \returns The current candidate object identifier, or zero when none has been selected.
/// \note Original decompilation identifier: \c func_8001F060 (0x8001F060).
s32 interactionGetCandidateId(void) {
    return gInteractionCandidateHandle;
}


/// Get the active interaction marker mode.
/// \returns The marker mode, or zero when inactive.
/// \note Original decompilation identifier: \c func_8001F06C (0x8001F06C).
s32 interactionGetMarkerMode(void) {
    return gInteractionMarkerMode;
}

/// Scale converting the primary region scale argument from tenths.
/// \note Original identifier: \c D_800A47B0 (0x800A47B0).
extern f32 gInteractionPrimaryScaleUnit;

/// Set the primary interaction region's horizontal and vertical scale.
/// \param scale Horizontal and vertical scale in tenths.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F078 (0x8001F078).
void interactionSetPrimaryRegionScale(s32 scale) {
    f32 regionScale;

    regionScale = (f32)scale * gInteractionPrimaryScaleUnit;
    gInteractionRegions[0].scaleX = regionScale;
    gInteractionRegions[0].scaleY = regionScale;
}

/// Separate symbol view of the primary interaction region margin.
/// \note Original identifier: \c D_80088C98 (0x80088C98).
extern f32 gInteractionPrimaryMargin;
/// Scale converting the primary region margin argument from tenths.
/// \note Original identifier: \c D_800A47B4 (0x800A47B4).
extern f32 gInteractionPrimaryMarginUnit;

/// Set the primary interaction region's margin.
/// \param margin Region margin in tenths of a coordinate unit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F0A0 (0x8001F0A0).
void interactionSetPrimaryRegionMargin(s32 margin) {
    gInteractionPrimaryMargin = (f32)margin * gInteractionPrimaryMarginUnit;
}

/// Scale converting the secondary region scale argument from tenths.
/// \note Original identifier: \c D_800A47B8 (0x800A47B8).
extern f32 gInteractionSecondaryScaleUnit;

/// Set the secondary interaction region's horizontal and vertical scale.
/// \param scale Horizontal and vertical scale in tenths.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F0C0 (0x8001F0C0).
void interactionSetSecondaryRegionScale(s32 scale) {
    f32 regionScale;

    regionScale = (f32)scale * gInteractionSecondaryScaleUnit;
    gInteractionRegions[1].scaleX = regionScale;
    gInteractionRegions[1].scaleY = regionScale;
}

/// Separate symbol view of the secondary interaction region margin.
/// \note Original identifier: \c D_80088CAC (0x80088CAC).
extern f32 gInteractionSecondaryMargin;
/// Scale converting the secondary region margin argument from tenths.
/// \note Original identifier: \c D_800A47BC (0x800A47BC).
extern f32 gInteractionSecondaryMarginUnit;

/// Set the secondary interaction region's margin.
/// \param margin Region margin in tenths of a coordinate unit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F0E8 (0x8001F0E8).
void interactionSetSecondaryRegionMargin(s32 margin) {
    gInteractionSecondaryMargin = (f32)margin * gInteractionSecondaryMarginUnit;
}
