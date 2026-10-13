/// \file
/// Declares text-window message preparation and presentation entry points.

#ifndef TEXT_WINDOW_H
#define TEXT_WINDOW_H

#include <ultra64.h>

/// Open an inactive text window and reset its pending message.
/// \param indicator Supported animated indicator identifier, or zero to hide it.
/// \param style Supported text-style identifier.
/// \param viewMode Scene-view mode; nonzero enables scene updates during presentation.
/// \returns One when opened, or zero when a window is already active.
/// \note Original identifier: \c func_8002C348 (0x8002C348).
s32 textWindowOpen(s32 indicator, s32 style, s32 viewMode);

/// Append encoded text to the pending message while storage remains available.
/// \param text Borrowed null-terminated text, copied during the call.
/// \returns Nothing; text beyond the pending-message capacity is discarded.
/// \note Original identifier: \c func_8002C2E4 (0x8002C2E4).
void textWindowAppendText(u8 *text);

/// Prepare the first page of the pending message for display.
/// \returns One when another page remains, or zero otherwise.
/// \note Original identifier: \c func_8002C3FC (0x8002C3FC).
s32 textWindowShowFirstPage(void);

/// Display the pending message and enter the text-window game mode.
/// \returns Nothing.
/// \note Original identifier: \c func_8002C874 (0x8002C874).
void textWindowEnterMode(void);

/// Initialize window storage and history before retaining text-image resources.
/// \returns Nothing.
/// \note Original identifier: \c func_8002B6B0 (0x8002B6B0).
void textWindowInitialize(void);

/// Advance window transitions and input, including scene updates when enabled.
/// \returns Nothing.
/// \note Original identifier: \c func_8002C6A4 (0x8002C6A4).
void textWindowUpdate(void);

/// Draw the text-window game mode with its scene and active window transition.
/// \returns Nothing.
/// \note Requires the active graphics command list and initialized scene rendering.
/// \note Original identifier: \c func_8002C44C (0x8002C44C).
void textWindowDrawFrame(void);

/// Draw window text and its prompt within the current screen.
/// \returns Nothing.
/// \note Requires prepared page images and the active graphics command list.
/// \note Original identifier: \c func_8002C3BC (0x8002C3BC).
void textWindowDrawEmbedded(void);

/// Mark the window inactive while retaining its text resources.
/// \returns Nothing.
/// \note Original identifier: \c func_8002C43C (0x8002C43C).
void textWindowDeactivate(void);

#endif
