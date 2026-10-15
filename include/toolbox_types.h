/// \file
/// Defines retained item slots and the toolbox display's shared storage layout.

#ifndef TOOLBOX_TYPES_H
#define TOOLBOX_TYPES_H

#include <ultra64.h>

/// Retained item identity in a toolbox row or special slot.
/// A slot tracks an object handle and does not itself extend the object's lifetime.
typedef struct ToolboxSlot {
    s32 kind; ///< Catalog item kind, or zero for an empty slot.
    s32 id;   ///< Object-manager handle, or zero when no object is retained.
} ToolboxSlot;

/// Shared toolbox display state and retained item identities.
/// Lifecycle routines manage the display objects and stored items; copying this record does not transfer ownership.
typedef struct ToolboxState {
    u8 *objects[3];            ///< Toolbox display base and two linked display objects.
    s32 spin;                  ///< Signed row rotation step.
    f32 rotation;              ///< Row rotation in degrees.
    s32 row;                   ///< Selected toolbox row.
    u32 mode;                  ///< Toolbox transition and interaction mode.
    s32 unknown01C;            ///< Retained word whose purpose remains unresolved.
    u64 timestamp;             ///< Transition time origin.
    ToolboxSlot slots[2][10];  ///< Collection and Item Storage rows.
    s32 spareIds[10];          ///< Handles of placeholder objects for empty display entries.
    ToolboxSlot singletons[2]; ///< Bonus and purchase slots, in that order.
} ToolboxState;

typedef char ToolboxSlotSizeCheck[(sizeof(ToolboxSlot) == 8) ? 1 : -1];
typedef char ToolboxStateSizeCheck[(sizeof(ToolboxState) == 0x100) ? 1 : -1];
#define TOOLBOX_OFFSET(field) ((u32) & (((ToolboxState *)0)->field))
typedef char ToolboxRowsOffsetCheck[(TOOLBOX_OFFSET(slots) == 0x28) ? 1 : -1];
typedef char ToolboxPlaceholdersOffsetCheck[(TOOLBOX_OFFSET(spareIds) == 0xC8) ? 1 : -1];
typedef char ToolboxSpecialSlotsOffsetCheck[(TOOLBOX_OFFSET(singletons) == 0xF0) ? 1 : -1];
#undef TOOLBOX_OFFSET

#endif
