/// \file
/// Declares scrolling text-board hierarchy creation and borrowed-content controls.

#ifndef SCROLL_BOARD_H
#define SCROLL_BOARD_H

#include <ultra64.h>

/// Create and register a hierarchy containing a scrolling text board.
/// \param columns Positive width in twelve-pixel character columns.
/// \param lines Positive number of visible sixteen-pixel text rows.
/// \param red Red color component.
/// \param green Green color component.
/// \param blue Blue color component.
/// \param alpha Opacity.
/// \param leadingOutline Top/left palette index zero through fifteen, or minus one to disable.
/// \param trailingOutline Bottom/right palette index zero through fifteen, or minus one to disable.
/// \returns Registered root managed through hierarchy destruction.
/// \note Original identifier: \c func_80025D24 (0x80025D24).
u8 *scrollBoardCreateHierarchy(s32 columns, s32 lines, u8 red, u8 green, u8 blue, u8 alpha, s32 leadingOutline,
                              s32 trailingOutline);

/// Assign borrowed text to a board and reset its scroll position, preserving speed.
/// \param root Live hierarchy containing an initialized scrolling board.
/// \param list Null-terminated array with at least one text entry per visible row,
/// or null to hide text. The array and strings must remain valid while assigned;
/// entries must be nonempty when outlines are enabled.
/// \param style Configuration index zero through two, or another value for defaults.
/// \returns Nothing.
/// \note Original identifier: \c func_80025DAC (0x80025DAC).
void scrollBoardSetHierarchyText(u8 *root, u8 **list, s32 style);

/// Set a board's scrolling speed.
/// \param root Live hierarchy containing an initialized scrolling board.
/// \param speed Pixels per second; positive advances, negative moves backward, zero stops.
/// \returns Nothing.
/// \note Original identifier: \c func_80025DCC (0x80025DCC).
void scrollBoardSetHierarchySpeed(u8 *root, f32 speed);

#endif
