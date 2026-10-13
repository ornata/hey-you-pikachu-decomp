/// \file
/// Declares collection storage, withdrawal, display and toolbox entry.

#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <ultra64.h>

/// Initialize empty toolbox contents with the default Item Storage selection and initial supply item.
/// \returns Nothing; existing display objects and items are not released.
/// \note Original decompilation identifier: \c func_8002FD60 (0x8002FD60).
void toolboxReset(void);

/// Remove the objects in the toolbox collection row and clear its entries.
/// \returns Nothing.
/// \pre Every nonzero collection handle identifies a live item not pending destruction.
/// \note Portability: The adjacent loop statements preserve the IDO scheduling required by the ROM.
/// \note Original decompilation identifier: \c func_8002ED3C (0x8002ED3C).
void toolboxClearCollection(void);

/// Store an item in the first empty entry of a toolbox row.
/// \param object Address of a live initialized item with a valid catalog kind; existing destination items are not destroyed.
/// \param row Toolbox row index: zero for Collection or one for Item Storage.
/// \returns Borrowed address of the destination entry in the shared table, or zero if the row is full.
/// \note Original decompilation identifier: \c func_8002F27C (0x8002F27C).
s32 toolboxStoreObjectInRow(s32 object, s32 row);

/// Record acquisition of a supply item in persistent progress.
/// \param itemKind Valid item catalog kind whose acquisition is being recorded.
/// \returns One if the kind has a supply-item progress flag to set, or zero otherwise.
/// \note Original decompilation identifier: \c func_8002F380 (0x8002F380).
s32 toolboxRecordSupplyItem(s32 itemKind);

/// Remove an eligible item from the toolbox and restore it to active scene interaction.
/// \param row Toolbox row index: zero for Collection or one for Item Storage.
/// \param col Entry index from zero to nine.
/// \returns Borrowed live item object, or null if the entry is empty or its item cannot be removed.
/// \pre Each occupied entry identifies a live item with a valid catalog kind.
/// \note Original decompilation identifier: \c func_8002F3FC (0x8002F3FC).
u8 *toolboxTakeObject(s32 row, s32 col);

/// Draw the toolbox scene, displayed items, and the current interface or item description.
/// \param address Address of the display-list cursor to append to and advance.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_80030F68 (0x80030F68).
void toolboxDrawFrame(s32 address);

/// Open the toolbox display and prepare its selected row and held item.
/// \param row Row to open: zero for Collection, one for Item Storage, or minus one to retain the current row.
/// \param useHeld Nonzero to choose the row from the held item when an item is present, overriding the requested row.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_800310F8 (0x800310F8).
void toolboxOpen(s32 row, s32 useHeld);

#endif
