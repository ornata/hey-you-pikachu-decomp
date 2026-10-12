/// \file
/// Declares scrolling text-panel creation, display, and entry lifetime operations.

#ifndef TEXT_PANEL_H
#define TEXT_PANEL_H

#include "text_panel_types.h"

/// Initialize a scrolling panel with white text and a blank entry.
/// \param panel Writable panel storage that remains valid until entry release.
/// \param visibleLines Nonnegative visible-line count whose storage requirements fit the heap.
/// \returns Nothing.
void textPanelInitialize(TextPanel *panel, s32 visibleLines);

/// Release a panel's text-image entries and entry tables.
/// \param panel Initialized panel; its separate color allocation is retained.
/// \returns Nothing.
void textPanelReleaseEntries(TextPanel *panel);

/// Append encoded text, replacing the oldest entry when full.
/// \param panel Initialized panel receiving the entry.
/// \param text Borrowed null-terminated encoded string; an empty string appends a blank entry.
/// \returns Panel-owned text-image address, or zero for a blank entry; invalidated by replacement, clear or release.
s32 textPanelAppend(TextPanel *panel, void *text);

/// Discard existing text entries and restore a blank entry.
/// \param panel Initialized panel with valid entry storage.
/// \returns Nothing.
void textPanelClear(TextPanel *panel);

/// Set one line's display color.
/// \param panel Initialized panel.
/// \param line Color-slot index within allocated capacity.
/// \param red Red component from zero through 255.
/// \param green Green component from zero through 255.
/// \param blue Blue component from zero through 255.
/// \param alpha Opacity from zero through 255.
/// \returns Nothing.
void textPanelSetLineColor(TextPanel *panel, s32 line, s32 red, s32 green, s32 blue, s32 alpha);

/// Set every line's display color.
/// \param panel Initialized panel.
/// \param red Red component from zero through 255.
/// \param green Green component from zero through 255.
/// \param blue Blue component from zero through 255.
/// \param alpha Opacity from zero through 255.
/// \returns Nothing.
void textPanelSetColor(TextPanel *panel, s32 red, s32 green, s32 blue, s32 alpha);

/// Set a panel's screen offset.
/// \param panel Initialized panel.
/// \param x Horizontal offset in screen pixels.
/// \param y Vertical offset in screen pixels.
/// \returns Nothing.
void textPanelSetPosition(TextPanel *panel, s32 x, s32 y);

/// Select the configuration used by subsequent appends.
/// \param panel Initialized panel.
/// \param configuration Index zero through two; other values select defaults.
/// \returns Nothing.
void textPanelSetTextConfiguration(TextPanel *panel, s32 configuration);

/// Draw the panel's prepared entries.
/// \param panel Initialized panel with valid images and color storage.
/// \param gdl Display-list cursor to append to and advance.
/// \param mode Drawing mode interpreted by the panel renderer.
/// \returns Nothing.
void textPanelDraw(TextPanel *panel, Gfx **gdl, s32 mode);

#endif
