/// \file
/// Declares queued message-bar display and resource lifecycle operations.

#ifndef MESSAGE_BAR_H
#define MESSAGE_BAR_H

#include <ultra64.h>

/// Queue message text or refresh the matching displayed message's duration.
/// \param style Supported font-style index.
/// \param duration Display duration in seconds.
/// \param text Null-terminated encoded text of at most 59 bytes, copied during the call.
/// \returns One when accepted or refreshed, or zero for empty text or a full queue.
/// \note With no pending message, a prefix match refreshes the displayed message.
/// \note Original identifier: \c func_80027988 (0x80027988).
s32 messageBarEnqueue(s32 style, f32 duration, u8 *text);

/// Initialize message display state before any text images have been retained.
/// \returns Nothing.
/// \note Original identifier: \c func_800278F4 (0x800278F4).
void messageBarInitialize(void);

/// Reset message display state and release retained text images.
/// \returns Nothing.
/// \note Original identifier: \c func_80027924 (0x80027924).
void messageBarResetAndReleaseImages(void);

/// Discard queued messages and end the active display.
/// \returns Nothing.
/// \note Original identifier: \c func_80027A88 (0x80027A88).
void messageBarClear(void);

#endif
