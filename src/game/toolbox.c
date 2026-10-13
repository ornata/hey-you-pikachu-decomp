#include "runtime_lifecycle.h"
#include "player_state.h"
#include "render_state.h"
#include "object_template.h"
#include "texture_descriptor.h"
#include "texture_rectangles.h"
#include "object_groups.h"
#include "math_angles.h"
#include "camera_overlay.h"
#include "runtime_control.h"
#include "controller_input.h"
#include "controller_rumble.h"
#include "font_image.h"
#include "text_image_configurations.h"
#include "text_window.h"
#include "message_bar.h"
#include "item_size.h"
#include "item_participation.h"
#include "item_mode.h"
#include "progress_field_types.h"
#include "progress_fields.h"
#include "vector3_types.h"
#include "matrix_types.h"
#include "matrix_point.h"
#include "camera_interface.h"
#include "matrix_orientation.h"
#include "object_flags.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "toolbox_items.h"
#include "item_object.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "item_text.h"
#include "runtime_clock.h"
#include "text_image_create.h"
#include "player_control.h"
#include "text_draw.h"
#include "object_lookup.h"
#include "object_types.h"
#include "item_types.h"
#include "player_types.h"
#include "text_image.h"
#include "toolbox_types.h"
#include "texture_draw_types.h"
#include "item_kind.h"
/// Owned toolbox display state, item rows, placeholders and special storage slots.
/// \note Original identifier: \c D_800F1BE0 (0x800F1BE0).
ToolboxState gToolboxState;

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/* Item seeded into the Item Storage row unless its story flag is clear
 * (-1: always). */
typedef struct {
    s32 flag;
    s32 kind;
} DefaultObject;



/* This object's .data, 0x8009A3F0..0x8009A6B0, in address order. */

/// Linked model template cloned to create the toolbox display.
/// \note Original identifier: \c D_8009A3F0 (0x8009A3F0).
ObjectTemplate gToolboxModelTemplate[6] = {
    {{1.0f, 1.0f, 1.0f},
     {0.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 0.0f},
     606.1f,
     NULL,
     &gToolboxModelTemplate[1],
     (Gfx *)0x0104F2F0,
     0, 0},
    {{1.0f, 1.0f, 1.0f},
     {0.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 0.0f},
     142.5f,
     &gToolboxModelTemplate[2],
     NULL,
     (Gfx *)0x0104F508,
     0, 0},
    {{1.0f, 1.0f, 1.0f},
     {0.0f, 0.0f, 0.0f},
     {0.0f, 312.1642f, 0.0f},
     612.1f,
     &gToolboxModelTemplate[4],
     &gToolboxModelTemplate[3],
     (Gfx *)0x0104FA00,
     0, 0},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {5.9223f, 0.0f, 0.0f}, 196.7f, NULL, NULL, (Gfx *)0x0104FD90, 0, 0},
    {{1.0f, 1.0f, 1.0f},
     {0.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 0.0f},
     606.1f,
     NULL,
     &gToolboxModelTemplate[5],
     (Gfx *)0x010501F0,
     0, 0},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 582.1f, NULL, NULL, (Gfx *)0x01050648, 0, 0},
};

/// Model template cloned for unoccupied toolbox slot displays.
/// \note Original identifier: \c D_8009A540 (0x8009A540).
ObjectTemplate gToolboxPlaceholderTemplate[2] = {
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.0f, NULL, &gToolboxPlaceholderTemplate[1], 0, 0, 0},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 257.0f, NULL, NULL, (Gfx *)0x01008860, 0x98, 0},
};

/// Base-object anchor positions indexed by Collection or Item Storage row.
/// \note Original identifier: \c D_8009A5B0 (0x8009A5B0).
Vec3f gToolboxRowAnchors[2] = {
    {-1.5f, -2.5f, -20.0f},
    {0.0f, -2.5f, -20.0f},
};

/// Initial Item Storage kinds and progress selectors controlling their availability.
/// \note Original identifier: \c D_8009A5C8 (0x8009A5C8).
DefaultObject gToolboxDefaultStoredItems[10] = {
    {-1, 0xA4},   {0x1B, 0x32}, {0x1C, 0x43}, {0x1D, 0x23}, {0x20, 0x44},
    {0x21, 0x45}, {0x22, 0x4A}, {0x23, 0x51}, {0x1E, 0x52}, {0x1F, 0x53},
};

/// Description prompt suffixes indexed by stored item classification.
/// \note Original identifier: \c D_8009A618 (0x8009A618).
char *gToolboxDescriptionSuffixes[7] = {
    "\n",
    "\nPress @I@J to take it out.",
    "\n(You can't take it out.)",
    "\nPress @I@J to take it out.",
    "\nPress @I@J to take it out.",
    "\nPress @I@J to open the menu.",
    "\n???",
};

/// Help messages cycled while the toolbox is open.
/// \note Original identifier: \c D_8009A634 (0x8009A634).
char *gToolboxHelpMessages[6] = {
    "Press @I@J to store or remove an item.",   "Press @K@L to read a description.",
    "Press @U@V to close the tool box.",        "Tilt @k@l up to open the Collection.",
    "Tilt @k@l down to open the Item Storage.", "Tilt @k@l left or right to choose an item.",
};

/// Display titles for the Collection and Item Storage rows.
/// \note Original identifier: \c D_8009A64C (0x8009A64C).
char *gToolboxRowTitles[2] = {"Collection", "Item Storage"};

/// Off-screen world position used to park toolbox objects.
/// \note Original identifier: \c D_8009A654 (0x8009A654).
Vec3f gToolboxParkingPosition = {1000.0f, 1000.0f, 1000.0f};

/// Index of the next toolbox help message to display.
/// \note Original identifier: \c D_8009A660 (0x8009A660).
s32 gToolboxNextHelpMessage = 0;

/// Terminated sprite list used to draw toolbox navigation feedback.
/// \note Original identifier: \c D_8009A664 (0x8009A664).
TextureRectangle gToolboxNavigationSprites[2] = {
    {0xF8, 0, 0, 32, 32, 0, 0, 1.0f, 1.0f},
    {-1, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f},
};

/// RGBA color of the depth-aware overlay behind the toolbox.
/// \note Original identifier: \c D_8009A6AC (0x8009A6AC).
u8 gToolboxBackdropColor[4] = {0, 0, 0, 0x80};
/// Storage type identifiers indexed by resolved progress field ID.
/// \note Original identifier: \c D_8009A754 (0x8009A754).
extern s32 gProgressFieldTypes[];
/// Separate view of the retained purchased item slot.
/// \note Original identifier: \c D_800F1CD8 (0x800F1CD8).
extern ToolboxSlot gToolboxPurchaseSlot;
/// Separate view of the stored purchased item handle.
/// \note Original identifier: \c D_800F1CDC (0x800F1CDC).
extern s32 gToolboxPurchaseHandle;
void toolboxStoreObjectInSlot(ToolboxSlot *, u8 *);
/// Separate view of the retained bonus item slot.
/// \note Original identifier: \c D_800F1CD0 (0x800F1CD0).
extern ToolboxSlot gToolboxBonusSlot;
/// Separate view of the stored bonus item handle.
/// \note Original identifier: \c D_800F1CD4 (0x800F1CD4).
extern s32 gToolboxBonusHandle;
/// Owned toolbox display state, item rows, placeholders and special storage slots.
/// \note Original identifier: \c D_800F1BE0 (0x800F1BE0).
extern ToolboxState gToolboxState;
/// Separate view of the two ten-entry toolbox item rows.
/// \note Original identifier: \c D_800F1C08 (0x800F1C08).
extern s32 gToolboxItemSlots;
/// Separate view of the Item Storage row beginning at its first kind word.
/// \note Original identifier: \c D_800F1C58 (0x800F1C58).
extern s32 gToolboxStorageRow;

/// Provide a toolbox hook that performs no action.
/// \param unused Unused argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002EB60 (0x8002EB60).
void toolboxNoOp(s32 unused) {
}


/// Orient a toolbox object relative to the current camera.
/// \param object Object whose local rotation is updated.
/// \param pitch Additional pitch in degrees.
/// \param yaw Additional yaw in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002EB68 (0x8002EB68).
void toolboxSetObjectCameraRotation(u8 *object, f32 pitch, f32 yaw) {
    f32 pad;
    f32 sp1C[12];

    matrix34MakeRotationXYZDegrees(sp1C, gPlayerState.pitchOffset + gPlayerState.pitch,
                                   gPlayerState.yaw + gPlayerState.viewShake, 0.0f);
    matrix33ApplyRotationZYXDegrees(sp1C, pitch, yaw, 0.0f);
    matrix33GetFoldedEulerDegrees(sp1C, (f32 *)(object + 0xC), (f32 *)(object + 0x10), (f32 *)(object + 0x14));
}

#include "scene_configuration.h"

/// Set the display scale of a toolbox object.
/// \param handle Handle of the object to scale; missing objects are ignored.
/// \param scale Desired scale relative to the object's normal display size.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002EBEC (0x8002EBEC).
void toolboxSetObjectScale(s32 handle, f32 scale) {
    u8 *liveObject;

    liveObject = objectFindLiveByHandle(handle);
    if (liveObject != NULL) {
        if (*(u16 *)(liveObject + 0x34) == 0x86) {
            itemSetScale(liveObject, scale);
            return;
        }
        *(f32 *)(liveObject + 0) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * scale;
        *(f32 *)(liveObject + 4) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * scale;
        *(f32 *)(liveObject + 8) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * scale;
    }
}

/// Apply the selected-item and unselected-item display scales to the current toolbox row.
/// \param t Initialized toolbox with row zero or one and live handles for its displayed items and placeholders.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002EC74 (0x8002EC74).
void toolboxScaleCurrentRow(ToolboxState *t) {
    ToolboxSlot *s;
    s32 i;
    s32 id;
    ToolboxSlot *row;
    s32 v;

    row = t->slots[t->row];
    for (s = row, i = 0; i < 10; i++, s++) {
        v = s->id;
        if (v == 0) {
            id = ((s32 *)t)[0x32 + i];
        } else {
            id = v;
        }
        if (i == 0) {
            toolboxSetObjectScale(id, 0.5f);
        } else {
            toolboxSetObjectScale(id, 0.3f);
        }
    }
}

#include "texture_tiles.h"
#include "toolbox.h"
/// Remove the objects in the toolbox collection row and clear its entries.
/// \returns Nothing.
/// \pre Every nonzero collection handle identifies a live item not pending destruction.
/// \note Portability: The adjacent loop statements preserve the IDO scheduling required by the ROM.
/// \note Original decompilation identifier: \c func_8002ED3C (0x8002ED3C).
void toolboxClearCollection(void) {
    s32 *slotWords;
    s32 *var_s0;
    s32 *var_s1;
    // clang-format off
    var_s1 = (s32 *)&gToolboxState; var_s0 = &gToolboxItemSlots; do { slotWords = var_s0; var_s1[10] = 0; if (var_s1[11] != 0) { itemDestroy(objectFindLiveByHandle(var_s0[1])); } var_s0 += 2; var_s1 += 2; slotWords[1] = 0; } while (var_s0 != &gToolboxStorageRow);
    // clang-format on
}

/// Hide and park the toolbox and its tracked objects outside the scene.
/// \returns Nothing; requires an initialized toolbox base object.
/// \note Original decompilation identifier: \c func_8002EDBC (0x8002EDBC).
void toolboxHideObjects(void) {
    u8 **objects;
    s32 row;
    s32 i;
    u8 *object;
    Vec3f home;

    home = gToolboxParkingPosition;
    objects = gToolboxState.objects;
    objectSetHierarchyFlags(objects[0], 0x2002);
    *(f32 *)(objects[0] + 0x18) = home.x;
    *(f32 *)(objects[0] + 0x1C) = home.y;
    *(f32 *)(objects[0] + 0x20) = home.z;
    for (row = 0; row < 2; row++) {
        for (i = 0; i < 10; i++) {
            object = objectFindLiveByHandle(gToolboxState.slots[row][i].id);
            if (object != NULL) {
                objectSetHierarchyFlags(object, 0x2002);
                *(f32 *)(object + 0x18) = home.x;
                *(f32 *)(object + 0x1C) = home.y;
                *(f32 *)(object + 0x20) = home.z;
            }
        }
    }
    for (i = 0; i < 10; i++) {
        object = objectFindLiveByHandle(gToolboxState.spareIds[i]);
        if (object != NULL) {
            objectSetHierarchyFlags(object, 0x2002);
            *(f32 *)(object + 0x18) = home.x;
            *(f32 *)(object + 0x1C) = home.y;
            *(f32 *)(object + 0x20) = home.z;
        }
    }
    object = objectFindLiveByHandle(gToolboxBonusHandle);
    if (object != NULL) {
        objectSetHierarchyFlags(object, 0x2002);
        *(f32 *)(object + 0x18) = home.x;
        *(f32 *)(object + 0x1C) = home.y;
        *(f32 *)(object + 0x20) = home.z;
    }
    object = objectFindLiveByHandle(gToolboxPurchaseHandle);
    if (object != NULL) {
        objectSetHierarchyFlags(object, 0x2002);
        *(f32 *)(object + 0x18) = home.x;
        *(f32 *)(object + 0x1C) = home.y;
        *(f32 *)(object + 0x20) = home.z;
    }
}


/// Make the toolbox and its tracked objects eligible for drawing.
/// \returns Nothing; requires an initialized toolbox base object.
/// \note Original decompilation identifier: \c func_8002EF88 (0x8002EF88).
void toolboxShowObjects(void) {
    s32 r;
    s32 i;
    u8 *o;

    objectClearHierarchyFlags(gToolboxState.objects[0], 2);
    for (r = 0; r < 2; r++) {
        for (i = 0; i < 10; i++) {
            o = objectFindLiveByHandle(gToolboxState.slots[r][i].id);
            if (o != 0) {
                objectClearHierarchyFlags(o, 2);
            }
        }
    }
    for (i = 0; i < 10; i++) {
        o = objectFindLiveByHandle(gToolboxState.spareIds[i]);
        if (o != 0) {
            objectClearHierarchyFlags(o, 2);
        }
    }
    o = objectFindLiveByHandle(gToolboxBonusHandle);
    if (o != 0) {
        objectClearHierarchyFlags(o, 2);
    }
    o = objectFindLiveByHandle(gToolboxPurchaseHandle);
    if (o != 0) {
        objectClearHierarchyFlags(o, 2);
    }
}


/// Place an item in a toolbox slot and remove it from active scene interaction.
/// \param slot Writable eight-byte entry receiving the item kind and handle; any prior entry is overwritten without destroying its item.
/// \param object Live initialized item to track in storage; must remain live until removed or explicitly destroyed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002F088 (0x8002F088).
void toolboxStoreObjectInSlot(ToolboxSlot *slot, u8 *object) {
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    TRACE("putItemBoxIntoContent(%p, %p(%s))\n", slot, object,
          itemGetDisplayName(((ItemStatePrefix *)objectState)->kind, 1));
    itemTryRelease(object, 1);
    ((ItemStatePrefix *)objectState)->state = 4;
    itemSetMode(object, ITEM_MODE_STORED);
    ((ItemStatePrefix *)objectState)->flags = (s32)(((ItemStatePrefix *)objectState)->flags & 0xBFFFFFFF);
    objectSetHierarchyFlags(object, 0x2000);
    slot->id = (s32)((RuntimeObjectPrefix *)object)->handle;
    slot->kind = (s32)((ItemStatePrefix *)objectState)->kind;
    itemRemoveFromSearchAndCollision(((RuntimeObjectPrefix *)object)->handle);
}

/// Separate classification-byte view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AB1 (0x80125AB1).
extern s8 gItemKindClassEntries[][0x74];
/// Store an item in a specified toolbox entry and clear duplicate entries for supply item kinds.
/// \param object Address of a live initialized item with a valid catalog kind; existing destination items are not destroyed.
/// \param row Toolbox row index: zero for Collection or one for Item Storage.
/// \param index Destination entry index from zero to nine.
/// \returns Borrowed address of the destination entry in the shared toolbox table.
/// \note Original decompilation identifier: \c func_8002F114 (0x8002F114).
s32 toolboxStoreObjectAt(s32 object, s32 row, s32 index) {
    u8 *data = *(u8 **)(object + 0x54);
    s16 objectKind = *(s16 *)(data + 4);
    s8 kind = gItemKindClassEntries[objectKind][0];
    s32 i;
    ToolboxSlot *slot;
    if (kind == 3 || kind == 2) {
        for (i = 0; i < 10; i++) {
            slot = &gToolboxState.slots[row][i];
            if ((objectKind = *(s16 *)(data + 4)) == slot->kind) {
                slot->kind = 0;
                slot->id = 0;
            }
        }
    }
    slot = &gToolboxState.slots[row][index];
    toolboxStoreObjectInSlot(slot, (u8 *)object);
    return (s32)slot;
}

s32 toolboxStoreObjectAt(s32, s32, s32);

/// Store an item in the first empty entry of a toolbox row.
/// \param object Address of a live initialized item with a valid catalog kind; existing destination items are not destroyed.
/// \param row Toolbox row index: zero for Collection or one for Item Storage.
/// \returns Borrowed address of the destination entry in the shared table, or zero if the row is full.
/// \note Original decompilation identifier: \c func_8002F27C (0x8002F27C).
s32 toolboxStoreObjectInRow(s32 object, s32 row) {
    s32 i;
    ToolboxSlot *s = gToolboxState.slots[row];

    TRACE("putItemIntoBox(%s)\n", itemGetDisplayName(*(s16 *)(*(u8 **)(object + 0x54) + 4), 1));
    for (i = 0; i < 10; i++, s++) {
        if (s->id == 0 && s->kind == 0) {
            return toolboxStoreObjectAt(object, row, i);
        }
    }
    TRACE("itembox full.\n");
    return 0;
}

/// Replace the stored bonus item with a new item, destroying the previous item if present.
/// \param object Address of a live initialized item distinct from the previous bonus item.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002F2F0 (0x8002F2F0).
void toolboxStoreBonusObject(u8 *object) {
    s32 *storedHandle = &gToolboxBonusHandle;
    if (*storedHandle != 0) {
        itemDestroy(objectFindLiveByHandle(*storedHandle));
    }
    TRACE("putBonusItemIntoBox(%s)\n", itemGetDisplayName(*(s16 *)(*(u8 **)(object + 0x54) + 4), 1));
    toolboxStoreObjectInSlot(&gToolboxBonusSlot, (u8 *)object);
}

/// Replace the stored purchase item with a new item, destroying the previous item if present.
/// \param object Address of a live initialized item distinct from the previous purchase item.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002F338 (0x8002F338).
void toolboxStorePurchasedObject(u8 *object) {
    s32 *storedHandle = &gToolboxPurchaseHandle;
    if (*storedHandle != 0) {
        itemDestroy(objectFindLiveByHandle(*storedHandle));
    }
    TRACE("putPurchaseItemIntoBox(%s)\n", itemGetDisplayName(*(s16 *)(*(u8 **)(object + 0x54) + 4), 1));
    toolboxStoreObjectInSlot(&gToolboxPurchaseSlot, (u8 *)object);
}


/// Record acquisition of a supply item in persistent progress.
/// \param itemKind Valid item catalog kind whose acquisition is being recorded.
/// \returns One if the kind has a supply-item progress flag to set, or zero otherwise.
/// \note Original decompilation identifier: \c func_8002F380 (0x8002F380).
s32 toolboxRecordSupplyItem(s32 itemKind) {
    s16 availabilityFlag;
    s8 supplyCategory;
    ItemKind *itemRecord;

    itemRecord = &gItemKinds[itemKind];
    supplyCategory = (s8)itemRecord->itemClass;
    if ((supplyCategory != 3) && (supplyCategory != 2)) {
        return 0;
    }
    availabilityFlag = itemRecord->availabilityField;
    if (availabilityFlag == -1) {
        return 0;
    }
    TRACE("keeping supply item, %s\n", itemGetDisplayName(itemKind, 1));
    progressSetValue(availabilityFlag, 1);
    return 1;
}


/// Remove an eligible item from the toolbox and restore it to active scene interaction.
/// \param row Toolbox row index: zero for Collection or one for Item Storage.
/// \param col Entry index from zero to nine.
/// \returns Borrowed live item object, or null if the entry is empty or its item cannot be removed.
/// \pre Each occupied entry identifies a live item with a valid catalog kind.
/// \note Original decompilation identifier: \c func_8002F3FC (0x8002F3FC).
u8 *toolboxTakeObject(s32 row, s32 col) {
    ToolboxSlot *slot;
    s8 kind;
    u8 *obj;
    u8 *ext;

    slot = &gToolboxState.slots[row][col];
    if (slot->id == 0) {
        return NULL;
    }
    kind = ((s8)gItemKinds[slot->kind].itemClass);
    if (kind == 5) {
        return NULL;
    }
    if (kind == 2) {
        return NULL;
    }
    obj = objectFindLiveByHandle(slot->id);
    ext = *(u8 **)(obj + 0x54);
    *(s8 *)(ext + 6) = 0;
    *(s32 *)(ext + 8) = 0;
    itemSetMode(obj, ITEM_MODE_GROUND_AND_AIR);
    *(s32 *)ext &= 0xBFFFFFFF;
    objectClearHierarchyFlags(obj, 0x2000);
    itemRegisterForSearch(*(s32 *)(obj + 0x38));
    slot->kind = 0;
    slot->id = 0;
    return obj;
}

void toolboxScaleCurrentRow(ToolboxState *);

/// Transfer an eligible toolbox item into the player's held-item selection.
/// \param player Player state receiving the item as its held object.
/// \param row Toolbox row index: zero for Collection or one for Item Storage.
/// \param index Entry index from zero to nine.
/// \returns Borrowed transferred item object, or null if the entry is empty or cannot be removed.
/// \note Original decompilation identifier: \c func_8002F500 (0x8002F500).
u8 *toolboxGiveObjectToPlayer(u8 *player, s32 row, s32 index) {
    u8 *sp24;
    u8 *itemObject;
    u8 *itemState;

    itemObject = toolboxTakeObject(row, index);
    if (itemObject == NULL) {
        return NULL;
    }
    itemState = ((RuntimeObjectPrefix *)itemObject)->state.bytes;
    sp24 = itemState;
    objectSetHierarchyFlags(itemObject, 0x2000);
    ((ItemStatePrefix *)itemState)->state = 1;
    ((ItemStatePrefix *)itemState)->owner = 0;
    itemSetMode(itemObject, ITEM_MODE_PLAYER_HELD);
    ((ItemStatePrefix *)itemState)->flags = (s32)(((ItemStatePrefix *)itemState)->flags | 0x40000000);
    ((PlayerControlPrefix *)player)->heldItemHandle = (s32)((RuntimeObjectPrefix *)itemObject)->handle;
    toolboxScaleCurrentRow(&gToolboxState);
    return itemObject;
}

/// Elapsed-time checkpoint controlling the delay between toolbox help messages.
/// \note Original identifier: \c D_800F1D20 (0x800F1D20).
extern s32 gToolboxHelpMessageMilliseconds;

/// Show a toolbox message containing an item's name and restart the help-message delay.
/// \param object Item whose display name is included in the message.
/// \param fmt Message format accepting one item-name string; formatted text must fit 128 bytes including its terminator.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002F5A0 (0x8002F5A0).
void toolboxShowItemMessage(u8 *object, char *fmt) {
    s16 *p;
    char buf[128];

    p = *(s16 **)(object + 0x54);
    sprintf(buf, fmt, itemGetDisplayName(p[2], 1));
    messageBarEnqueue(0, 3.5f, (u8 *)buf);
    gToolboxHelpMessageMilliseconds = runtimeGetElapsedMilliseconds();
}


/// Check whether an item's category belongs in a toolbox row.
/// \param object Item object to check.
/// \param row Toolbox row index: zero for Collection or one for Item Storage.
/// \returns One if the item category is accepted by the row, or zero otherwise.
/// \note Original decompilation identifier: \c func_8002F5F8 (0x8002F5F8).
s32 toolboxCanStoreObjectInRow(u8 *object, s32 row) {
    u8 *p = *(u8 **)(object + 0x54);
    s8 t = gItemKindClassEntries[*(s16 *)(p + 4)][0];

    if (t == 1) {
        if (row == 0) {
            return 1;
        }
        return 0;
    }
    if (t == 3 || t == 2 || t == 5) {
        if (row == 1) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/// Enter a toolbox mode and restart its transition timer.
/// \param table Initialized toolbox whose mode and millisecond transition timer are updated.
/// \param mode Toolbox mode: zero enters, one opens a row, two interacts, three closes a row, four exits, or five shows a description.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002F680 (0x8002F680).
void toolboxSetMode(u8 *table, s32 mode) {
    *(s32 *)(table + 0x18) = mode;
    *(s64 *)(table + 0x20) = runtimeGetElapsedMilliseconds();
}

/// Separate view of the selected Collection or Item Storage row.
/// \note Original identifier: \c D_800F1BF4 (0x800F1BF4).
extern s32 gToolboxSelectedRow;
/// Requested action on toolbox exit: close, change row or open the system menu.
/// \note Original identifier: \c D_800F1D18 (0x800F1D18).
extern s32 gToolboxExitAction;
/// Separate handle-field view of toolbox item slots; each slot has an eight-byte stride.
/// \note Original identifier: \c D_800F1C0C (0x800F1C0C).
extern s32 gToolboxItemHandleEntries[][20];
/// Confirm the selected toolbox item, exchanging held and stored items or opening its menu when appropriate.
/// \param player Player state whose held item participates in the confirmed operation.
/// \returns Nothing; rejected transfers display feedback and leave the items in place.
/// \note Original decompilation identifier: \c func_8002F6B0 (0x8002F6B0).
void toolboxConfirmItemTransfer(u8 *player) {
    ToolboxSlot *slot = gToolboxState.slots[gToolboxState.row];
    s8 kind;
    u8 *other;
    u8 *held;
    u8 *taken;
    s32 i;
    u8 *data;
    u8 *otherState;
    if (objectFindLiveByHandle(slot->id) != NULL) {
    }
    kind = gItemKindClassEntries[slot->kind][0];
    switch (kind) {
    case 5:
        playerPlayTrackedSoundEffect(player, 0xC5);
        gToolboxExitAction = 2;
        messageBarEnqueue(0, 0.1f, (u8 *)" ");
        toolboxSetMode((u8 *)&gToolboxState, 3);
        return;
    case 2:
        playerPlayTrackedSoundEffect(player, 0x92);
        toolboxShowItemMessage(objectFindLiveByHandle(slot->id), "You can't take out the ^4%s^^.");
        return;
    }
    held = objectFindLiveByHandle(*(s32 *)(player + 0x84));
    if (held != NULL) {
        if (toolboxCanStoreObjectInRow(held, gToolboxSelectedRow) == 0) {
            playerPlayTrackedSoundEffect(player, 0x92);
            if (gToolboxSelectedRow == 1) {
                toolboxShowItemMessage(held, "Put the ^4%s^^ in the Collection.");
                return;
            }
            toolboxShowItemMessage(held, "Put the ^4%s^^ in the Item Storage.");
            return;
        }
        i = 0;
        if (gToolboxSelectedRow == 1) {
            data = *(u8 **)(held + 0x54);
            do {
                other = objectFindLiveByHandle(
                    *(s32 *)((u8 *)gToolboxItemHandleEntries + gToolboxState.row * 0x50 + i * 8));
                if (other != NULL) {
                    otherState = *(u8 **)(other + 0x54);
                    if (*(s16 *)(data + 4) == *(s16 *)(otherState + 4)) {
                        playerPlayTrackedSoundEffect(player, 0x92);
                        toolboxShowItemMessage(held, "You already have the ^4%s^^.");
                        return;
                    }
                }
                i++;
            } while (i != 10);
        }
        itemTryRelease(held, 1);
    }
    taken = toolboxGiveObjectToPlayer(player, gToolboxSelectedRow, 0);
    if (held != NULL)
        toolboxStoreObjectAt((s32)held, gToolboxSelectedRow, 0);
    if (taken != NULL && held != NULL) {
        playerPlayTrackedSoundEffect(player, 0xC5);
        toolboxShowItemMessage(held, "You stored the ^4%s^^.");
        toolboxShowItemMessage(taken, "Instead, you took out the ^4%s^^.");
        return;
    }
    if (taken != NULL) {
        playerPlayTrackedSoundEffect(player, 0xC5);
        toolboxShowItemMessage(taken, "You took out the ^4%s^^.");
        return;
    }
    if (held != NULL) {
        playerPlayTrackedSoundEffect(player, 0xC5);
        toolboxShowItemMessage(held, "You stored the ^4%s^^.");
        return;
    }
    playerPlayTrackedSoundEffect(player, 0x92);
    messageBarEnqueue(0, 2.0f, (u8 *)"There isn't anything there.");
}

/// Reads a saved progress field for toolbox supply restoration.
/// \param field Saved progress-field selector.
/// \returns The current field value.
/// \note Matching: the signed selector declaration preserves IDO code generation;
/// the shared unsigned declaration changes the ROM bytes in this caller.
/// \note Original decompilation identifier: \c __osSpSetPc (0x80032C1C).
s32 progressGetValue(s32 field);
/// Recreate stored toolbox items and populate available supplies from story progress.
/// \param table Address of the shared toolbox table containing valid saved item kinds and cleared object handles; current scene and item resources must be initialized.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002F9A4 (0x8002F9A4).
void toolboxRestoreStoredObjects(u8 *table) {
    u8 *slot;
    s32 i;
    u8 *object;
    s32 offset;

    toolboxNoOp((s32)table);
    for (i = 0, offset = 0; i < 10; offset += 8, i++) {
        slot = table + offset;
        if (gToolboxDefaultStoredItems[i].flag == -1 || progressGetValue(gToolboxDefaultStoredItems[i].flag) != 0) {
            toolboxStoreObjectAt(
                (s32)itemCreate(1000.0f, 1000.0f, 1000.0f, 0.0f, 0.0f, gToolboxDefaultStoredItems[i].kind), 1, i);
        } else {
            *(s32 *)(slot + 0x7C) = 0;
            *(s32 *)(slot + 0x78) = 0;
        }
    }
    for (i = 0, offset = 0; i < 10; offset += 8, i++) {
        slot = table + offset;
        if (*(u32 *)(slot + 0x28) != 0) {
            toolboxStoreObjectAt((s32)itemCreate(1000.0f, 1000.0f, 1000.0f, 0.0f, 0.0f, *(s32 *)(slot + 0x28)), 0, i);
        }
    }
    slot = table + 0xF0;
    if (*(s32 *)slot != 0) {
        object = itemCreate(1000.0f, 1000.0f, 1000.0f, 0.0f, 0.0f, *(s32 *)slot);
        *(s32 *)slot = 0;
        toolboxStoreBonusObject(object);
    }
    slot = table + 0xF8;
    if (*(s32 *)slot != 0) {
        object = itemCreate(1000.0f, 1000.0f, 1000.0f, 0.0f, 0.0f, *(s32 *)slot);
        *(s32 *)slot = 0;
        toolboxStorePurchasedObject(object);
    }
    toolboxNoOp((s32)table);
}

void toolboxRestoreStoredObjects(u8 *);
/// Owned text images for the two toolbox row titles.
/// \note Original identifier: \c D_800F1D28 (0x800F1D28).
extern TextImage *gToolboxRowTitleImages[];
/// Separate view of the toolbox linked display part stored in the third object slot.
/// \note Original identifier: \c D_800F1BE8 (0x800F1BE8).
extern u8 *gToolboxLinkedDisplayPart;
/// Create the toolbox display, empty-slot markers, row titles, and stored item objects.
/// \returns Nothing; requires the saved toolbox contents and scene resources.
/// \note Original decompilation identifier: \c func_8002FB64 (0x8002FB64).
void toolboxCreateDisplayObjects(void) {
    u8 *object;
    s32 row;
    s32 i;
    u8 **objects = &gToolboxState.objects[1];

    object = objectCreateHierarchyFromTemplate(gToolboxModelTemplate);
    objectSetHierarchyFlags(object, 0x2000);
    objectRegisterActiveHierarchy(object);
    *(f32 *)(object + 0) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC);
    *(f32 *)(object + 4) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC);
    *(f32 *)(object + 8) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC);
    gToolboxState.objects[0] = object;
    *objects = *(u8 **)(*(u8 **)(object + 0x2C) + 0x28);
    gToolboxLinkedDisplayPart = *(u8 **)(*objects + 0x28);
    for (row = 0; row < 2; row++) {
        for (i = 0; i < 10; i++) {
            TRACE("contents[%d][%d].objID = %d\n", row, i, gToolboxState.slots[row][i].id);
            gToolboxState.slots[row][i].id = 0;
        }
    }
    gToolboxBonusHandle = 0;
    gToolboxPurchaseHandle = 0;
    for (i = 0; i < 10; i++) {
        object = objectCreateHierarchyFromTemplate(gToolboxPlaceholderTemplate);
        objectSetHierarchyFlags(object, 0x2000);
        objectRegisterActiveHierarchy(object);
        *(f32 *)(object + 0xC) = 0.0f;
        *(f32 *)(object + 0x10) = 0.0f;
        *(f32 *)(object + 0x14) = 0.0f;
        *(f32 *)(object + 0x18) = 0.0f;
        *(f32 *)(object + 0x1C) = 0.0f;
        *(f32 *)(object + 0x20) = 0.0f;
        *(f32 *)(object + 0) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * 0.3f;
        *(f32 *)(object + 4) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * 0.3f;
        *(f32 *)(object + 8) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * 0.3f;
        gToolboxState.spareIds[i] = *(s32 *)(object + 0x38);
    }
    for (i = 0; i < 2; i++) {
        gToolboxRowTitleImages[i] = 0;
        textImageCreateFromText((TextImage **)(&gToolboxRowTitleImages[i]), (u8 *)(gToolboxRowTitles[i]),
                                gTextImageConfigurations);
    }
    toolboxRestoreStoredObjects((u8 *)&gToolboxState);
}

void _bzero(void *, s32);
/// Initialize empty toolbox contents with the default Item Storage selection and initial supply item.
/// \returns Nothing; existing display objects and items are not released.
/// \note Original decompilation identifier: \c func_8002FD60 (0x8002FD60).
void toolboxReset(void) {
    s32 row;
    s32 i;
    ToolboxSlot *singleton = gToolboxState.singletons;
    _bzero(&gToolboxState, sizeof(gToolboxState));
    gToolboxSelectedRow = 1;
    gToolboxExitAction = 0;
    for (row = 0; row < 2; row++) {
        for (i = 0; i < 10; i++) {
            gToolboxState.slots[row][i].id = 0;
            gToolboxState.slots[row][i].kind = 0;
        }
    }
    singleton->id = 0;
    singleton->kind = 0;
    singleton[1].id = 0;
    singleton[1].kind = 0;
    gToolboxStorageRow = 0xA4;
}

/// Restore the toolbox display and current row to their initial orientation.
/// \param t Initialized toolbox with row zero or one, linked display parts, and live handles for occupied entries.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002FE24 (0x8002FE24).
void toolboxResetRowRotation(ToolboxState *t) {
    u8 *o;
    s32 i;

    *(f32 *)(t->objects[1] + 0xC) = 0.0f;
    *(f32 *)(t->objects[1] + 0x10) = 0.0f;
    *(f32 *)(t->objects[1] + 0x14) = 0.0f;
    *(f32 *)(t->objects[2] + 0x18) = 0.0f;
    *(f32 *)(t->objects[2] + 0x1C) = 0.0f;
    *(f32 *)(t->objects[2] + 0x20) = 0.0f;
    t->spin = 0;
    t->rotation = 180.0f;
    for (i = 0; i < 10; i++) {
        if (t->slots[t->row][i].id != 0) {
            o = objectFindLiveByHandle(t->slots[t->row][i].id);
            *(f32 *)(o + 0xC) = 0.0f;
            *(f32 *)(o + 0x10) = 0.0f;
            *(f32 *)(o + 0x14) = 0.0f;
        }
    }
}

#define VEC3_SET(v, xv, yv, zv) ((v).x = (xv), (v).y = (yv), (v).z = (zv))

/// Separate view of the toolbox item-row rotation angle.
/// \note Original identifier: \c D_800F1BF0 (0x800F1BF0).
extern f32 gToolboxRowRotation;
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
/// Separate symbol view of the primary player record's held item handle.
/// \note Original identifier: \c D_800EF724 (0x800EF724).
extern s32 gPlayerHeldItemHandle;
/// Camera-derived transform used to draw toolbox interface elements.
/// \note Original identifier: \c D_800F1CE8 (0x800F1CE8).
extern Matrix34 gToolboxInterfaceTransform;
/// Position and orient the displayed toolbox items and the player's held item.
/// \param angle Rotation of the item carousel in degrees.
/// \param height Vertical offset of the carousel in toolbox presentation coordinates.
/// \param depth Depth offset of the carousel in toolbox presentation coordinates.
/// \returns Nothing.
/// \pre The shared toolbox has row zero or one, live display items/placeholders, and an initialized presentation transform.
/// \note Portability: The VEC3_SET expression preserves the IDO scheduling required by the ROM.
/// \note Original decompilation identifier: \c func_8002FEEC (0x8002FEEC).
void toolboxLayoutCurrentRow(f32 angle, f32 height, f32 depth) {
    ToolboxSlot *slot;
    s32 i;
    u8 *object;
    f32 direction;
    f32 rotation;
    f32 *spin = &gToolboxRowRotation;
    Vec3f position;
    {
        u8 *row = (u8 *)&gToolboxState + gToolboxState.row * 0x50;
        slot = (ToolboxSlot *)(row + 0x28);
    }
    for (i = 0; i < 10; i++) {
        if (slot->id != 0)
            object = objectFindLiveByHandle(slot->id);
        else
            object = objectFindLiveByHandle(gToolboxState.spareIds[i]);
        direction = (i * 36.0f + angle) - 90.0f;
        position.x = 2.0f * mathCosDegrees(direction);
        position.y = mathSinDegrees(direction) * 0.5f + height;
        position.z = (-6.0f + depth) - mathSinDegrees(direction) * 1.5f;
        matrix34TransformPointVector((f32 *)&gToolboxInterfaceTransform, (f32 *)(&position), (f32 *)(object + 0x18));
        if (i != 0 || *(u16 *)(object + 0x34) != 0x86) {
            toolboxSetObjectCameraRotation(object, -5.0f, 180.0f);
        } else {
            rotation = *spin;
            rotation += gRuntimeFrameSeconds * 100.0f;
            while (rotation < 0.0f)
                rotation += 360.0f;
            while (rotation >= 360.0f)
                rotation -= 360.0f;
            *spin = rotation;
            toolboxSetObjectCameraRotation(object, -5.0f, rotation);
        }
        objectUpdateHierarchy((RuntimeObjectPrefix *)object);
        slot++;
    }
    if (gPlayerHeldItemHandle != 0) {
        object = objectFindLiveByHandle(gPlayerHeldItemHandle);
        if (object != NULL) {
            VEC3_SET(position, 0, -1.5f, -4.0f);
            matrix34TransformPointVector((f32 *)&gToolboxInterfaceTransform, (f32 *)(&position), (f32 *)(object + 0x18));
            *(f32 *)(object + 0xC) = 0.0f;
            *(f32 *)(object + 0x10) = 0.0f;
            *(f32 *)(object + 0x14) = 0.0f;
            objectUpdateHierarchy((RuntimeObjectPrefix *)object);
        }
    }
}


/// Position and orient the toolbox display for its current row.
/// \param t Initialized toolbox with row zero or one and a live base object.
/// \param dy Vertical offset from the current row's presentation anchor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800301B0 (0x800301B0).
void toolboxPlaceBaseForRow(ToolboxState *t, f32 dy) {
    Vec3f v;

    v.x = gToolboxRowAnchors[t->row].x;
    v.y = gToolboxRowAnchors[t->row].y;
    v.z = gToolboxRowAnchors[t->row].z;
    v.y += dy;
    matrix34TransformPointVector((f32 *)&gToolboxInterfaceTransform, (f32 *)(&v), (f32 *)(t->objects[0] + 0x18));
    if (t->row == 0) {
        toolboxSetObjectCameraRotation(t->objects[0], -5.0f, 210.0f);
    } else {
        toolboxSetObjectCameraRotation(t->objects[0], -5.0f, 180.0f);
    }
    objectUpdateHierarchy((RuntimeObjectPrefix *)t->objects[0]);
}

/// Show the next toolbox help message when the help delay has elapsed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003028C (0x8003028C).
void toolboxUpdateHelpMessage(void) {
    s32 *cur = &gToolboxNextHelpMessage;
    s32 v;

    if ((u64)runtimeGetElapsedMilliseconds() - (u32)gToolboxHelpMessageMilliseconds >= 0xBB9) {
        v = *cur;
        messageBarEnqueue(0, 3.5f, (u8 *)gToolboxHelpMessages[v]);
        v = *cur + 1;
        while (v < 0) {
            v += 6;
        }
        *cur = v;
        if (v >= 6) {
            do {
                v -= 6;
            } while (v >= 6);
            *cur = v;
        }
        gToolboxHelpMessageMilliseconds = runtimeGetElapsedMilliseconds();
    }
}

/// Draw the current toolbox row title and its translucent background.
/// \param address Address of the display-list cursor to append to and advance.
/// \returns Nothing; requires an initialized row-title image.
/// \note Original decompilation identifier: \c func_80030350 (0x80030350).
void toolboxDrawRowTitle(s32 address) {
    Gfx **gdl = (Gfx **)address;
    s32 row = gToolboxSelectedRow;
    s32 width = textMeasureWithFont((u8 *)gToolboxRowTitles[row], gTextImageConfigurations, 1);
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_MODULATEI_PRIM, G_CC_MODULATEI_PRIM);
    gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 200);
    gDPFillRectangle((*gdl)++, 31, 189, width + 40, 207);
    textDrawBegin(*gdl);
    textDrawSetFont(0);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
    textDrawSetSpacingMode(1);
    textDrawSetScale(1.0f, 1.0f);
    textDrawSetArea(0, 0, 319, 239);
    textDrawSetColor(255, 255, 255, 254);
    textDrawSetCursor(36, 190);
    textDrawImageAtCursor((TextImage *)gToolboxRowTitleImages[row]);
    textDrawGetDisplayListCursor(gdl);
}

/// Segmented shared display list preparing textured screen rectangle drawing.
/// \note Original identifier: \c D_100C8C8 (0x100C8C8).
extern Gfx gTextureRectangleSetupDisplayList[];
/// Draw a scaled toolbox interface icon at a screen position.
/// \param gdl Display-list cursor to append to and advance.
/// \param texture Icon texture index in the toolbox interface texture set.
/// \param x Left screen coordinate in pixels.
/// \param y Top screen coordinate in pixels.
/// \param width Unscaled icon width in pixels.
/// \param height Unscaled icon height in pixels.
/// \param scaleX Nonzero horizontal icon scale.
/// \param scaleY Nonzero vertical icon scale.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800304D8 (0x800304D8).
void toolboxDrawIcon(Gfx **gdl, s32 texture, s32 x, s32 y, s32 width, s32 height, f32 scaleX, f32 scaleY) {
    TextureDescriptor t;
    textureGetDescriptor(5, texture, &t);
    gDPPipeSync((*gdl)++);
    if (t.format == G_IM_FMT_CI) {
        gDPSetTextureLUT((*gdl)++, G_TT_RGBA16);
    } else {
        gDPSetTextureLUT((*gdl)++, G_TT_NONE);
    }
    gSPDisplayList((*gdl)++, gTextureRectangleSetupDisplayList);
    textureUploadTile(gdl, t.image, t.palette, t.format, t.size, t.width, t.height, 0, 0, 1, 1);
    gSPTextureRectangle((*gdl)++, x * 4, y * 4, (s32)((x + (width - 1) * scaleX) * 4.0f),
                        (s32)((y + (height - 1) * scaleY) * 4.0f), 0, 0, -32,
                        (s32)(1024.0f / scaleX >= 0.0f ? 1024.0f / scaleX + 0.5f : 1024.0f / scaleX - 0.5f),
                        (s32)(-1024.0f / scaleY >= 0.0f ? -1024.0f / scaleY + 0.5f : -1024.0f / scaleY - 0.5f));
    gSPTexture((*gdl)++, 0x7FFF, 0, 0, 0, G_OFF);
    gMoveWd((*gdl)++, G_MW_FOG, 0x10, 0x7FFF);
}

/// Separate view of the elapsed-time checkpoint for toolbox transitions.
/// \note Original identifier: \c D_800F1C00 (0x800F1C00).
extern u64 gToolboxTransitionMilliseconds;
/// Separate view of the toolbox display transition mode.
/// \note Original identifier: \c D_800F1BF8 (0x800F1BF8).
extern u32 gToolboxMode;
/// Separate view of the selected Collection or Item Storage row.
/// \note Original identifier: \c D_800F1BF4 (0x800F1BF4).
/// Separate view of the signed item-row rotation step.
/// \note Original identifier: \c D_800F1BEC (0x800F1BEC).
/// Row requested when reopening the toolbox after a transition.
/// \note Original identifier: \c D_800F1D1C (0x800F1D1C).
/// Requested action on toolbox exit: close, change row or open the system menu.
/// \note Original identifier: \c D_800F1D18 (0x800F1D18).
extern s32 gToolboxRotationStep, gToolboxSelectedRow, gToolboxExitAction, gToolboxRequestedRow;
/// Draw the toolbox navigation arrows and eligible selected-item marker with transition feedback.
/// \param address Address of the display-list cursor to append to and advance.
/// \returns Nothing.
/// \pre Toolbox row and transition state, interface textures, and the global graphics cursor are initialized.
/// \note Portability: The selected-arrow blocks preserve the IDO scheduling required by the ROM.
/// \note Original decompilation identifier: \c func_8003077C (0x8003077C).
void toolboxDrawNavigation(s32 address) {
    Gfx **gdl = (Gfx **)address;
    u8 *object;
    s32 id;
    f32 scaleO;
    s32 sizeO, halfO;
    s32 xO, yO;
    f32 elapsed;
    s32 *row = &gToolboxSelectedRow;
    u64 now;
    f32 scaleL;
    s32 sizeL, xL, yL;
    f32 scaleR;
    s32 sizeR, xR, yR;
    f32 scaleU;
    s32 sizeU, halfU, xU, yU;
    f32 scaleD;
    s32 sizeD, halfD, xD, yD;
    f64 phase, distance;

    gDPPipeSync((*gdl)++);
    gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    now = runtimeGetElapsedMilliseconds();
    elapsed = now - gToolboxTransitionMilliseconds;
    phase = elapsed / 200.0f;
    distance = phase >= 0.5 ? phase - 0.5 : -(phase - 0.5);
    if (gToolboxRotationStep == 1) {
        scaleL = (0.5 - distance) + 1.0;
        sizeL = (s32)(32.0f * scaleL);
        xL = 52 - sizeL / 2;
        do {
            yL = 128 - sizeL / 2;
            gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 255);
            toolboxDrawIcon(gdl, 245, xL, yL, 32, 32, scaleL, scaleL);
        } while (0);
    } else {
        gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 128);
        toolboxDrawIcon(gdl, 245, 36, 112, 32, 32, 1.0f, 1.0f);
    }
    if (gToolboxRotationStep == -1) {
        scaleR = (0.5 - distance) + 1.0;
        sizeR = (s32)(32 * scaleR);
        xR = 268 - sizeR / 2;
        do {
            yR = 128 - sizeR / 2;
            gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 255);
            toolboxDrawIcon(gdl, 246, xR, yR, 32, 32, scaleR, scaleR);
        } while (0);
    } else {
        gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 128);
        toolboxDrawIcon(gdl, 246, 252, 112, 32, 32, 1.0f, 1.0f);
    }
    if (*row == 1) {
        if (gToolboxMode == 3 && gToolboxExitAction != 2 && *row != gToolboxRequestedRow) {
            phase = elapsed / 300.0f;
            distance = phase >= 0.5 ? phase - 0.5 : -(phase - 0.5);
            scaleU = (0.5 - distance) + 1.0f;
            gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 255);
            sizeU = (s32)(32.0f * scaleU);
            halfU = sizeU / 2;
            xU = 160 - halfU;
            yU = 68 - halfU;
            toolboxDrawIcon(gdl, 247, xU, yU, 32, 32, scaleU, scaleU);
        } else {
            gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 128);
            toolboxDrawIcon(gdl, 247, 144, 52, 32, 32, 1.0f, 1.0f);
        }
    }
    if (*row == 0) {
        if (gToolboxMode == 3 && *row != gToolboxRequestedRow) {
            phase = elapsed / 300.0f;
            distance = phase >= 0.5 ? phase - 0.5 : -(phase - 0.5);
            scaleD = (0.5 - distance) + 1.0f;
            gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 255);
            sizeD = (s32)(32.0f * scaleD);
            halfD = sizeD / 2;
            xD = 160 - halfD;
            yD = 196 - halfD;
            toolboxDrawIcon(gdl, 244, xD, yD, 32, 32, scaleD, scaleD);
        } else {
            gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 128);
            toolboxDrawIcon(gdl, 244, 144, 180, 32, 32, 1.0f, 1.0f);
        }
    }
    if (gToolboxRotationStep == 0) {
        object = (u8 *)objectFindLiveByHandle(gToolboxState.slots[gToolboxSelectedRow][0].id);
        if (object != NULL) {
            if (*row == 0) {
                scaleO = (f64) * (f32 *)(object + 0x24) * 4.0;
                if ((f64)scaleO < 1.0)
                    scaleO = 1.0f;
                else if ((f64)scaleO > 4.0)
                    scaleO = 4.0f;
                gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 200);
            } else {
                id = gToolboxState.slots[gToolboxSelectedRow][0].kind;
                if (gItemKindClassEntries[id][0] == 2 || gItemKindClassEntries[id][0] == 5)
                    return;
                scaleO = 2.0f;
                gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 150, 200);
            }
            gToolboxNavigationSprites[0].width = 32;
            gToolboxNavigationSprites[0].height = 32;
            gToolboxNavigationSprites[0].scaleX = scaleO;
            gToolboxNavigationSprites[0].scaleY = scaleO;
            sizeO = (s32)(32.0f * scaleO);
            halfO = sizeO / 2;
            xO = 160 - halfO;
            yO = 150 - halfO;
            gToolboxNavigationSprites[0].x = xO;
            gToolboxNavigationSprites[0].y = yO;
            textureDrawRectangles(5, (TextureRectangle *)gToolboxNavigationSprites, gdl);
        }
    }
}

/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
/// Initial draw count used to defer toolbox rendering until its display is ready.
/// \note Original identifier: \c D_800F1CE0 (0x800F1CE0).
extern s32 gToolboxOpeningDrawCount;
void toolboxDrawRowTitle(s32);
void toolboxDrawNavigation(s32);

/// Draw the toolbox scene, displayed items, and the current interface or item description.
/// \param address Address of the display-list cursor to append to and advance.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_80030F68 (0x80030F68).
void toolboxDrawFrame(s32 address) {
    ToolboxSlot *row;
    s32 i;
    u8 **hdr = gToolboxState.objects;
    u8 *obj;

    objectDrawGroup(0);
    renderDrawColorOverlayWithDepth(gToolboxBackdropColor[0], gToolboxBackdropColor[1], gToolboxBackdropColor[2],
                                    gToolboxBackdropColor[3]);
    cameraUpdateOverlayColors(gRenderCameraIndex, 0);
    cameraDrawOverlay(gRenderCameraIndex);
    cameraDrawSecondaryOverlay(gRenderCameraIndex);
    if (gToolboxOpeningDrawCount < 2) {
        gToolboxOpeningDrawCount++;
        return;
    }
    row = gToolboxState.slots[gToolboxSelectedRow];
    objectDrawHierarchyGroup((RuntimeObjectPrefix *)(s32)hdr[0], 0x2000);
    toolboxUpdateHelpMessage();
    for (i = 0; i < 10; i++) {
        if (row[i].id == 0) {
            obj = objectFindLiveByHandle(gToolboxState.spareIds[i]);
        } else {
            obj = objectFindLiveByHandle(row[i].id);
        }
        if (obj != 0) {
            objectDrawHierarchyGroup((RuntimeObjectPrefix *)(s32)obj, 0x2000);
        }
    }
    if (gPlayerHeldItemHandle != 0) {
        obj = objectFindLiveByHandle(gPlayerHeldItemHandle);
        if (obj != 0) {
            objectDrawHierarchyGroup((RuntimeObjectPrefix *)(s32)obj, 0x2000);
        }
    }
    if (gToolboxMode == 5) {
        textWindowDrawEmbedded();
    } else {
        toolboxDrawRowTitle(address);
        toolboxDrawNavigation(address);
    }
}

/// Row requested when reopening the toolbox after a transition.
/// \note Original identifier: \c D_800F1D1C (0x800F1D1C).
extern s32 gToolboxRequestedRow;
/// Open the toolbox display and prepare its selected row and held item.
/// \param row Row to open: zero for Collection, one for Item Storage, or minus one to retain the current row.
/// \param useHeld Nonzero to choose the row from the held item when an item is present, overriding the requested row.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_800310F8 (0x800310F8).
void toolboxOpen(s32 row, s32 useHeld) {
    u64 now;
    u8 *held;
    u8 *state;
    ToolboxSlot *r;
    ToolboxState *table;
    s32 i;

    controllerRequestRumbleReset(0);
    runtimeRequestGameModeChange(1);
    held = (u8 *)objectFindLiveByHandle(gPlayerHeldItemHandle);
    if (held != NULL) {
        state = *(u8 **)(held + 0x54);
        if (useHeld) {
            if (gItemKindClassEntries[*(s16 *)(state + 4)][0] == 1) {
                row = 0;
            } else {
                row = 1;
            }
        }
    }
    table = &gToolboxState;
    if (row != -1) {
        table->row = row;
    }
    toolboxSetMode((u8 *)table, 0);
    toolboxResetRowRotation(table);
    toolboxScaleCurrentRow(table);
    gToolboxOpeningDrawCount = 0;
    gToolboxRequestedRow = table->row;
    r = table->slots[table->row];
    for (i = 0; i < 10; i++) {
        objectFindLiveByHandle(r[i].id);
    }
    if (held != NULL) {
        objectSetHierarchyFlags(held, 0x2000);
        toolboxSetObjectScale(gPlayerHeldItemHandle, 0.5f);
    }
    toolboxShowObjects();
    controllerConfigureStickButtons(1, 0x1E, 0x1E, 0x1E, 0x1E);
    now = runtimeGetElapsedMilliseconds();
    gToolboxHelpMessageMilliseconds = now - 0xBB8;
}

void toolboxHideObjects(void);
void systemMenuEnter(void);

/// Finish closing the toolbox and continue the pending return, row change, or item-menu action.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_8003126C (0x8003126C).
void toolboxFinishClosing(void) {
    ToolboxSlot *row = gToolboxState.slots[gToolboxState.row];
    s32 i;
    s32 *held = &gPlayerHeldItemHandle;

    for (i = 0; i < 10; i++) {
        objectFindLiveByHandle(row[i].id);
    }
    if (*held != 0) {
        objectClearHierarchyFlags(objectFindLiveByHandle(*held), 0x2000);
        toolboxSetObjectScale(*held, 1.0f);
    }
    messageBarClear();
    toolboxHideObjects();
    controllerConfigureStickButtons(0, 0x1E, 0x1E, 0x1E, 0x1E);
    switch (gToolboxExitAction) {
    case 0:
        runtimeRequestGameModeChange(0);
        break;
    case 1:
        toolboxOpen(gToolboxRequestedRow, 0);
        break;
    case 2:
        systemMenuEnter();
        gToolboxExitAction = 0;
        break;
    default:
        TRACE("ERROR: Cannot change mode to %d\n", gToolboxExitAction);
        break;
    }
}

void toolboxConfirmItemTransfer(u8 *);
/// Primary controller new and auto-repeated button presses; aliases the repeated word of the processed sample.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
extern s32 gControllerPrimaryRepeatedButtons;
/// Separate view of the signed item-row rotation step.
/// \note Original identifier: \c D_800F1BEC (0x800F1BEC).
extern s32 gToolboxRotationStep;
/// Separate view of the ten toolbox placeholder object handles.
/// \note Original identifier: \c D_800F1CA8 (0x800F1CA8).
extern s32 gToolboxPlaceholderHandles;

/* Item kind and its borrowed description text. */
typedef struct {
    char *text;
    s32 id;
} DescriptionSelection313;

/// Handle toolbox navigation, item descriptions, transfers, and closing requests.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_80031398 (0x80031398).
void toolboxHandleInput(void) {
    s32 *spin = &gToolboxState.spin;
    s32 buttons;
    s32 id;
    DescriptionSelection313 selected;
    char line[40];
    s8 kind;

    if (gToolboxState.mode == 5) {
        buttons = gControllerPrimaryRepeatedButtons;
        if (buttons & 0x8000) {
            textWindowDeactivate();
            toolboxSetMode((u8 *)&gToolboxState, 2);
        }
    } else if ((gToolboxMode == 2) && (gToolboxRotationStep == 0)) {
        buttons = gControllerPrimaryRepeatedButtons;
        if (buttons & 0x1000) {
            gToolboxExitAction = 0;
            messageBarEnqueue(0, 1.0f, (u8 *)"You closed the tool box.");
            toolboxSetMode((u8 *)&gToolboxState, 3);
            playerPlayTrackedSoundEffect((u8 *)&gPlayerState, 0xB8);
            return;
        }
        if (buttons & 0x4000) {
            TRACE("itembox help mode.\n");
            if (gToolboxState.slots[gToolboxSelectedRow][0].id != 0) {
                selected.id = gToolboxState.slots[gToolboxSelectedRow][0].kind;
                selected.text = (char *)itemGetHelpText(selected.id, 1);
            } else {
                selected.id = 0;
                selected.text = "(There isn't anything there.)\nWhen you see an item,\npress @K@L to read a\n"
                                "description of it.";
            }
            if (textWindowOpen(0, 0, 0) != 0) {
                if (selected.id != 0) {
                    kind = ((s8)gItemKinds[selected.id].itemClass);
                    sprintf(line, "%s%-22s^^%s\n", ((kind == 2) || (kind == 5)) ? "^^" : "^4",
                            itemGetDisplayName(selected.id, 1), itemGetClassLabelSuffix(selected.id));
                    textWindowAppendText((u8 *)line);
                }
                textWindowAppendText((u8 *)selected.text);
                textWindowAppendText((u8 *)gToolboxDescriptionSuffixes[((s8)gItemKinds[selected.id].itemClass)]);
                textWindowShowFirstPage();
                toolboxSetMode((u8 *)&gToolboxState, 5);
            }
        } else {
            if (buttons & 0x8000) {
                toolboxConfirmItemTransfer((u8 *)&gPlayerState);
                return;
            }
            if (buttons & 0x300) {
                if (buttons & 0x100) {
                    *spin = -1;
                } else {
                    *spin = 1;
                }
                playerPlayTrackedSoundEffect((u8 *)&gPlayerState, 0xAD);
                gToolboxState.timestamp = runtimeGetElapsedMilliseconds();

                id = gToolboxState.slots[gToolboxState.row][0].id;
                if (id == 0) {
                    toolboxSetObjectScale(gToolboxPlaceholderHandles, 0.3f);
                } else {
                    toolboxSetObjectScale(id, 0.3f);
                }
                gToolboxRowRotation = 180.0f;
                return;
            }
            if (((buttons & 0x400) && (gToolboxSelectedRow == 0)) ||
                ((buttons & 0x800) && (gToolboxSelectedRow == 1))) {
                gToolboxExitAction = 1;
                toolboxSetMode((u8 *)&gToolboxState, 3);
                playerPlayTrackedSoundEffect((u8 *)&gPlayerState, 0xB8);
                buttons = gControllerPrimaryRepeatedButtons;
                if (buttons & 0x400) {
                    gToolboxRequestedRow = 1;
                    return;
                }
                gToolboxRequestedRow = 0;
            }
        }
    }
}

/// Advance the current toolbox row selection by one entry with wraparound.
/// \param t Toolbox with row zero or one; positive spin selects the last entry, otherwise the second entry.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80031720 (0x80031720).
void toolboxRotateRowEntries(ToolboxState *t) {
    ToolboxSlot *row;
    s32 i;
    ToolboxSlot tmp;

    row = t->slots[t->row];
    if (t->spin > 0) {
        tmp = row[9];
        for (i = 9; i > 0; i--) {
            row[i] = row[i - 1];
        }
        row[0] = tmp;
    } else {
        tmp = row[0];
        for (i = 0; i < 9; i++) {
            row[i] = row[i + 1];
        }
        row[9] = tmp;
    }
}

void toolboxLayoutCurrentRow(f32, f32, f32);
void toolboxRotateRowEntries(ToolboxState *);

/// Animate movement to the next toolbox item and finalize its selection.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_800318A8 (0x800318A8).
void toolboxUpdateCarousel(void) {
    u64 now;
    ToolboxSlot *row = gToolboxState.slots[gToolboxState.row];
    f32 angle;
    f32 degreesPerSpin = 36.0f;
    f32 duration = 200.0f;
    f32 t;

    if (gToolboxState.spin != 0) {
        now = runtimeGetElapsedMilliseconds();
        t = now - gToolboxTransitionMilliseconds;
        if (t >= duration) {
            toolboxRotateRowEntries(&gToolboxState);
            if (row[0].id == 0) {
                toolboxSetObjectScale(gToolboxPlaceholderHandles, 0.5f);
            } else {
                toolboxSetObjectScale(row[0].id, 0.5f);
            }
            gToolboxRotationStep = 0;
        }
    } else {
        t = 0;
    }
    angle = gToolboxRotationStep * degreesPerSpin * t / duration;
    toolboxLayoutCurrentRow(angle, 0.0f, 0.0f);
    toolboxPlaceBaseForRow(&gToolboxState, 0);
}


/// Animate the selected toolbox row opening into its interactive display.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_800319DC (0x800319DC).
void toolboxAnimateRowOpening(void) {
    s32 *row = &gToolboxSelectedRow;
    f32 t = runtimeGetElapsedMilliseconds() - gToolboxTransitionMilliseconds;
    f32 k;

    if (t >= 300.0f) {
        t = 300.0f;
        gToolboxMode = 2;
    }
    if (*row == 0) {
        k = t / 300.0f;
        *(f32 *)(gToolboxState.objects[1] + 0xC) = -k * 90.0f;
        toolboxLayoutCurrentRow(360.0f * k, -1.0f * k * k + 1.0f, gToolboxRowAnchors[*row].z * (1.0f - k));
    } else {
        k = t / 300.0f;
        *(f32 *)(gToolboxState.objects[2] + 0x20) = k * 250.0f;
        toolboxLayoutCurrentRow(360.0f * k, k * k - 1.0f, gToolboxRowAnchors[*row].z * (1.0f - k));
    }
    toolboxPlaceBaseForRow(&gToolboxState, 0);
}

/// Animate the selected toolbox row closing before the toolbox leaves the display.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_80031B5C (0x80031B5C).
void toolboxAnimateRowClosing(void) {
    s32 *row = &gToolboxSelectedRow;
    f32 t = runtimeGetElapsedMilliseconds() - gToolboxTransitionMilliseconds;
    f32 k;

    if (t >= 300.0f) {
        toolboxLayoutCurrentRow(0.0f, 0.0f, 2000.0f);
        toolboxPlaceBaseForRow(&gToolboxState, 0);
        toolboxSetMode((u8 *)&gToolboxState, 4);
        return;
    }
    if (*row == 0) {
        k = t / 300.0f;
        *(f32 *)(gToolboxState.objects[1] + 0xC) = -(1.0f - k) * 90.0f;
        toolboxLayoutCurrentRow(360.0f * k, -1.0f * k * k + 1.0f, gToolboxRowAnchors[*row].z * k);
    } else {
        k = t / 300.0f;
        *(f32 *)(gToolboxState.objects[2] + 0x20) = (1.0f - k) * 250.0f;
        toolboxLayoutCurrentRow(360.0f * k, (1.0f - k) * (1.0f - k) - 1.0f, gToolboxRowAnchors[*row].z * k);
    }
    toolboxPlaceBaseForRow(&gToolboxState, 0);
}

void toolboxFinishClosing(void);

/// Animate the toolbox entering or leaving the display and advance the pending transition.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_80031D10 (0x80031D10).
void toolboxAnimateBaseTransition(void) {
    u64 now = runtimeGetElapsedMilliseconds();
    f32 t = now - gToolboxTransitionMilliseconds;

    if (t >= 200.0f || gToolboxExitAction != 0) {
        if (gToolboxMode == 0) {
            toolboxPlaceBaseForRow(&gToolboxState, 0.0f);
            toolboxSetMode((u8 *)&gToolboxState, 1);
            playerPlayTrackedSoundEffect((u8 *)&gPlayerState, 0xB9);
            if (gToolboxSelectedRow == 0) {
                playerPlayTrackedSoundEffect((u8 *)&gPlayerState, 0xB5);
            } else {
                playerPlayTrackedSoundEffect((u8 *)&gPlayerState, 0xB6);
            }
            return;
        }
        if (gToolboxMode == 4) {
            toolboxFinishClosing();
        }
    } else {
        if (gToolboxMode == 0) {
            toolboxLayoutCurrentRow(0.0f, 0.0f, 2000.0f);
            toolboxPlaceBaseForRow(&gToolboxState, t / 200.0f * 15.0f + -15.0f);
            return;
        }
        if (gToolboxMode == 4) {
            toolboxLayoutCurrentRow(0.0f, 0.0f, 2000.0f);
            toolboxPlaceBaseForRow(&gToolboxState, t / 200.0f * -15.0f);
        }
    }
}

void toolboxUpdateCarousel(void);
void toolboxAnimateRowOpening(void);
void toolboxAnimateRowClosing(void);
void toolboxAnimateBaseTransition(void);

/// Update the toolbox presentation and its current display animation.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_80031ECC (0x80031ECC).
void toolboxUpdateDisplay(void) {
    cameraBuildInterfaceTransform((f32 *)&gToolboxInterfaceTransform);
    switch (gToolboxMode) {
    case 0:
    case 4:
        toolboxAnimateBaseTransition();
        break;
    case 1:
        toolboxAnimateRowOpening();
        break;
    case 2:
    case 5:
        toolboxUpdateCarousel();
        break;
    case 3:
        toolboxAnimateRowClosing();
        break;
    }
}
