/// \file
/// Defines scrolling text-panel storage shared by panel builders and screen renderers.

#ifndef TEXT_PANEL_TYPES_H
#define TEXT_PANEL_TYPES_H
#include "text_image.h"
#include "font_image_types.h"

/// Scrolling panel owning line images and their display-order and color tables.
/// Initialization establishes storage for capacity entries. ReleaseEntries releases
/// image-entry storage but retains the separately allocated color table.
typedef struct TextPanel {
    s32 count;           ///< Number of retained lines, bounded by capacity.
    s32 nextEntry;       ///< Circular entry index replaced by the next appended line.
    s16 x, y;            ///< Display offset in screen pixels.
    u8 (*colors)[4];     ///< Owned per-line RGBA color table.
    s32 capacity;        ///< Number of entry slots, including the additional scroll line.
    TextImage **images;         ///< Display-order image pointers in the shared entry allocation.
    TextImage **entries;        ///< Circular image-entry pointers in the shared entry allocation.
    FontImage *configuration; ///< Borrowed font descriptor, or null for defaults.
} TextPanel;

typedef char TextPanelSizeCheck[(sizeof(TextPanel) == 0x20) ? 1 : -1];
typedef char TextPanelImagesOffsetCheck[(((u32) & ((TextPanel *)0)->images) == 0x14) ? 1 : -1];
typedef char TextPanelEntriesOffsetCheck[(((u32) & ((TextPanel *)0)->entries) == 0x18) ? 1 : -1];
#endif
