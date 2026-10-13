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

#endif
