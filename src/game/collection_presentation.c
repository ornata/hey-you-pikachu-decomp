#include "backdrop_draw.h"
#include "common.h"
#include "text_image_create.h"

#include "string_bytes.h"
#include "actor_parts.h"
#include "audio_cue.h"
#include "player_control.h"
#include "text_draw.h"
#include "toolbox_types.h"
#include "item_kind.h"
#include "object_lookup.h"
#include "scene_types.h"
#include "heap_alloc.h"
#include "text_image.h"

s32 progressSetValue(s32, s32);
void presentationDrawAreaLabel(Gfx **);

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Friendship messages indexed by the cleared chapter.
/// \note Original identifier: \c D_800999F0 (0x800999F0).
char *gChapterFriendshipMessages[20] = {
    "You talked to ^6PIKACHU^^!\n\n^6PIKACHU^^ wants to do more\nfun things together.\n",
    "You and ^6PIKACHU^^ are\nstarting to become friends!\n^6PIKACHU^^ is friends with\na lot of Pok\xA1\xADmon.\n",
    "You and ^6PIKACHU^^ are friends!\n\n^6PIKACHU^^ really enjoys\nfield trips.",
    "You and ^6PIKACHU^^ have\nsomething in common!\n^6PIKACHU^^ loves having\npicnics.",
    "You had great fun fishing\nwith ^6PIKACHU^^!\n\n^6PIKACHU^^ knows lots of ways\nto have fun.",
    "You and ^6PIKACHU^^ caught\na really big one!\n\n^6PIKACHU^^ loves to fish.\n",
    "You and ^6PIKACHU^^ caught\na whole lot of fish!\n\n^6PIKACHU^^ caught a big one,\nso it's in a good mood.\n",
    "You're getting along even\nbetter with ^6PIKACHU^^!\n\n^6PIKACHU^^ enjoys sprinkling\nwater on ^4ODDISH^^.\n",
    "You went on a picnic\nwith ^6PIKACHU^^!\n\n^6PIKACHU^^ filled its tummy\nwith a very tasty meal!\n",
    "You and ^6PIKACHU^^ helped\nthe lost ^4POLIWAG^^\ntogether!\n^6PIKACHU^^ is happy because\n^4SQUIRTLE^^ thanked "
    "it.\n",
    "You and ^6PIKACHU^^ traveled\nvery far together!\n\n^6PIKACHU^^ can't wait to\nopen more ^4treasure chest^^s.\n",
    "You and ^6PIKACHU^^ had fun at\nthe pi\xA1\xB1"
    "ata party!\n\n^6PIKACHU^^ feels sick from\neating too many treats.\n",
    "^6PIKACHU^^ understands your\nfeelings!\n^6PIKACHU^^ wants to talk\nwith you some more.\n",
    "You and ^6PIKACHU^^ worked\nhard together!\n^6PIKACHU^^ has become quite\ngood at caring for\n^4CATERPIE^^.\n",
    "You and ^6PIKACHU^^ have\nbecome good friends!\n^6PIKACHU^^ is very happy that\nit found the "
    "legendary\n^4ODDISH^^.\n",
    "Your friendship with\n^6PIKACHU^^ has grown!\n^6PIKACHU^^ is relieved that it\ncould help all of the "
    "lost\n^4POLIWAG^^.\n",
    "You had a great adventure\nwith ^6PIKACHU^^!\n^6PIKACHU^^'s ears are still\nringing from the cannon's\nblast.\n",
    "You and ^6PIKACHU^^ are pals!\n\n^6PIKACHU^^ is very pleased\nthat it had the chance to\neat something special.\n",
    "You strengthened your\nfriendship with ^6PIKACHU^^!\n\nIt looks like ^6PIKACHU^^'s mood\nhas improved.\n",
    "You gazed at the stars\nwith ^6PIKACHU^^!\n\n^6PIKACHU^^ had no idea there\nwere so many stars in the sky.\n",
};

typedef struct Vec3_295BC {
    f32 x, y, z;
} Vec3_295BC;

/* One sprite of a textureDrawRectangles draw list; texture < 0 ends the list and a
 * negative width/height means the texture's own size. */
typedef struct TextureDraw {
    s32 texture;
    s32 x, y;
    s32 width, height;
    s32 s, t;
    f32 scaleX, scaleY;
} TextureDraw;

#define TEXTURE_DRAW_END {-1, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f}

/* Draw-list template: up to three sprites plus the terminator. */
typedef struct Tmpl90 {
    TextureDraw draws[4];
} Tmpl90;

typedef struct {
    s32 ids[3];
} Choice28B84;

/// Camera-relative position of the item shown in collection results.
/// \note Original identifier: \c D_80099A40 (0x80099A40).
Vec3_295BC gCollectionItemPresentationPosition = {2.5f, -0.5f, -10.0f};
/// Camera-relative actor position for the card presentation.
/// \note Original identifier: \c D_80099A4C (0x80099A4C).
Vec3_295BC gCardPresentationActorPosition = {-2.6f, -4.0f, -11.0f};
/// Camera-relative actor position for collection results.
/// \note Original identifier: \c D_80099A58 (0x80099A58).
Vec3_295BC gCollectionPresentationActorPosition = {-4.5f, -4.0f, -12.7f};
/// Current stage of the collection results presentation.
/// \note Original identifier: \c D_80099A64 (0x80099A64).
u32 gCollectionPresentationStage = 0;
/// Current stage of the card presentation.
/// \note Original identifier: \c D_80099A68 (0x80099A68).
s32 gCardPresentationStage = 0;
/// Presentation-space focus targets used by the actor during cards and collection results.
/// \note Original identifier: \c D_80099A6C (0x80099A6C).
Vec3_295BC gPresentationActorFocusTargets[3] = {
    {0.0f, 0.0f, 0.0f},
    {-3.0f, 1.0f, -5.0f},
    {3.0f, 1.0f, -5.0f},
};
/// Texture rectangles for card presentation decorations.
/// \note Original identifier: \c D_80099A90 (0x80099A90).
Tmpl90 gCardPresentationDecorationTemplate = {{
    {0x23, 0x14, 0xB8, -1, -1, 0, 0, 1.0f, 1.0f},
    {0x24, 0x44, 0xB8, -1, -1, 0, 0, 1.0f, 1.0f},
    {0x25, 0x65, 0xCC, -1, -1, 0, 0, 1.0f, 1.0f},
    TEXTURE_DRAW_END,
}};
/// Card panel rectangles whose textures come from the selected area card.
/// \note Original identifier: \c D_80099B20 (0x80099B20).
Tmpl90 gCardPresentationPanelTemplate = {{
    {0, 0x30, 0x3C, 0x80, 0x20, 0, 0, 1.0f, 1.0f},
    {0, 0xB0, 0x3C, 0x60, 0x20, 0, 0, 1.0f, 1.0f},
    {0, 0xD4, 0x5C, 0x40, 0x20, 0, 0, 1.0f, 1.0f},
    TEXTURE_DRAW_END,
}};
/// Third card panel texture indexed by chase stage for area 11.
/// \note Original identifier: \c D_80099BB0 (0x80099BB0).
Choice28B84 gArea11CardChaseStageTextures = {{0xE, 0xF, 0x10}};
/// Third card panel texture indexed by chase stage for area 5.
/// \note Original identifier: \c D_80099BBC (0x80099BBC).
Choice28B84 gArea5CardChaseStageTextures = {{0xE, -1, 0xF}};
/// Third card panel texture indexed by chase stage for area 13.
/// \note Original identifier: \c D_80099BC8 (0x80099BC8).
Choice28B84 gArea13CardChaseStageTextures = {{-1, 0xE, 0xF}};
/// Texture rectangles for collection result decorations.
/// \note Original identifier: \c D_80099BD4 (0x80099BD4).
TextureDraw gCollectionPresentationDecorations[7] = {
    {0x29, 0x1C, 0x10, 0x40, 0x40, 0, 0, 1.0f, 1.0f},
    {0x2A, 0x5C, 0x10, 0x80, 0x40, 0, 0, 1.0f, 1.0f},
    {0x2B, 0xDC, 0x10, 0x40, 0x40, 0, 0, 1.0f, 1.0f},
    {0x26, 0x8C, 0xBF, 0x20, 0x20, 0, 0, 1.0f, 1.0f},
    {0x27, 0xAC, 0xBF, 0x40, 0x20, 0, 0, 1.0f, 1.0f},
    {0x28, 0xEC, 0xBF, 0x20, 0x20, 0, 0, 1.0f, 1.0f},
    TEXTURE_DRAW_END,
};
/// Actor response actions for presented items in response category two.
/// \note Original identifier: \c D_80099CD0 (0x80099CD0).
s32 gPresentationItemCategory2ResponseActions[4] = {0xA3, 0xE5, 0xE6, 0x3D};
/// Actor response actions for presented items in response category one.
/// \note Original identifier: \c D_80099CE0 (0x80099CE0).
s32 gPresentationItemCategory1ResponseActions[4] = {0x6E, 0x3C, 0xA8, 0xAF};
/// Actor response actions for presented items in response category zero.
/// \note Original identifier: \c D_80099CF0 (0x80099CF0).
s32 gPresentationItemCategory0ResponseActions[4] = {0xD2, 0x40, 0x7A, 0x31};
/// Previous actor response variant, used to avoid immediate repetition.
/// \note Original identifier: \c D_80099D00 (0x80099D00).
s32 gCollectionPreviousResponseIndex = 0;

typedef struct {
    f32 f0;
    f32 f4;
    s32 f8;
    s32 fC;
    f32 f10;
    u8 pad14[0x444];
    s32 f458;
    volatile s32 f45C;
    u8 pad460[0x48];
} Unk4A8;
/// Shared queued-message, timing, scrolling and text-image state for the message bar.
/// \note Original identifier: \c D_800F06C0 (0x800F06C0).
extern Unk4A8 gMessageBarState;
void cameraUpdateOverlayColors(s32, s32);
void cameraDrawSecondaryOverlay(s32);
void objectDrawHierarchyGroup(s32, s32);
void presentationDrawCardDecorations(Gfx **);
void presentationDrawCurrentCardPanels(Gfx **);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
void actionScriptStart(s32, s32);  /* extern */
void textWindowAppendText(char *); /* extern */
s32 textWindowOpen(s32, s32, s32); /* extern */
s32 textWindowShowFirstPage();     /* extern */
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
/// Separate symbol view of the primary player friendship-title level.
/// \note Original identifier: \c D_800EF796 (0x800EF796).
extern u8 gPlayerFriendshipLevel;
/// Separate symbol view of the last awarded primary player friendship milestone, or minus one when none awaits presentation.
/// \note Original identifier: \c D_800EF798 (0x800EF798).
extern s32 gPlayerFriendshipMilestone;

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Separate view of the message-bar image slot prepared for the next transition.
/// \note Original identifier: \c D_800F0B1C (0x800F0B1C).
extern s32 gMessageBarWorkImageIndex;


/// Interior availability progress-flag view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AB2 (0x80125AB2).
extern u8 gItemKindAvailabilityFlagEntries[];

s32 itemGetDisplayName(s32, s32);

/// Record the configured progress flag for a live item in a kept-item slot.
/// \param slot Readable kept-item slot; any live handle must identify an initialized item with a valid catalog kind.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028240 (0x80028240).
void inventoryRecordKeptItemProgress(u8 *slot) {
    s16 availabilityFlag;
    u8 *liveObject;
    u8 *p;

    liveObject = objectFindLiveByHandle(*(s32 *)(slot + 4));
    if (liveObject != NULL) {
        p = *(u8 **)(liveObject + 0x54);
        TRACE("keeping item found, %s\n", itemGetDisplayName(*(s16 *)(p + 4), 1));
        availabilityFlag = *(s16 *)(gItemKindAvailabilityFlagEntries + (*(s16 *)(p + 4) * 0x74));
        if (availabilityFlag != -1) {
            progressSetValue(availabilityFlag, 1);
        }
    }
}

/// Separate view of the two ten-entry toolbox item rows.
/// \note Original identifier: \c D_800F1C08 (0x800F1C08).
extern u8 gToolboxItemSlots[];
/// Separate view of the Item Storage row beginning at its first kind word.
/// \note Original identifier: \c D_800F1C58 (0x800F1C58).
extern u8 gToolboxStorageRow[];
/// Separate view of the retained bonus item slot.
/// \note Original identifier: \c D_800F1CD0 (0x800F1CD0).
extern u8 gToolboxBonusSlot[];
/// Separate view of the retained purchased item slot.
/// \note Original identifier: \c D_800F1CD8 (0x800F1CD8).
extern u8 gToolboxPurchaseSlot[];

/// Record progress for kept items and clear the temporary item slot.
/// \returns One if any retained item slot contains a nonzero handle, or zero otherwise; the cleared temporary slot does not affect this result.
/// \note Portability: Integer-address casts preserve the original alias handling and require 32-bit pointers.
/// \note Original decompilation identifier: \c func_800282A4 (0x800282A4).
s32 inventoryRecordKeptItemsAndClearTemporarySlot(void) {
    u8 *slot;
    s32 result;
    u8 *end;

    result = 0;
    end = gToolboxStorageRow, slot = gToolboxItemSlots;
    do {
        inventoryRecordKeptItemProgress(slot);
        if (*(s32 *)(slot + 4) != 0) {
            result = 1;
        }
        slot += 8;
    } while (slot != end);
    inventoryRecordKeptItemProgress(gToolboxBonusSlot);
    if (*(s32 *)(gToolboxBonusSlot + 4) != 0) {
        result = 1;
    }
    inventoryRecordKeptItemProgress(gToolboxPurchaseSlot);
    *(s32 *)((u32)gToolboxPurchaseSlot + 4) = 0;
    *(s32 *)(u32)gToolboxPurchaseSlot = 0;
    return result;
}

typedef struct Blk5C {
    s32 w[21];
    struct Blk408 *ext;
    s32 w58;
} Blk5C;
typedef struct Blk408 {
    s32 w[0x102];
} Blk408;
/// Saved actor record retained while presentation placement and pose are active.
/// \note Original identifier: \c D_800F0C00 (0x800F0C00).
extern Blk5C gPresentationSavedActor;
/// Saved actor behavior state used to restore the actor after a presentation.
/// \note Original identifier: \c D_800F0C60 (0x800F0C60).
extern Blk408 gPresentationSavedActorState;
typedef struct {
    f32 m[12];
} Blk30;
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern s32 gPlayerState;
/// Separate symbol view of the primary player record's position Y.
/// \note Original identifier: \c D_800EF6A4 (0x800EF6A4).
extern f32 gPlayerPositionY;
/// Separate symbol view of the primary player record's position Z.
/// \note Original identifier: \c D_800EF6A8 (0x800EF6A8).
extern s32 gPlayerPositionZ;
/// Separate symbol view of the primary player record's view yaw.
/// \note Original identifier: \c D_800EF6AC (0x800EF6AC).
extern f32 gPlayerYaw;
/// Separate symbol view of the primary player record's additional view pitch.
/// \note Original identifier: \c D_800EF6B0 (0x800EF6B0).
extern f32 gPlayerPitchOffset;
/// Separate symbol view of the primary player record's view pitch.
/// \note Original identifier: \c D_800EF6B4 (0x800EF6B4).
extern f32 gPlayerPitch;
/// Separate symbol view of the primary player record's horizontal view shake.
/// \note Original identifier: \c D_800EF6B8 (0x800EF6B8).
extern f32 gPlayerViewShake;
/// Separate symbol view of the primary player record's eye height.
/// \note Original identifier: \c D_800EF6BC (0x800EF6BC).
extern f32 gPlayerEyeHeight;
void playerRestoreState(void);
u8 *actionScriptStop();
void actionBeginPoseTransition(s32, s32, f32);
void objectClearHierarchyFlags(u8 *, u16);
void cameraSetPositionAndAngles(s32, s32, f32, s32, f32, f32, f32);

#define ACT_F(i) (((f32 *)gPikachuActor)[i])
#define EXT_F(i) (((f32 *)&gPresentationSavedActorState)[i])

/// Restore the active actor's saved transform and behavior settings, then resume its scene pose and camera.
/// \returns Nothing; requires a saved snapshot of the same live, initialized actor.
/// \note Portability: The unprototyped action-script stop call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_80028344 (0x80028344).
void presentationRestoreActiveActor(void) {
    f32 *save;
    u8 *ext = (u8 *)((Blk5C *)gPikachuActor)->ext;

    playerRestoreState();
    save = (f32 *)&gPresentationSavedActor;
    ACT_F(0x11) = save[0x11];
    ACT_F(0x12) = save[0x12];
    ACT_F(0x13) = save[0x13];
    ACT_F(6) = save[6];
    ACT_F(7) = save[7];
    ACT_F(8) = save[8];
    ACT_F(3) = save[3];
    ACT_F(4) = save[4];
    ACT_F(5) = save[5];
    ACT_F(0) = save[0];
    ACT_F(1) = save[1];
    ACT_F(2) = save[2];
    ACT_F(9) = save[9];
    ((f32 *)ext)[3] = EXT_F(3);
    ((f32 *)ext)[4] = EXT_F(4);
    ((f32 *)ext)[5] = EXT_F(5);
    *(Blk30 *)(ext + 0x18) = *(Blk30 *)&EXT_F(6);
    ((f32 *)ext)[0x12] = EXT_F(0x12);
    *(u16 *)(ext + 0x1E8) = *(u16 *)((u8 *)&gPresentationSavedActorState + 0x1E8);
    actionScriptStop(gPikachuActor, &gPresentationSavedActorState);
    actionBeginPoseTransition(gPikachuActor, 0x4A, 0.1f);
    objectClearHierarchyFlags((u8 *)gPikachuActor, 0x2000);
    cameraSetPositionAndAngles(0, gPlayerState, gPlayerPositionY + gPlayerEyeHeight, gPlayerPositionZ,
                               gPlayerPitchOffset + gPlayerPitch, gPlayerYaw + gPlayerViewShake, 0.0f);
}

#undef ACT_F
#undef EXT_F

void playerSaveState(void);

/// Save the active actor's state before a presentation changes its pose and placement.
/// \returns Nothing; requires a live actor with initialized model and behavior state.
/// \note Original decompilation identifier: \c func_80028518 (0x80028518).
void presentationSaveActiveActor(void) {
    playerSaveState();
    gPresentationSavedActor = *(Blk5C *)gPikachuActor;
    gPresentationSavedActorState = *((Blk5C *)gPikachuActor)->ext;
}

/// Transform from camera-relative presentation coordinates to world coordinates.
/// \note Original identifier: \c D_800F0BA8 (0x800F0BA8).
extern u8 gPresentationToWorldTransform[];
void matrix34TransformPointVector(u8 *, Vec3_295BC *, u8 *);
void objectUpdateHierarchy(u8 *);
void presentationUpdateFocusPoint(Vec3_295BC *);
/// Place the active actor at a presentation-space point while retaining its ground-clearance offset.
/// \param point Three-component position in the current presentation coordinate system.
/// \returns Nothing; requires an initialized active actor, its live ground-reference object, and a valid presentation transform.
/// \note Original decompilation identifier: \c func_800285B8 (0x800285B8).
void presentationPlaceActiveActor(void *point) {
    Vec3_295BC world;
    u8 *player;
    f32 heightDelta;
    f32 *src;
    Vec3_295BC input;
    u8 *output;
    f32 worldHeight;
    f32 newHeight;
    u8 *matrix = gPresentationToWorldTransform;
    u8 *state = *(u8 **)((u8 *)gPikachuActor + 0x54);
    presentationUpdateFocusPoint(&input);
    matrix34TransformPointVector(matrix, &input, state + 0x1EC);
    src = point;
    input.x = src[0];
    input.y = src[1];
    input.z = src[2];
    output = (u8 *)&world;
    matrix34TransformPointVector(matrix, &input, output);
    player = (u8 *)gPikachuActor;
    worldHeight = world.y;
    heightDelta = *(f32 *)(player + 0x48) - *(f32 *)(**(u8 ***)(player + 0x54) + 0x48);
    newHeight = worldHeight + heightDelta;
    world.y = newHeight;
    *(f32 *)(player + 0x18) = world.x;
    *(f32 *)((u8 *)gPikachuActor + 0x1C) = world.y;
    *(f32 *)((u8 *)gPikachuActor + 0x20) = world.z;
    *(f32 *)((u8 *)gPikachuActor + 0x44) = world.x;
    *(f32 *)((u8 *)gPikachuActor + 0x48) = world.y;
    *(f32 *)((u8 *)gPikachuActor + 0x4C) = world.z;
    objectUpdateHierarchy((u8 *)gPikachuActor);
}

/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern f32 gRenderCameras[][56];
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
/// World-space midpoint height of the two actor joints sampled during presentation preparation.
/// \note Original identifier: \c D_800F106C (0x800F106C).
extern f32 gPresentationActorJointMidpointHeight;
f32 vector3GetYawDegrees(f32 *);
void actionStopPlayback(u8 *);
void actionScriptStartWithMode(u8 *, s32, s32);
void actorAddRotationAxisDegrees(u8 *, f32, s32);
void actorSetBehaviorInhibitFlags(u8 *, u16);
void presentationPlaceActiveActor(void *);
void presentationResetFocusPoint(u8 *);
/// Place the active actor for a camera-facing presentation and begin its presentation pose.
/// \param point Three-component position in the current presentation coordinate system.
/// \returns Nothing; requires an initialized active actor with both presentation camera joints and a valid presentation transform.
/// \note Portability: The integer literal in the camera-height division preserves the original IDO instruction selection.
/// \note Original decompilation identifier: \c func_800286B0 (0x800286B0).
void presentationPrepareActiveActor(void *point) {
    s32 firstJointId = 0x47;
    s32 secondJointId = 0x4C;
    u8 *secondJoint;
    f32 d[3];
    u8 *firstJoint;
    u8 *tgt;
    f32 yaw;
    f32 h;
    f32 secondHeight;
    u8 *ext = *(u8 **)(gPikachuActor + 0x54);

    tgt = (u8 *)gRenderCameras[gRenderCameraIndex];
    objectSetHierarchyFlags(gPikachuActor, 0x2000);
    presentationPlaceActiveActor(point);
    actionScriptStop((u8 *)gPikachuActor);
    actionStopPlayback((u8 *)gPikachuActor);
    d[0] = *(f32 *)(tgt + 0) - *(f32 *)(gPikachuActor + 0x44);
    d[1] = *(f32 *)(tgt + 4) - *(f32 *)(gPikachuActor + 0x48);
    d[2] = *(f32 *)(tgt + 8) - *(f32 *)(gPikachuActor + 0x4C);
    yaw = vector3GetYawDegrees(d);
    actorAddRotationAxisDegrees((u8 *)gPikachuActor, -*(f32 *)(ext + 0xC), 0);
    actorAddRotationAxisDegrees((u8 *)gPikachuActor, yaw - *(f32 *)(ext + 0x10), 1);
    *(s32 *)(ext + 0x19C) = 0;
    matrix34TransformPointVector(gPresentationToWorldTransform, &gPresentationActorFocusTargets[1], ext + 0x1EC);
    presentationResetFocusPoint((u8 *)&gPresentationActorFocusTargets[1]);
    actionScriptStartWithMode((u8 *)gPikachuActor, 0x57, 2);
    firstJoint = actorFindPart((u8 *)gPikachuActor, firstJointId);
    h = *(f32 *)(firstJoint + 0x48);
    secondJoint = actorFindPart((u8 *)gPikachuActor, secondJointId);
    secondHeight = *(f32 *)(secondJoint + 0x48);
    gPresentationActorJointMidpointHeight = (h + secondHeight) / 2;
    actorSetBehaviorInhibitFlags((u8 *)gPikachuActor, 0xC);
}

/// Owned text image for the current area region name, or null if unavailable.
/// \note Original identifier: \c D_800F0BE0 (0x800F0BE0).
extern TextImage *gPresentationAreaLabelImage;

/// Draws the current area label when one is available.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028830 (0x80028830).
void presentationDrawAreaLabel(Gfx **gdl) {
    if (gPresentationAreaLabelImage != NULL) {
        textDrawBegin(*(s32 *)gdl);
        textDrawSetFont(0);
        textDrawReset();
        textDrawSetPalette(gDefaultTextPalette);
        textDrawSetArea(0x6D, 0x19, 0xD3, 0x29);
        textDrawSetCursor((s32)(0x66 - gPresentationAreaLabelImage->width) / 2,
                          (s32)(0x10 - gPresentationAreaLabelImage->height) / 2);
        textDrawSetColor(0, 0, 0, 0xFE);
        textDrawImageAtCursor((TextImage *)gPresentationAreaLabelImage);
        textDrawGetDisplayListCursor(gdl);
        textDrawReset();
        textDrawSetPalette(gDefaultTextPalette);
    }
}

void textureDrawRectangles(s32, u8 *, Gfx **);

/// Draws the decorative elements of the card presentation.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028918 (0x80028918).
void presentationDrawCardDecorations(Gfx **gdl) {
    Tmpl90 t = gCardPresentationDecorationTemplate;

    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    textureDrawRectangles(6, (u8 *)&t, gdl);
}

/// Current area scenario index shared by actor scripts, scene population, cards and collection titles.
/// \note Original identifier: \c D_800F0B70 (0x800F0B70).
extern s32 gAreaScenarioIndex;

/// Draw the current card panels when they are available; requires an initialized area card table and a valid selected card index.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800289D4 (0x800289D4).
void presentationDrawCurrentCardPanels(Gfx **gdl) {
    s32 *ids;
    s32 i;
    Tmpl90 t;

    t = gCardPresentationPanelTemplate;
    ids = (*(s32 ***)(gCurrentAreaConfiguration + 0x74))[gAreaScenarioIndex];
    if (ids != NULL) {
        gDPPipeSync((*gdl)++);
        gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode((*gdl)++, G_CC_DECALRGBA, G_CC_DECALRGBA);
        for (i = 0; i < 3; i++) {
            t.draws[i].texture = ids[i];
        }
        textureDrawRectangles(6, (u8 *)&t, gdl);
    }
}

/// Draw the active actor and current card presentation when card data is available; requires initialized area and actor state.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028AD4 (0x80028AD4).
void presentationDrawCardScreen(Gfx **gdl) {
    if (*(s32 *)(gCurrentAreaConfiguration + 0x74) != 0) {
        backdropDraw(gdl);
        gDPPipeSync((*gdl)++);
        gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
        objectDrawHierarchyGroup(gPikachuActor, 0x2000);
        presentationDrawAreaLabel(gdl);
        presentationDrawCurrentCardPanels(gdl);
        presentationDrawCardDecorations(gdl);
        cameraUpdateOverlayColors(0, 0);
        cameraDrawSecondaryOverlay(0);
    }
}

void cameraBuildInterfaceTransform(void *);
void cameraResetOverlayColor(s32, s32, s32, s32, s32);
void cameraFadePrimarySecondaryOverlay(s32, s32, s32, s32, f32);
void controllerRequestRumbleReset(s32);
u64 runtimeGetElapsedMilliseconds(void);
void runtimeRequestGameModeChange(s32);
s32 actionScriptIsStopped(s32);
void hudSetAreaLabelVisible(s32);
void presentationPrepareActiveActor(void *);
void presentationSetFocusTarget(void *);
void backdropSelectForView(s32, s32, s32);
void playerResetViewOrientation(void);
u8 playerGetChaseStage(void);
/// Primary controller new and auto-repeated button presses; aliases the repeated word of the processed sample.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
extern s32 gControllerPrimaryRepeatedButtons;
/// Whether the current entrance suppresses the area card presentation.
/// \note Original identifier: \c D_800F0BA0 (0x800F0BA0).
extern s32 gCardPresentationSuppressed;
/// Audio cue selected for the current area card presentation.
/// \note Original identifier: \c D_800F0BA4 (0x800F0BA4).
extern s32 gCardPresentationAudioCue;
/// Actor action script selected for the current area card presentation.
/// \note Original identifier: \c D_800F1068 (0x800F1068).
extern s32 gCardPresentationActionScript;
/// Remaining duration of the primary camera secondary overlay transition.
/// \note Original identifier: \c D_800ABDCC (0x800ABDCC).
extern f32 gPrimarySecondaryOverlayRemainingSeconds;
/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern u32 gRuntimeCurrentArea;
/// Elapsed-millisecond deadline after which the card presentation can finish.
/// \note Original identifier: \c D_800F1070 (0x800F1070).
extern u32 gCardPresentationDismissalDeadline;
/// Advance the current card presentation and return to gameplay when it finishes.
/// Requires initialized area and actor state and a chase stage from zero through two.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028B84 (0x80028B84).
void presentationUpdateCardMode(void) {
    s32 **cards;
    u32 now;
    Choice28B84 scene11Images;
    Choice28B84 scene5Images;
    Choice28B84 scene13Images;
    s32 selectedImage;
    s32 *selectedImages;

    cards = (*(s32 ***)(gCurrentAreaConfiguration + 0x74));
    if ((cards == 0) || (cards[gAreaScenarioIndex] == 0) || (gCardPresentationSuppressed != 0)) {
        hudSetAreaLabelVisible(1);
        runtimeRequestGameModeChange(0);
        return;
    }
    cameraBuildInterfaceTransform(&gPresentationToWorldTransform);
    if (actionScriptIsStopped(gPikachuActor) != 0) {
        actionScriptStart(gPikachuActor, 0x57);
    }
    switch (gCardPresentationStage) {
    case 0:
        controllerRequestRumbleReset(0);
        if ((*(s32 ***)(gCurrentAreaConfiguration + 0x74))[gAreaScenarioIndex] != 0) {
            if (gRuntimeCurrentArea == 11 || gRuntimeCurrentArea == 5 || gRuntimeCurrentArea == 13) {
                scene11Images = gArea11CardChaseStageTextures;
                scene5Images = gArea5CardChaseStageTextures;
                scene13Images = gArea13CardChaseStageTextures;
                switch (gRuntimeCurrentArea) {
                case 11:
                    selectedImages = scene11Images.ids;
                    break;
                case 5:
                    selectedImages = scene5Images.ids;
                    break;
                case 13:
                    selectedImages = scene13Images.ids;
                    break;
                default:
                    TRACE("Strange area. curAreaNum = %d\n", gRuntimeCurrentArea);
                    break;
                }
                selectedImage = selectedImages[playerGetChaseStage()];
                (*(s32 ***)(gCurrentAreaConfiguration + 0x74))[gAreaScenarioIndex][2] = selectedImage;
                TRACE("title image changed, to %d\n", selectedImage);
            }
        }
        backdropSelectForView(0, 0xA, 0);
        presentationSaveActiveActor();
        playerResetViewOrientation();
        cameraBuildInterfaceTransform(&gPresentationToWorldTransform);
        presentationPrepareActiveActor(&gCardPresentationActorPosition);
        actionScriptStartWithMode((u8 *)gPikachuActor, gCardPresentationActionScript, 2);
        presentationSetFocusTarget(gPresentationActorFocusTargets);
        cameraResetOverlayColor(0, 0, 0, 0, 0);
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.6f);
        audioChangeCurrentCue(gCardPresentationAudioCue, (f64)0.1f, (f64)0.1f);
        switch (gCardPresentationAudioCue) {
        case 20:
            gCardPresentationDismissalDeadline = (u32)(runtimeGetElapsedMilliseconds() + 0x1CCA);
            break;
        default:
        case 21:
            gCardPresentationDismissalDeadline = (u32)(runtimeGetElapsedMilliseconds() + 0xC62);
            break;
        }
        gCardPresentationStage = 1;
        return;
    case 1:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            gCardPresentationStage = 2;
        }
        presentationPlaceActiveActor(&gCardPresentationActorPosition);
        return;
    case 2:
        now = (u32)runtimeGetElapsedMilliseconds();
        if ((gControllerPrimaryRepeatedButtons != 0) || ((u32)gCardPresentationDismissalDeadline < now)) {
            audioChangeCurrentCue(-1, (f64)0.1f, (f64)0.1f);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.8f);
            gCardPresentationStage = 3;
        }
        presentationPlaceActiveActor(&gCardPresentationActorPosition);
        return;
    case 3:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            backdropSelectForView(0, ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->backdrop, 1);
            presentationRestoreActiveActor();
            hudSetAreaLabelVisible(1);
            runtimeRequestGameModeChange(0);
            gCardPresentationStage = 0;
            return;
        }
        presentationPlaceActiveActor(&gCardPresentationActorPosition);
        return;
    }
}

/// Shared text-image configurations selected by text panels, menus and message styles.
/// \note Original identifier: \c D_80074DE0 (0x80074DE0).
extern u8 gTextImageConfigurations[];
/// Collection title text used to create the presentation title image.
/// \note Original identifier: \c D_800F0B78 (0x800F0B78).
extern u8 gCollectionPresentationTitleText[];
/// Owned text image for the collection title.
/// \note Original identifier: \c D_800F0BE8 (0x800F0BE8).
extern s32 gCollectionPresentationTitleImage;

/// Create owned text for the current collection presentation title.
/// Requires a title of at most 79 bytes and no unreleased previous title image.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028FB0 (0x80028FB0).
void presentationCreateCollectionTitleText(void) {
    s8 sp18[0x50];

    sprintf(sp18, "%s", gCollectionPresentationTitleText);
    gCollectionPresentationTitleImage = 0;
    textImageCreateFromText((TextImage **)(&gCollectionPresentationTitleImage), (u8 *)(sp18),
                            (FontImage *)(gTextImageConfigurations));
}

/// Interior collection-value view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AAC (0x80125AAC).
extern s32 gItemKindCollectionValueEntries[];
/// Two owned item-name and value images alternated during collection results.
/// \note Original identifier: \c D_800F0BF0 (0x800F0BF0).
extern u8 *gCollectionPresentationSubtitleImages[2];
/// Index of the currently displayed collection subtitle image.
/// \note Original identifier: \c D_800F0BF8 (0x800F0BF8).
extern s32 gCollectionPresentationSubtitleIndex;
/// Updates the collection subtitle with an item’s name and value.
/// \param object Live, initialized item with a valid catalog kind, or null to leave the subtitle unchanged.
/// The formatted item name and value must fit in 79 bytes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028FFC (0x80028FFC).
void presentationSetCollectionItemText(u8 *object) {
    s8 buffer[0x50];
    u8 *state;
    u8 **slot;
    s32 value;
    s32 radix = 10;
    if (object != NULL) {
        state = *(u8 **)(object + 0x54);
        value = *(s32 *)((u8 *)gItemKindCollectionValueEntries + *(s16 *)(state + 4) * 0x74);
        sprintf(buffer, "%s : @Y@Z%d", itemGetDisplayName(*(s16 *)(state + 4), 1), value);
        while ((value /= radix) >= 2) {
        }
        gCollectionPresentationSubtitleIndex ^= 1;
        slot = &gCollectionPresentationSubtitleImages[gCollectionPresentationSubtitleIndex];
        if (*slot != NULL) {
            textImageDestroy((TextImage *)*slot);
            *slot = NULL;
        }
        textImageCreateFromText((TextImage **)((s32 *)slot), (u8 *)(buffer), (FontImage *)(gTextImageConfigurations));
    }
}


/// Draw the prepared collection title and any current item subtitle; requires a live title image.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002912C (0x8002912C).
void presentationDrawCollectionText(Gfx **gdl) {
    u8 *sub;

    textDrawBegin(*(s32 *)gdl);
    textDrawSetFont(0);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
    textDrawSetColor(0xFF, 0xFF, 0xFF, 0xFE);
    textDrawSetSpacingMode(1);
    textDrawSetArea(0x1E, 0x1F, 0x118, 0x2F);
    textDrawSetCursor((s32)(0xFA - ((TextImage *)gCollectionPresentationTitleImage)->width) / 2,
                      (s32)(0x10 - ((TextImage *)gCollectionPresentationTitleImage)->height) / 2);
    textDrawImageAtCursor((TextImage *)gCollectionPresentationTitleImage);
    sub = gCollectionPresentationSubtitleImages[gCollectionPresentationSubtitleIndex];
    if (sub != NULL) {
        textDrawSetArea(0x1E, 0x31, 0x118, 0x41);
        textDrawSetCursor((s32)(0xFA - ((TextImage *)sub)->width) / 2, (s32)(0x10 - ((TextImage *)sub)->height) / 2);
        textDrawImageAtCursor((TextImage *)sub);
    }
    textDrawGetDisplayListCursor(gdl);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
}

/// Accumulated collection value awarded to the player at the end of results.
/// \note Original identifier: \c D_800F0BD8 (0x800F0BD8).
extern s32 gCollectionPresentationScore;
/// Owned text image for the current collection score, or zero before creation.
/// \note Original identifier: \c D_800F0BE4 (0x800F0BE4).
extern TextImage *gCollectionPresentationScoreImage;

/// Refreshes the displayed text for the current collection score.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80029298 (0x80029298).
void presentationRefreshCollectionScoreText(void) {
    s8 sp20[0x28];

    sprintf(sp20, "@Y@Z%d", gCollectionPresentationScore);
    if (gCollectionPresentationScoreImage != 0) {
        textImageDestroy((TextImage *)gCollectionPresentationScoreImage);
        gCollectionPresentationScoreImage = 0;
    }
    textImageCreateFromText((TextImage **)(&gCollectionPresentationScoreImage), (u8 *)(sp20),
                            (FontImage *)(gTextImageConfigurations));
}

/// Draws the current collection score when its text is available.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80029304 (0x80029304).
void presentationDrawCollectionScore(Gfx **gdl) {
    if (gCollectionPresentationScoreImage != 0) {
        textDrawBegin(*(s32 *)gdl);
        textDrawSetFont(0);
        textDrawReset();
        textDrawSetPalette(gDefaultTextPalette);
        textDrawSetArea(0x90, 0xC7, 0x110, 0xD7);
        textDrawSetCursor(0, (s32)(0x10 - ((TextImage *)gCollectionPresentationScoreImage)->height) / 2);
        textDrawSetColor(0xFF, 0xFF, 0xFF, 0xFE);
        textDrawSetSpacingMode(1);
        textDrawImageAtCursor((TextImage *)gCollectionPresentationScoreImage);
        textDrawGetDisplayListCursor(gdl);
        textDrawReset();
        textDrawSetPalette(gDefaultTextPalette);
    }
}


/// Draws the decorative elements of the collection presentation.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800293D8 (0x800293D8).
void presentationDrawCollectionDecorations(Gfx **gdl) {
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    textureDrawRectangles(6, (u8 *)gCollectionPresentationDecorations, gdl);
}

/// Live item displayed during collection results, or zero when none is shown.
/// \note Original identifier: \c D_800F0BDC (0x800F0BDC).
extern s32 gCollectionPresentationItem;

/// Draws the item currently shown in the collection presentation, if any.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80029494 (0x80029494).
void presentationDrawCollectionItem(void) {
    if (gCollectionPresentationItem != 0) {
        objectDrawHierarchyGroup(gCollectionPresentationItem, 0x2000);
    }
}

/// Draws the collection presentation for its current stage.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800294C4 (0x800294C4).
void presentationDrawCollectionScreen(Gfx **gdl) {
    backdropDraw(gdl);
    objectDrawHierarchyGroup(gPikachuActor, 0x2000);
    presentationDrawCollectionItem();
    if (gCollectionPresentationStage != 4 && gCollectionPresentationStage != 6) {
        presentationDrawCollectionDecorations(gdl);
        presentationDrawCollectionText(gdl);
        presentationDrawCollectionScore(gdl);
    }
    if (gCollectionPresentationStage == 4) {
        textWindowDrawEmbedded();
    }
    cameraUpdateOverlayColors(0, 0);
    cameraDrawSecondaryOverlay(0);
}


/// Gets the collection score awarded for an item.
/// \param id Handle of a live object; an item must have initialized state and a valid catalog kind.
/// \returns The item’s collection value, or zero for an object that is not an item.
/// \note Original decompilation identifier: \c func_80029560 (0x80029560).
s32 presentationGetItemCollectionValue(s32 id) {
    u8 *liveObject;
    u8 *p;

    liveObject = objectFindLiveByHandle(id);
    if (*(u16 *)(liveObject + 0x34) != 0x86) {
        return 0;
    }
    p = *(u8 **)(liveObject + 0x54);
    return *(s32 *)((u8 *)gItemKindCollectionValueEntries + (*(s16 *)(p + 4) * 0x74));
}


/// Position, scale, and turn an item for the collection presentation.
/// \param item Live item with an initialized model hierarchy; requires initialized area and presentation transforms.
/// \param heightOffset Vertical offset from the item’s reference position in presentation coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800295BC (0x800295BC).
void presentationPlaceCollectionItem(u8 *item, f32 heightOffset) {
    Vec3_295BC v;

    v.x = gCollectionItemPresentationPosition.x;
    v.y = gCollectionItemPresentationPosition.y;
    v.z = gCollectionItemPresentationPosition.z;
    v.y += heightOffset;
    matrix34TransformPointVector(gPresentationToWorldTransform, &v, item + 0x18);
    *(f32 *)(item + 0x10) += 3.0;
    *(f32 *)(item + 0) = *(f32 *)(gCurrentAreaConfiguration + 0xC);
    *(f32 *)(item + 4) = *(f32 *)(gCurrentAreaConfiguration + 0xC);
    *(f32 *)(item + 8) = *(f32 *)(gCurrentAreaConfiguration + 0xC);
    objectUpdateHierarchy(item);
}

u32 randomBelow(s32);
u32 randomNext(void);
/// Interior presentation-response category view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AB0 (0x80125AB0).
extern s8 gItemKindResponseCategoryEntries[];

/// Chooses a response for the active actor based on the presented item.
/// \param id Handle of a live, initialized item with a valid catalog kind; requires an initialized active actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002966C (0x8002966C).
void presentationChooseActorResponseToItem(s32 id) {
    s32 index;
    u8 *object;
    u8 *state;
    s32 action;
    s32 category;
    s32 *previous;

    previous = &gCollectionPreviousResponseIndex;
    object = objectFindLiveByHandle(id);
    state = *(u8 **)(object + 0x54);
    category = gItemKindResponseCategoryEntries[*(s16 *)(state + 4) * 0x74];
    index = randomBelow(4);
    if (index == *previous) {
        index++;
        while (index < 0)
            index += 4;
        while (index >= 4)
            index -= 4;
    }
    *previous = index;
    switch (category) {
    case 2:
        action = gPresentationItemCategory2ResponseActions[index];
        presentationSetFocusTarget(&gPresentationActorFocusTargets[2]);
        break;
    case 0:
        action = gPresentationItemCategory0ResponseActions[index];
        presentationSetFocusTarget(gPresentationActorFocusTargets);
        break;
    case 1:
        presentationSetFocusTarget(&gPresentationActorFocusTargets[randomBelow(3)]);
        if (randomNext() * (1.0 / 4294967296.0) < 0.3)
            return;
        action = gPresentationItemCategory1ResponseActions[index];
        break;
    default:
        action = 0x57;
        break;
    }
    actionScriptStart(gPikachuActor, action);
}

/// Separate view of the retained bonus item slot.
/// \note Original identifier: \c D_800F1CD0 (0x800F1CD0).
/// Separate view of the two ten-entry toolbox item rows.
/// \note Original identifier: \c D_800F1C08 (0x800F1C08).
extern u8 gToolboxItemSlots[], gToolboxBonusSlot[];
/// Per-kind item motion, rendering, response and description configuration.
/// \note Original identifier: \c D_80125A40 (0x80125A40).
extern ItemKind gItemKinds[];
/// Owned toolbox display state, item rows, placeholders and special storage slots.
/// \note Original identifier: \c D_800F1BE0 (0x800F1BE0).
extern ToolboxState gToolboxState;
/// Primary controller new and auto-repeated button presses; aliases the repeated word of the processed sample.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
extern s32 gControllerPrimaryRepeatedButtons;
void presentationChooseActorResponseToItem(s32);

/// Advance the presentation and scoring of collected items, removing each eligible item from its kept slot.
/// Requires valid slot kinds, live item handles throughout the sequence, and a score total that fits in signed 32 bits.
/// \returns One when all eligible collected items have been presented, or zero while the sequence is in progress.
///
/// \note Portability: The empty debug check preserves IDO register allocation; its original operand is not recoverable.
/// \note Original decompilation identifier: \c func_80029814 (0x80029814).
s32 presentationUpdateItemCollectionSequence(void) {
    ToolboxSlot *row = (ToolboxSlot *)gToolboxItemSlots;
    static s32 selectionIndex = 0;
    static u32 transitionTime = 0;
    static s32 transitionState = 0;
    static s32 selectedId = 0;
    u8 *object;
    f32 progress;
    f64 height;
    s32 index;
    s32 id;
    ToolboxSlot *slot;
    s32 value;
    f32 elapsedTicks;
    gCollectionPresentationItem = 0;
    switch (transitionState) {
    case 0:
        for (;;) {
            if (selectionIndex >= 10) {
                /* The overflow slot's id, read through the D_800F1BE0 base. */
                if (gToolboxState.singletons[0].id == 0) {
                    selectionIndex = 0;
                    return 1;
                }
                slot = (ToolboxSlot *)gToolboxBonusSlot;
            } else {
                slot = &row[selectionIndex];
            }
            if ((s8)gItemKinds[slot->kind].itemClass != 1 || slot->id == 0) {
                selectionIndex++;
                continue;
            }
            break;
        }
        id = slot->id;
        selectedId = id;
        object = objectFindLiveByHandle(id);
        objectClearHierarchyFlags(object, 2);
        slot->id = 0;
        slot->kind = 0;
        transitionTime = runtimeGetElapsedMilliseconds();
        transitionState = 1;
        presentationSetFocusTarget(gPresentationActorFocusTargets);
        presentationSetCollectionItemText(object);
    case 1:
        object = objectFindLiveByHandle(selectedId);
        elapsedTicks = (f32)(runtimeGetElapsedMilliseconds() - transitionTime);
        progress = elapsedTicks / 600.0f;
        if (progress >= 1.0) {
            transitionTime = runtimeGetElapsedMilliseconds();
            transitionState = 2;
            presentationChooseActorResponseToItem(selectedId);
            value = presentationGetItemCollectionValue(selectedId);
            if (value >= 5000)
                audioPlayCue(0x1D);
            playerPlayTrackedSoundEffect((u8 *)&gPlayerState, 0xAE);
            gCollectionPresentationScore += value;
            presentationRefreshCollectionScoreText();
        }
        height = (1.0 - progress) * 20.0;
        progress = height;
        presentationPlaceCollectionItem(object, progress);
        break;
    case 2:





        if (selectedId == 1) {
        }
        object = objectFindLiveByHandle(selectedId);
        elapsedTicks = (f32)(runtimeGetElapsedMilliseconds() - transitionTime);
        progress = elapsedTicks / 1000.0f;
        if (progress >= 1.0 || gControllerPrimaryRepeatedButtons != 0) {
            transitionTime = runtimeGetElapsedMilliseconds();
            transitionState = 3;
        }
        presentationPlaceCollectionItem(object, 0.0f);
        break;
    case 3:
        object = objectFindLiveByHandle(selectedId);
        elapsedTicks = (f32)(runtimeGetElapsedMilliseconds() - transitionTime);
        progress = elapsedTicks / 300.0f;
        if (progress >= 1.0) {
            u32 completedAt = runtimeGetElapsedMilliseconds();
            index = selectionIndex;
            transitionTime = completedAt;
            index++;
            selectionIndex = index;
            transitionState = 0;
        }
        height = progress * -20.0;
        progress = height;
        presentationPlaceCollectionItem(object, progress);
        break;
    }
    gCollectionPresentationItem = (s32)object;
    return 0;
}

/* Placed after presentationUpdateItemCollectionSequence so it follows that function's statics. */
/// Elapsed-millisecond checkpoint for collection result focus and dialogue waits.
/// \note Original identifier: \c D_80099D14 (0x80099D14).
u32 gCollectionResultsWaitStartMilliseconds = 0;

/// Show the pending chapter friendship message and the active actor’s corresponding reaction, then consume the pending result.
/// Requires an initialized active actor and a pending message index from zero through 19.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80029BE4 (0x80029BE4).
void presentationShowChapterFriendshipResult(void) {
    u8 level;
    s32 value;

    textWindowOpen(2, 0, 0);
    textWindowAppendText(gChapterFriendshipMessages[gPlayerFriendshipMilestone]);
    textWindowShowFirstPage();
    level = gPlayerFriendshipLevel;
    value = level;
    TRACE("value = %d\n", value);
    if (value < 3) {
        actionScriptStart(gPikachuActor, 0x8E);
    } else if (value < 6) {
        actionScriptStart(gPikachuActor, 0xEB);
    } else if (value < 9) {
        actionScriptStart(gPikachuActor, 0x3E);
    } else if (value < 0xC) {
        actionScriptStart(gPikachuActor, 0x8C);
    } else {
        actionScriptStart(gPikachuActor, 0x87);
    }
    gPlayerFriendshipMilestone = -1;
}

u64 runtimeGetElapsedMilliseconds();
void runtimeRequestProgramChange(s32);
void audioStopCategories(s32, f64);
void areaSetNextDestination(s32, s32);
void playerAddPoints(s32);
/// Whether collection results were bypassed because no kept items or chapter result required presentation.
/// \note Original identifier: \c D_8009B584 (0x8009B584).
extern s32 gCollectionResultsBypassed;

/// Advance the collection score and chapter result presentation, then leave the results mode when it finishes.
/// Requires initialized actor, area, and inventory state; the awarded points must fit in signed 32 bits.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80029CD4 (0x80029CD4).
void presentationUpdateCollectionResultsMode(void) {
    playerResetViewOrientation();
    cameraBuildInterfaceTransform(gPresentationToWorldTransform);
    if (actionScriptIsStopped(gPikachuActor) != 0) {
        actionScriptStart(gPikachuActor, 0x57);
    }
    switch (gCollectionPresentationStage) {
    case 0:
        controllerRequestRumbleReset(0);
        if ((inventoryRecordKeptItemsAndClearTemporarySlot() == 0) && (gPlayerFriendshipMilestone == -1)) {
            gCollectionResultsBypassed = 1;
            areaSetNextDestination(0, 0);
            runtimeRequestProgramChange(0);
            return;
        }
        gCollectionResultsBypassed = 0;
        backdropSelectForView(0, 0xB, 0);
        cameraResetOverlayColor(0, 0, 0, 0, 0);
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.6f);
        audioStopCategories(3, 0.6f);
        presentationPrepareActiveActor(&gCollectionPresentationActorPosition);
        gCollectionPresentationScore = 0;
        presentationRefreshCollectionScoreText();
        gCollectionPresentationItem = 0;
        gCollectionPresentationStage = 1;
        return;
    case 1:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            gCollectionPresentationStage = 2;
        }
        presentationPlaceActiveActor(&gCollectionPresentationActorPosition);
        return;
    case 2:
        if (presentationUpdateItemCollectionSequence() != 0) {
            playerAddPoints(gCollectionPresentationScore);
            if (gPlayerFriendshipMilestone != -1) {
                presentationSetFocusTarget(gPresentationActorFocusTargets);
                audioPlayCue(0x26);
                gCollectionPresentationStage = 3;
                gCollectionResultsWaitStartMilliseconds = (u32)runtimeGetElapsedMilliseconds();
            } else {
                if (gCollectionPresentationScore <= 0) {
                    actionScriptStart(gPikachuActor, 0x7D);
                    audioPlayCue(0x22);
                } else {
                    audioPlayCue(0x1C);
                }
                presentationSetFocusTarget(&gPresentationActorFocusTargets[randomBelow(3)]);
                gCollectionResultsWaitStartMilliseconds = (u32)runtimeGetElapsedMilliseconds();
                gCollectionPresentationStage = 5;
            }
        }
        presentationPlaceActiveActor(&gCollectionPresentationActorPosition);
        return;
    case 3:
        if (runtimeGetElapsedMilliseconds() - (u64)gCollectionResultsWaitStartMilliseconds >= 0x259) {
            presentationShowChapterFriendshipResult();
            gCollectionPresentationStage = 4;
        }
        presentationPlaceActiveActor(&gCollectionPresentationActorPosition);
        return;
    case 4:
        if (gControllerPrimaryRepeatedButtons & 0x8000) {
            audioStopCategories(3, 0.1f);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.8f);
            gCollectionPresentationStage = 6;
        }
        presentationPlaceActiveActor(&gCollectionPresentationActorPosition);
        return;
    case 5:
        if ((gControllerPrimaryRepeatedButtons != 0) ||
            (runtimeGetElapsedMilliseconds() - (u64)gCollectionResultsWaitStartMilliseconds >= 0xBB9)) {
            audioStopCategories(3, 0.1f);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.8f);
            gCollectionPresentationStage = 6;
        }
        presentationPlaceActiveActor(&gCollectionPresentationActorPosition);
        return;
    case 6:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            gCollectionPresentationStage = 0;
            areaSetNextDestination(0, 0);
            runtimeRequestProgramChange(0);
        }
        presentationPlaceActiveActor(&gCollectionPresentationActorPosition);
        return;
    default:
        return;
    }
}

s32 progressGetValue(s32);
s32 progressAddValue(s32, s32);
void playerSetChaseStage(s32);
/// Selected eye-catch group index, or minus one when no interlude is selected.
/// \note Original identifier: \c D_8009B580 (0x8009B580).
extern s32 gEyeCatchSelectedGroup;
/// Pending completed-visit card and progress update, consumed by area-zero card selection.
/// \note Original identifier: \c D_800F0B74 (0x800F0B74).
extern s32 gCompletedVisitCardPending;
s32 eyeCatchChooseGroup();
void scriptVmResetGlobals(void);

/// Selects the area 0 card presentation and updates related progress for a completed visit.
/// \param actorAction Unused; the caller’s presentation action remains unchanged.
/// \param audioCue Receives the presentation audio cue ID for a supported result; otherwise unchanged.
/// \returns The selected card index, or zero when the chapter has no supported selection.
/// \note Original decompilation identifier: \c func_8002A0E4 (0x8002A0E4).
s32 presentationSelectArea0Card(s32 *actorAction, s32 *audioCue) {
    s32 sp1C;
    s32 sp18;

    sp1C = progressGetCurrentChapter();
    TRACE("iedeTimer:%d otukaiTimer:%d\n", progressGetValue(0x13), progressGetValue(0x14));
    TRACE("gameClear:%d timer:%d r:%d\n", gCompletedVisitCardPending, progressGetValue(0x1A), sp1C);
    if (gCompletedVisitCardPending != 0) {
        if ((gCollectionResultsBypassed != 0) && (progressGetCurrentChapter() >= 5)) {
            gEyeCatchSelectedGroup = eyeCatchChooseGroup();
        }
        progressAddValue(6, 1);
        progressSetValue(0x12, 0);
        *(s32 *)(gCurrentAreaConfiguration + 0x64) = 3;
        scriptVmResetGlobals();
        playerSetChaseStage(progressGetValue(0xC));
        if (progressGetValue(0xC2) != 0) {
            progressAddValue(0x13, 1);
        } else {
            progressAddValue(0x13, 2);
        }
        progressAddValue(0x14, 1);
        if (sp1C == 8) {
            progressSetValue(0x12, 1);
        } else if ((sp1C == 7) && (progressGetValue(0x11) == 0) && (progressGetValue(0x12) == 0)) {
            if ((progressGetValue(0x1A) >= 0xE) || ((s32)randomBelow(progressGetValue(0x13)) >= 8)) {
                progressSetValue(0x11, 1);
            } else {
                sp18 = randomBelow(progressGetValue(0x14));
                if ((progressGetValue(0xE) != 0) && (sp18 >= 0xE)) {
                    progressSetValue(0x12, 1);
                }
            }
        }
        gCompletedVisitCardPending = 0;
        *audioCue = 0x15;
        return 9;
    }
    *(s32 *)(gCurrentAreaConfiguration + 0x64) = 1;
    if (progressGetValue(0x11) != 0) {
        progressSetValue(0x1A, 0);
        progressSetValue(0x13, 0);
        *audioCue = 0x15;
        return 6;
    }
    if (progressGetValue(0x12) != 0) {
        progressSetValue(0x14, 0);
        *audioCue = 0x15;
        return 7;
    }
    switch (sp1C) {
    case 1:
        *audioCue = 0x15;
        return 0;
    case 2:
        *audioCue = 0x15;
        return 1;
    case 3:
        *audioCue = 0x15;
        return 2;
    case 4:
        *audioCue = 0x15;
        return 3;
    case 5:
        *audioCue = 0x15;
        return 4;
    case 6:
        *audioCue = 0x15;
        return 5;
    case 7:
        *audioCue = 0x15;
        return 4;
    case 8:
        progressSetValue(0x14, 0);
        *audioCue = 0x15;
        return 7;
    case 9:
        *audioCue = 0x15;
        return 8;
    default:
        TRACE("ERROR: Strange flow. chapter:%d\n", sp1C);
        return 0;
    }
}

/// Selects the area 2 card presentation according to chapter progress.
/// \param actorAction Unused; the caller’s presentation action remains unchanged.
/// \param audioCue Receives the presentation audio cue ID for a supported result; otherwise unchanged.
/// \returns The selected card index, or zero when the chapter has no supported selection.
/// \note Original decompilation identifier: \c func_8002A404 (0x8002A404).
s32 presentationSelectArea2Card(s32 *actorAction, s32 *audioCue) {
    s32 currentChapter;

    currentChapter = progressGetCurrentChapter();
    switch (currentChapter) {
    case 4:
        *audioCue = 0x15;
        return 1;
    case 6:
        *audioCue = 0x15;
        return 2;
    case 8:
        *(s32 *)(gCurrentAreaConfiguration + 0x64) = 2;
        *audioCue = 0x15;
        return 3;
    case 9:
        *(s32 *)(gCurrentAreaConfiguration + 0x64) = 2;
        *audioCue = 0x15;
        return 4;
    case 3:
        *(s32 *)(gCurrentAreaConfiguration + 0x64) = 2;
        *audioCue = 0x15;
        return 0;
    case 5:
        *audioCue = 0x15;
        return 5;
    case 7:
        if (progressGetValue(0x12) != 0) {
            *(s32 *)(gCurrentAreaConfiguration + 0x64) = 2;
            *audioCue = 0x15;
            return 6;
        }
        *audioCue = 0x15;
        return 5;
    default:
        TRACE("ERROR: Strange flow. chapter:%d\n", currentChapter);
        return 0;
    }
}

/// Selects the card presentation for area 3 according to game progress.
/// \param actorAction Receives the presentation action ID for chapters 2 and 4; otherwise unchanged.
/// \param audioCue Receives the presentation audio cue ID, or minus one for silence, for a supported chapter; otherwise unchanged.
/// \returns The selected card index, or zero for an unsupported chapter.
/// \note Original decompilation identifier: \c func_8002A528 (0x8002A528).
s32 presentationSelectArea3Card(s32 *actorAction, s32 *audioCue) {
    s32 currentChapter;

    currentChapter = progressGetCurrentChapter();
    switch (currentChapter) { /* irregular */
    case 2:
        *actorAction = 0x115;
        *audioCue = 0x14;
        return 0;
    case 4:
        *actorAction = 0x115;
        *audioCue = 0x14;
        return 1;
    case 9:
        *audioCue = -1;
        return 2;
    default:
        TRACE("ERROR: Strange flow. chapter:%d\n", currentChapter);
        return 0;
    }
}

/// Selects the card presentation for area 4 according to game progress.
/// \param actorAction Receives the presentation action ID for chapter 4; otherwise unchanged.
/// \param audioCue Receives the presentation audio cue ID for a supported chapter; otherwise unchanged.
/// \returns The selected card index, always zero.
/// \note Original decompilation identifier: \c func_8002A5B4 (0x8002A5B4).
s32 presentationSelectArea4Card(s32 *actorAction, s32 *audioCue) {
    s32 chapter = progressGetCurrentChapter();

    if (chapter == 4) {
        *actorAction = 0x115;
        *audioCue = 0x14;
        return 0;
    }
    TRACE("ERROR: Strange flow. chapter:%d\n", chapter);
    return 0;
}

/// Selects the card presentation for area 5 according to game progress.
/// \param actorAction Receives the presentation action ID for a supported chapter; otherwise unchanged.
/// \param audioCue Receives the presentation audio cue ID for a supported chapter; otherwise unchanged.
/// \returns The selected card index, or zero for an unsupported chapter.
/// \note Original decompilation identifier: \c func_8002A604 (0x8002A604).
s32 presentationSelectArea5Card(s32 *actorAction, s32 *audioCue) {
    s32 currentChapter;

    currentChapter = progressGetCurrentChapter();
    switch (currentChapter) { /* irregular */
    case 4:
        *actorAction = 0x115;
        *audioCue = 0x14;
        return 0;
    case 3:
    case 5:
    case 7:
        *actorAction = 0x57;
        *audioCue = 0x15;
        return 1;
    default:
        TRACE("ERROR: Strange flow. chapter:%d\n", currentChapter);
        return 0;
    }
}

s32 progressGetValue(s32); /* extern */

/// Selects the card presentation for areas 6 through 8 according to game progress.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index.
/// \note Original decompilation identifier: \c func_8002A68C (0x8002A68C).
s32 presentationSelectArea6Through8Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    if (progressGetValue(0x12) != 0) {
        *actorAction = 0x3B;
        *audioCue = 0x15;
        return 0;
    }
    *actorAction = 0x8C;
    *audioCue = 0x15;
    return 1;
}

/// Selects the card presentation for areas 9 and 10.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index, always zero.
/// \note Original decompilation identifier: \c func_8002A6F8 (0x8002A6F8).
s32 presentationSelectArea9And10Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    *actorAction = 0xA8;
    *audioCue = 0x15;
    return 0;
}

/// Selects the card presentation for areas 11 and 12.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index, always zero.
/// \note Original decompilation identifier: \c func_8002A738 (0x8002A738).
s32 presentationSelectArea11And12Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    *actorAction = 0x87;
    *audioCue = 0x15;
    return 0;
}

/// Selects the card presentation for area 13.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index, always zero.
/// \note Original decompilation identifier: \c func_8002A778 (0x8002A778).
s32 presentationSelectArea13Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    *actorAction = 0xDD;
    *audioCue = 0x15;
    return 0;
}

/// Selects the card presentation for area 14.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index, always zero.
/// \note Original decompilation identifier: \c func_8002A7B8 (0x8002A7B8).
s32 presentationSelectArea14Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    *actorAction = 0x33;
    *audioCue = 0x15;
    return 0;
}


/// Selects the card presentation for area 15 according to game progress.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index.
/// \note Original decompilation identifier: \c func_8002A7F8 (0x8002A7F8).
s32 presentationSelectArea15Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    if (progressGetValue(0x12) != 0) {
        *actorAction = 0xF4;
        *audioCue = 0x15;
        return 0;
    }
    *actorAction = 0xF4;
    *audioCue = 0x15;
    return 1;
}

/// Selects the card presentation for area 16.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index, always zero.
/// \note Original decompilation identifier: \c func_8002A864 (0x8002A864).
s32 presentationSelectArea16Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    *actorAction = 0xF4;
    *audioCue = 0x15;
    return 0;
}

/// Selects the card presentation for areas 17 through 19.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index, always zero.
/// \note Original decompilation identifier: \c func_8002A8A4 (0x8002A8A4).
s32 presentationSelectArea17Through19Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    *actorAction = 0x32;
    *audioCue = 0x15;
    return 0;
}

/// Selects the card presentation for area 20 according to game progress.
/// \param actorAction Receives the presentation action ID for a supported chapter; otherwise unchanged.
/// \param audioCue Receives the presentation audio cue ID for a supported chapter; otherwise unchanged.
/// \returns The selected card index, or zero for an unsupported chapter.
/// \note Original decompilation identifier: \c func_8002A8E4 (0x8002A8E4).
s32 presentationSelectArea20Card(s32 *actorAction, s32 *audioCue) {
    s32 currentChapter;

    currentChapter = progressGetCurrentChapter();
    switch (currentChapter) { /* irregular */
    case 1:
        *actorAction = 0xF4;
        *audioCue = 0x15;
        return 0;
    case 7:
        *actorAction = 0xF4;
        *audioCue = 0x15;
        return 1;
    default:
        TRACE("ERROR: Strange flow. chapter:%d\n", currentChapter);
        return 0;
    }
}

/// Selects the card presentation for area 21.
/// \param actorAction Receives the active actor’s presentation action ID.
/// \param audioCue Receives the presentation audio cue ID.
/// \returns The selected card index, always zero.
/// \note Original decompilation identifier: \c func_8002A958 (0x8002A958).
s32 presentationSelectArea21Card(s32 *actorAction, s32 *audioCue) {
    progressGetCurrentChapter();
    *actorAction = 0x32;
    *audioCue = 0x15;
    return 0;
}

/// Selects the card presentation for area 22 according to game progress.
/// \param actorAction Receives the presentation action ID for chapter 9; otherwise unchanged.
/// \param audioCue Receives the presentation audio cue ID for a supported chapter; otherwise unchanged.
/// \returns The selected card index, or zero for an unsupported chapter.
/// \note Original decompilation identifier: \c func_8002A998 (0x8002A998).
s32 presentationSelectArea22Card(s32 *actorAction, s32 *audioCue) {
    s32 currentChapter;

    progressGetCurrentChapter();
    currentChapter = progressGetCurrentChapter();
    switch (currentChapter) { /* irregular */
    case 0:
        *audioCue = 0;
        return 1;
    case 9:
        *actorAction = 0x32;
        *audioCue = 0x15;
        return 0;
    default:
        TRACE("ERROR: Strange flow. chapter:%d\n", currentChapter);
        return 0;
    }
}

s8 *areaGetRegionName(u32);
/// Entrance index used to initialize the player and companion in the current area.
/// \note Original identifier: \c D_800AE4F4 (0x800AE4F4).
extern s32 gRuntimeCurrentEntrance;
/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern u32 gRuntimeCurrentArea;
/// Whether the current entrance suppresses the area card presentation.
/// \note Original identifier: \c D_800F0BA0 (0x800F0BA0).
extern s32 gCardPresentationSuppressed;
/// Audio cue selected for the current area card presentation.
/// \note Original identifier: \c D_800F0BA4 (0x800F0BA4).
extern s32 gCardPresentationAudioCue;
/// Actor action script selected for the current area card presentation.
/// \note Original identifier: \c D_800F1068 (0x800F1068).
extern s32 gCardPresentationActionScript;

/// Prepare the current area’s card presentation and owned collection result text.
/// Requires a supported area, initialized area data, and no unreleased presentation text images.
/// The selected collection title must fit its destination and contain at most 79 bytes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002AA08 (0x8002AA08).
void presentationInitializeForCurrentArea(void) {
    s32 collectionTitleAddress;
    s32 collectionTitleTableAddress;
    s8 *regionName;

    cameraResetOverlayColor(0, 0, 0, 0, 0);
    cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.0f);
    if (gRuntimeCurrentEntrance != 0) {
        gCardPresentationSuppressed = 1;
    } else {
        gCardPresentationSuppressed = 0;
    }
    gCardPresentationActionScript = 0x57;
    switch (gRuntimeCurrentArea) {
    case 0:
        gAreaScenarioIndex =
            presentationSelectArea0Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 1:
        gCardPresentationAudioCue = 0x15;
        gAreaScenarioIndex = 0;
        break;
    case 2:
        gAreaScenarioIndex =
            presentationSelectArea2Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 3:
        gAreaScenarioIndex =
            presentationSelectArea3Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 4:
        gAreaScenarioIndex =
            presentationSelectArea4Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 5:
        gAreaScenarioIndex =
            presentationSelectArea5Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 6:
    case 7:
    case 8:
        gAreaScenarioIndex =
            presentationSelectArea6Through8Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 9:
    case 10:
        gAreaScenarioIndex =
            presentationSelectArea9And10Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 11:
    case 12:
        gAreaScenarioIndex =
            presentationSelectArea11And12Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 13:
        gAreaScenarioIndex =
            presentationSelectArea13Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 14:
        gAreaScenarioIndex =
            presentationSelectArea14Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 15:
        gAreaScenarioIndex =
            presentationSelectArea15Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 16:
        gAreaScenarioIndex =
            presentationSelectArea16Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 17:
    case 18:
    case 19:
        gAreaScenarioIndex =
            presentationSelectArea17Through19Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 20:
        gAreaScenarioIndex =
            presentationSelectArea20Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 21:
        gAreaScenarioIndex =
            presentationSelectArea21Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    case 22:
        gAreaScenarioIndex =
            presentationSelectArea22Card(&gCardPresentationActionScript, &gCardPresentationAudioCue);
        break;
    default:
        TRACE("ERROR : unknown area (%d) ?\n", gRuntimeCurrentArea);
        break;
    }
    collectionTitleTableAddress = *(s32 *)(gCurrentAreaConfiguration + 0x78);
    if (collectionTitleTableAddress != 0) {
        collectionTitleAddress = ((s32 *)collectionTitleTableAddress)[gAreaScenarioIndex];
        if (collectionTitleAddress != 0) {
            stringCopy(gCollectionPresentationTitleText, (const u8 *)collectionTitleAddress);
        }
    }
    regionName = areaGetRegionName(gRuntimeCurrentArea);
    if (regionName == 0) {
        gPresentationAreaLabelImage = 0;
    } else {
        textImageCreateFromText((TextImage **)(&gPresentationAreaLabelImage), (u8 *)(regionName),
                                (FontImage *)(gTextImageConfigurations));
    }
    presentationCreateCollectionTitleText();
    gCollectionPresentationScore = 0;
    gCollectionPresentationScoreImage = 0;
    presentationRefreshCollectionScoreText();
    gCollectionPresentationSubtitleImages[0] = 0;
    gCollectionPresentationSubtitleImages[1] = 0;
    gCollectionPresentationSubtitleIndex = 0;
}


/// Gets the current story chapter.
/// \returns The current chapter number.
/// \note Original decompilation identifier: \c osAfterPreNMI (0x8002ACE4).
s32 progressGetCurrentChapter(void) {
    return progressGetValue(0);
}

/// Sets the current story chapter and unlocks the milestones available at that chapter.
/// \param chapter Story chapter to store and use for milestone unlocks.
/// \returns Nothing.
///
/// \note Portability: The empty debug range check preserves IDO register allocation; the original bounds are not recoverable.
/// \note Original decompilation identifier: \c func_8002AD04 (0x8002AD04).
void progressSetChapterAndUnlockMilestones(s32 chapter) {
    if (chapter < 0 || chapter > 9) {
    }
    progressSetValue(0, chapter);
    if (chapter > 0) {
        progressSetValue(0x44, 1);
        if (chapter >= 3) {
            progressSetValue(0x3B, 1);
            progressSetValue(0x3E, 1);
            progressSetValue(0x3C, 1);
            if (chapter >= 5) {
                progressSetValue(0x3D, 1);
                progressSetValue(0x3F, 1);
                progressSetValue(0x43, 1);
            }
        }
    }
}

void progressSetChapterAndUnlockMilestones(s32);
/// Separate symbol view of the primary player profile points balance.
/// \note Original identifier: \c D_800EF790 (0x800EF790).
extern s32 gPlayerProfilePoints;
/// Collection value of item kind 0x8F, also used by the chapter progression threshold.
/// \note Original identifier: \c D_80129B78 (0x80129B78).
extern s32 gItemKind8FCollectionValue;

/// Updates the story chapter after a visit according to completed objectives and unlocks its milestones.
/// \returns The resulting chapter number, which may remain unchanged or return to an earlier chapter.
/// \note Original decompilation identifier: \c func_8002AD9C (0x8002AD9C).
s32 progressUpdateChapterAfterVisit(void) {
    s32 currentChapter;
    s32 var_s0;

    currentChapter = progressGetValue(0);
    var_s0 = currentChapter;
    switch (currentChapter) {
    case 3:
        if (progressAddValue(0xD, 1) >= 4) {
            var_s0 += 1;
            playerSetChaseStage(1);
        }
        break;
    case 5:
        if ((progressGetValue(0x20) != 0) && (progressGetValue(0xB9) != 0) && (progressGetValue(0xBA) != 0) &&
            (progressGetValue(0xAC) != 0)) {
            var_s0 += 1;
            playerSetChaseStage(2);
            progressSetValue(0x1A, 0);
            progressSetValue(0x13, 0);
            progressSetValue(0x14, 0);
        }
        break;
    case 7:
        if ((progressGetValue(0xE) == 0) && ((gItemKind8FCollectionValue + 0x64) < gPlayerProfilePoints) &&
            (progressGetValue(0x23) != 0) && (progressGetValue(0xAD) != 0) && (progressGetValue(0xAF) != 0) &&
            (progressGetValue(0xBD) != 0) && (progressGetValue(0xAE) != 0)) {
            var_s0 += 1;
        }
        break;
    case 8:
        if (progressGetValue(0xF0) != 0) {
            var_s0 += 1;
        } else {
            var_s0 = 7;
        }
        break;
    case 9:
        var_s0 = 7;
        progressSetValue(0xE, 1);
        break;
    default:
        var_s0 = currentChapter + 1;
        break;
    }
    TRACE("chapterGo(%d)\n", var_s0);
    progressSetChapterAndUnlockMilestones(var_s0);
    return var_s0;
}
