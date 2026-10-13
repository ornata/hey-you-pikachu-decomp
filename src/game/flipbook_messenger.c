#include "camera_storage.h"
#include "vector_metrics.h"
#include "math_angles.h"
#include "random.h"
#include "object_transform.h"
#include "scroll_board.h"
#include "messenger.h"
#include "camera_types.h"
#include "linked_effect.h"
#include "actor_meters.h"
#include "actor_part_shake.h"
#include "collision_registration.h"
#include "action_query.h"
#include "object_bounds.h"
#include "object_matrix_buffers.h"
#include "object_movement.h"
#include "object_flags.h"
#include "object_create.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "heap_free.h"
/// \file
/// Flipbook text and messenger effects that deliver actor messages.
/// \ingroup VoiceInteraction
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_audio.h"
#include "text_draw.h"
#include "text_panel.h"
#include "integer_animation.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "object_types.h"
#include "actor_script.h"
#include "heap_types.h"

/// Private text model owning two panels and drawing storage.
/// Panel colors survive destruction; caller-owned model storage remains valid.
typedef struct Flipbook {
    /* 0x00 */ TextPanel panels[2]; ///< Displayed and prepared text banks.
    /* 0x40 */ s32 width; ///< Surface width in pixels.
    /* 0x44 */ s32 stripCount; ///< Number of visible sixteen-pixel rows.
    /* 0x48 */ s32 pendingSwap; ///< Nonzero when the next draw must select prepared text.
    /* 0x4C */ s32 activeBank; ///< Currently displayed panel and command bank.
    /* 0x50 */ Gfx *displayList; ///< Owned drawing allocation and surface display list.
    /* 0x54 */ Gfx *rowBranches[2]; ///< Per-bank row branches within the drawing allocation.
    /* 0x5C */ Gfx *commands[2]; ///< Per-bank text commands within the drawing allocation.
    /* 0x64 */ Vtx *vertices; ///< Surface vertices within the drawing allocation.
    /* 0x68 */ u8 pad68[4]; ///< Unresolved storage; no semantic interpretation established.
    /* 0x6C */ u8 color[4]; ///< RGBA modulation color.
    /* 0x70 */ u8 outlineIndices[2]; ///< Leading and trailing outline indices; 255 disables an outline.
} Flipbook;

typedef char FlipbookSizeCheck[(sizeof(Flipbook) == 0x74) ? 1 : -1];
typedef char FlipbookWidthOffsetCheck[(((u32) & ((Flipbook *)0)->width) == 0x40) ? 1 : -1];
typedef char FlipbookRowsOffsetCheck[(((u32) & ((Flipbook *)0)->stripCount) == 0x44) ? 1 : -1];
typedef char FlipbookBankOffsetCheck[(((u32) & ((Flipbook *)0)->activeBank) == 0x4C) ? 1 : -1];
typedef char FlipbookBranchesOffsetCheck[(((u32) & ((Flipbook *)0)->rowBranches) == 0x54) ? 1 : -1];
typedef char FlipbookColorOffsetCheck[(((u32) & ((Flipbook *)0)->color) == 0x6C) ? 1 : -1];
typedef char FlipbookOutlinesOffsetCheck[(((u32) & ((Flipbook *)0)->outlineIndices) == 0x70) ? 1 : -1];
typedef char FlipbookDrawingOffsetCheck[(((u32)&((Flipbook *)0)->displayList) == 0x50) ? 1 : -1];
typedef char FlipbookVerticesOffsetCheck[(((u32)&((Flipbook *)0)->vertices) == 0x64) ? 1 : -1];

/// Owned scrolling state and model, borrowing its assigned text-pointer array.
typedef struct ScrollBoardState {
    u8 **text; ///< Borrowed null-terminated array of encoded text pointers.
    s32 topLine; ///< First text entry displayed by the model.
    s32 textCount; ///< Number of assigned non-null text entries.
    f32 rowOffset; ///< Vertical offset within the current sixteen-pixel row.
    f32 speed; ///< Signed scrolling speed in pixels per second.
    Flipbook model; ///< Owned text panels and drawing storage.
} ScrollBoardState;

typedef char ScrollBoardStateSizeCheck[(sizeof(ScrollBoardState) == 0x88) ? 1 : -1];
typedef char ScrollBoardModelOffsetCheck[(((u32) & ((ScrollBoardState *)0)->model) == 0x14) ? 1 : -1];
typedef char ScrollBoardSpeedOffsetCheck[(((u32) & ((ScrollBoardState *)0)->speed) == 0x10) ? 1 : -1];
typedef char ScrollBoardRowsOffsetCheck[(((u32) & ((ScrollBoardState *)0)->model.stripCount) == 0x58) ? 1 : -1];

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on
void messengerBeginHoming(u8 *);
s32 messengerGetActorContactMode(u8 *);
void messengerDeliver(u8 *, u8 *, u8 *, s32);
void messengerBeginDissipation(f32 *);
/// Integer appearance animation used while a messenger dissipates.
/// \note Original identifier: \c D_8009999C (0x8009999C).
extern u8 gMessengerDissipationAnimation[];
/// Handle of the most recently launched messenger, or zero when no messenger is tracked.
/// \note Original identifier: \c gzip_data_0000 (0x80099960).
extern s32 gTrackedMessengerHandle;
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

void flipbookInitializeGeometry(u8 *object, u8 red, u8 green, u8 blue, u8 alpha);
void flipbookBuildGeometryDisplayList(u8 *);
void flipbookResetImageDisplayLists(u8 *);

/// Initialize a text-display model with the requested dimensions, color, and text outlines.
/// \param object Writable model storage of at least 116 bytes, valid until destruction; previous resources must already be released.
/// \param columns Positive model width in twelve-pixel character columns.
/// \param lines Nonnegative number of sixteen-pixel text lines.
/// \param red Red color component, from 0 through 255.
/// \param green Green color component, from 0 through 255.
/// \param blue Blue color component, from 0 through 255.
/// \param alpha Opacity, from 0 transparent to 255 opaque.
/// \param leadingOutline Top and left outline palette index from 0 through 15, or -1 to disable it.
/// \param trailingOutline Bottom and right outline palette index from 0 through 15, or -1 to disable it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025030 (0x80025030).
void flipbookInitialize(u8 *object, s32 columns, s32 lines, u8 red, u8 green, u8 blue, u8 alpha, s32 leadingOutline, s32 trailingOutline) {
    s32 branchSize;
    u32 commandSize;
    s32 prefixSize;
    u8 *cursor;

    TRACE("sbdInit(%p, %d, %d)\n", object, columns, lines);
    TRACE("sbd->line:%d\n", lines);
    textPanelInitialize(&((Flipbook *)object)->panels[0], lines);
    textPanelInitialize(&((Flipbook *)object)->panels[1], lines);
    ((Flipbook *)object)->width = columns * 0xC;
    ((Flipbook *)object)->stripCount = lines;
    ((Flipbook *)object)->pendingSwap = 0;
    ((Flipbook *)object)->activeBank = 0;
    ((Flipbook *)object)->outlineIndices[0] = leadingOutline;
    branchSize = lines * 8;
    prefixSize = (branchSize * 8) + 0xA0;
    commandSize = (lines * 0x14 + 0x14) * sizeof(Gfx);
    ((Flipbook *)object)->outlineIndices[1] = trailingOutline;
    cursor = (u8 *)heapAllocLocal((HpHdr *)&gGameHeap, (commandSize * 2) + prefixSize + (branchSize * 2) + (lines << 6),
                                 0x8E);
    ((Flipbook *)object)->displayList = (Gfx *)cursor;
    cursor += prefixSize;
    ((Flipbook *)object)->rowBranches[0] = (Gfx *)cursor;
    cursor += branchSize;
    ((Flipbook *)object)->rowBranches[1] = (Gfx *)cursor;
    cursor += branchSize;
    ((Flipbook *)object)->commands[0] = (Gfx *)cursor;
    cursor += commandSize;
    ((Flipbook *)object)->commands[1] = (Gfx *)cursor;
    cursor += commandSize;
    ((Flipbook *)object)->vertices = (Vtx *)cursor;
    flipbookInitializeGeometry(object, red, green, blue, alpha);
    flipbookBuildGeometryDisplayList(object);
    flipbookResetImageDisplayLists(object);
    ((Flipbook *)object)->color[0] = 0xFF;
    ((Flipbook *)object)->color[1] = 0xFF;
    ((Flipbook *)object)->color[2] = 0xFF;
    ((Flipbook *)object)->color[3] = 0xFF;
}

/// Release the model drawing storage and text images, retaining the panel color storage and caller-owned model storage.
/// \param object Initialized text-display model with valid owned drawing and text storage.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025170 (0x80025170).
void flipbookDestroy(Flipbook *object) {
    heapFreeLocal(&gGameHeap, object->displayList);
    textPanelReleaseEntries(&object->panels[0]);
    textPanelReleaseEntries(&object->panels[1]);
}

/// Initialize the text-display model surface with the requested color.
/// \param object Initialized text-display model with valid owned drawing and text storage.
/// \param red Red color component, from 0 through 255.
/// \param green Green color component, from 0 through 255.
/// \param blue Blue color component, from 0 through 255.
/// \param alpha Opacity, from 0 transparent to 255 opaque.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800251B4 (0x800251B4).
void flipbookInitializeGeometry(u8 *object, u8 red, u8 green, u8 blue, u8 alpha) {
    s32 y;
    s32 halfWidth;
    Vtx *vertices;
    s32 i;
    y = (((Flipbook *)object)->stripCount * 16) / 2;
    halfWidth = ((Flipbook *)object)->width / 2;
    vertices = *(Vtx **)(object + 0x64);
    for (i = 0; i < ((Flipbook *)object)->stripCount; i++) {
        vertices[0].v.ob[0] = 0;
        vertices[0].v.ob[1] = y;
        vertices[0].v.ob[2] = halfWidth;
        vertices[0].v.flag = 0;
        vertices[0].v.tc[0] = 0;
        vertices[0].v.tc[1] = 0;
        vertices[0].v.cn[0] = red;
        vertices[0].v.cn[1] = green;
        vertices[0].v.cn[2] = blue;
        vertices[0].v.cn[3] = alpha;
        vertices[1].v.ob[1] = y - 16;
        vertices[1].v.ob[0] = 0;
        vertices[1].v.ob[2] = halfWidth;
        vertices[1].v.flag = 0;
        vertices[1].v.tc[0] = 0;
        vertices[1].v.tc[1] = 0x200;
        vertices[1].v.cn[0] = red;
        vertices[1].v.cn[1] = green;
        vertices[1].v.cn[2] = blue;
        vertices[1].v.cn[3] = alpha;
        vertices[2].v.ob[1] = y - 16;
        vertices[2].v.ob[0] = 0;
        vertices[2].v.ob[2] = halfWidth - ((Flipbook *)object)->width;
        vertices[2].v.flag = 0;
        vertices[2].v.tc[0] = ((Flipbook *)object)->width << 5;
        vertices[2].v.cn[3] = alpha;
        vertices[2].v.cn[2] = blue;
        vertices[2].v.cn[1] = green;
        vertices[2].v.cn[0] = red;
        vertices[2].v.tc[1] = 0x200;
        vertices[3].v.ob[1] = y;
        vertices[3].v.ob[0] = 0;
        vertices[3].v.ob[2] = halfWidth - ((Flipbook *)object)->width;
        vertices[3].v.flag = 0;
        vertices[3].v.tc[0] = ((Flipbook *)object)->width << 5;
        vertices[3].v.cn[3] = alpha;
        vertices[3].v.cn[2] = blue;
        vertices[3].v.cn[1] = green;
        vertices[3].v.cn[0] = red;
        vertices[3].v.tc[1] = 0;
        vertices += 4;
        y -= 16;
    }
}

/// Prepare drawing commands for the text-display model surface.
/// \param object Initialized text-display model with valid owned drawing and text storage.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025308 (0x80025308).
void flipbookBuildGeometryDisplayList(u8 *object) {
    Gfx *gp;
    Vtx *vtx;
    s32 vtxLeft;
    s32 vtxMax;
    s32 vtxIdx;
    s32 i;

    gp = ((Flipbook *)object)->displayList;
    vtxMax = 0;
    vtxIdx = 0;
    vtxLeft = (s32)((u32)((Flipbook *)object)->stripCount << 2);
    vtx = ((Flipbook *)object)->vertices;
    gDPPipeSync(gp++);
    gSPLoadGeometryMode(gp++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetRenderMode(gp++, 0x0C184A50, 0);
    gDPSetTextureLOD(gp++, G_TL_TILE);
    gDPSetTextureDetail(gp++, G_TD_CLAMP);
    gDPSetTexturePersp(gp++, G_TP_PERSP);
    gDPSetTextureFilter(gp++, G_TF_BILERP);
    gDPSetTextureConvert(gp++, G_TC_FILT);
    gSPTexture(gp++, 0x7FFF, 0, 0, G_TX_RENDERTILE, G_ON);
    gMoveWd(gp++, G_MW_FOG, 0x10, 0x7FFF);
    for (i = 0; i < ((Flipbook *)object)->stripCount; i++) {
        s32 *count = &((Flipbook *)object)->stripCount;

        if (vtxIdx >= vtxMax) {
            vtxIdx = 0;
            vtxMax = (vtxLeft > 32) ? 32 : vtxLeft;
            gSPVertex(gp++, vtx, vtxMax, 0);
            vtxLeft -= vtxMax;
            vtx += vtxMax;
            TRACE("vtxIdx:%d  vtxMax:%d  vtxLeft:%d\n", vtxIdx, vtxMax, vtxLeft);
        }
        gSPDisplayList(gp++, (Gfx *)0x06000000 + i);
        gSP2Triangles(gp++, vtxIdx, vtxIdx + 1, vtxIdx + 2, 0, vtxIdx, vtxIdx + 2, vtxIdx + 3, 0);
        vtxIdx += 4;
        if (gp - ((Flipbook *)object)->displayList > *count * 8 + 20) {
            TRACE("ERROR: gpSize over:%d\n", gp - ((Flipbook *)object)->displayList);
        }
    }
    gSPTexture(gp++, 0x7FFF, 0, 0, G_TX_RENDERTILE, G_OFF);
    gMoveWd(gp++, G_MW_FOG, 0x10, 0x7FFF);
    gSPEndDisplayList(gp++);
}

/// Reset the model drawing buffers to show no text images.
/// \param object Initialized text-display model with valid owned drawing and text storage.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800255A0 (0x800255A0).
void flipbookResetImageDisplayLists(u8 *object) {
    s32 i;
    Gfx *p = *(Gfx **)(object + 0x54);

    for (i = 0; i < ((Flipbook *)object)->stripCount * 2; i++) {
        gSPEndDisplayList(p++);
    }
}

/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;

/// Prepare the latest text-display model content and color for drawing.
/// \param f Initialized text-display model to prepare; a writable global drawing cursor must be available.
/// \returns A borrowed display list owned by the model, valid until destruction.
/// \note Original decompilation identifier: \c func_800255E8 (0x800255E8).
Gfx *flipbookPrepareDraw(Flipbook *f) {
    if (f->pendingSwap != 0) {
        f->pendingSwap = 0;
        f->activeBank ^= 1;
    }
    TRACE("drawIdx:%d\n", f->activeBank);
    gSPSegment(gRenderDisplayListCursor++, 6, osVirtualToPhysical(f->rowBranches[f->activeBank]));
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, f->color[0], f->color[1], f->color[2], f->color[3]);
    return f->displayList;
}

/// Prepare updated text-image drawing commands for the next model draw.
/// \param object Initialized text-display model with valid owned drawing and text storage.
/// \returns Nothing.
///
/// \note Portability: the compiled-out size check retains an IDO saved register; its original bound is unrecoverable, so the source uses a placeholder.
/// \note Original decompilation identifier: \c func_800256C8 (0x800256C8).
void flipbookBuildImageDisplayLists(u8 *object) {
    Flipbook *lists;
    TextImage *image;
    Gfx *commands;
    Gfx *branches;
    TextImage **images;
    s32 index;
    s32 bank;
    textDrawSetPalette(gDefaultTextPalette);
    bank = ((Flipbook *)object)->activeBank ^ 1;
    branches = ((Flipbook *)object)->rowBranches[bank];
    commands = ((Flipbook *)object)->commands[bank];
    images = (TextImage **)((Flipbook *)object)->panels[bank].images + 1;
    for (index = 0; index < ((Flipbook *)object)->stripCount; index++) {
        gSPBranchList(branches++, commands);
        image = *images;
        if (image != NULL) {
            if (image->width == 100 || image->height == 100) {
            }
            textImageLoadTexture(&commands, image);
            if ((*images)->format == G_IM_FMT_CI) {
                gDPSetCombine(commands++, 0x40C3FF, 0xFFFFFE38);
            } else {
                gDPSetCombine(commands++, 0x4417FF, 0x3FFDFE38);
            }
        } else {
            gSPTexture(commands++, 0x7FFF, 0, 0, 0, G_OFF);
            gMoveWd(commands++, G_MW_FOG, 0x10, 0x7FFF);
            gDPSetCombine(commands++, 0xFFFFFF, 0xFFFDF638);
        }
        gSPEndDisplayList(commands++);
        images++;
    }
    TRACE("workIdx:%d\n", bank);
    TRACE("ERROR: gpSize over:%d\n", commands - ((Flipbook *)object)->commands[bank]);
    lists = (Flipbook *)object;
    lists->pendingSwap = 1;
}

void image4BitAddOffsetOutline(u8 *, s32, s32, s32, s32);
/// Replace the model text using its configured outline appearance.
/// \param object Initialized text-display model with valid owned drawing and text storage.
/// \param text At least count null-terminated text-entry addresses, read during this call; entries must be nonempty when outlines are enabled.
/// \param count Nonnegative number of text entries to display.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002588C (0x8002588C).
void flipbookSetText(u8 *object, u8 **text, s32 count) {
    u8 *lineImage;
    s32 activeBuffer;
    s8 leadingOutline;
    s8 trailingOutline;
    TextPanel *inactivePanel;

    TRACE("sbdApply(%p, %p, %d)\n", object, text, count);
    activeBuffer = ((Flipbook *)object)->activeBank;
    inactivePanel = &((Flipbook *)object)->panels[activeBuffer ^ 1];
    textPanelClear(inactivePanel);
    if (count != 0) {
        do {
            TRACE(" Apply:(%s)\n", *text);
            lineImage = (u8 *)textPanelAppend(inactivePanel, *text);
            trailingOutline = (s8)((Flipbook *)object)->outlineIndices[1];
            if ((trailingOutline >= 0) && (trailingOutline < 0x10)) {
                image4BitAddOffsetOutline(lineImage, 1, 0, 0, (s32)trailingOutline);
                image4BitAddOffsetOutline(lineImage, 0, 1, 0, (s32) (s8)((Flipbook *)object)->outlineIndices[1]);
            }
            leadingOutline = (s8)((Flipbook *)object)->outlineIndices[0];
            if ((leadingOutline >= 0) && (leadingOutline < 0x10)) {
                image4BitAddOffsetOutline(lineImage, -1, 0, 0, (s32)leadingOutline);
                image4BitAddOffsetOutline(lineImage, 0, -1, 0, (s32) (s8)((Flipbook *)object)->outlineIndices[0]);
            }
            count -= 1;
            text++;
        } while (count != 0);
    }
    flipbookBuildImageDisplayLists(object);
}

/// Set the text-display model drawing color and opacity.
/// \param object Initialized text-display model with valid owned drawing and text storage.
/// \param red Red color component, from 0 through 255.
/// \param green Green color component, from 0 through 255.
/// \param blue Blue color component, from 0 through 255.
/// \param alpha Opacity, from 0 transparent to 255 opaque.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002599C (0x8002599C).
void flipbookSetColor(u8 *object, u8 red, u8 green, u8 blue, u8 alpha) {
    object[0x6C] = red;
    object[0x6D] = green;
    object[0x6E] = blue;
    object[0x6F] = alpha;
}

void _bzero(void *, s32);

/// Initialize the scrolling text-board object state.
/// \param object Board receiving owned state; previous state must already be released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800259C0 (0x800259C0).
void scrollBoardInitializeObject(u8 *object) {
    s32 savedAllocationAddress;
    s32 allocationAddress;

    allocationAddress = (s32)heapAllocLocal((HpHdr *)&gGameHeap, sizeof(ScrollBoardState), 0x1C5);
    savedAllocationAddress = allocationAddress;
    _bzero((void *)allocationAddress, sizeof(ScrollBoardState));
    ((RuntimeObjectPrefix *)object)->state.address = savedAllocationAddress;
}

/// Release the scrolling text-board state and displayed text.
/// \param object Initialized scrolling text board with valid model state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025A0C (0x80025A0C).
void scrollBoardDestroyObject(u8 *object) {
    u8 *p = ((RuntimeObjectPrefix *)object)->state.bytes;

    flipbookDestroy(&((ScrollBoardState *)p)->model);
    heapFreeLocal(&gGameHeap, p);
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance the scrolling text-board position and stop at either end of its text.
/// \param object Initialized scrolling text board with valid model state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025A44 (0x80025A44).
void scrollBoardUpdate(u8 *object) {
    ScrollBoardState *boardState;
    f32 scrollSpeed;
    f32 zero = 0.0f;
    s32 lastTopLine;
    s32 topLine;

    boardState = ((RuntimeObjectPrefix *)object)->state.pointer;
    if (boardState->text != 0) {
        scrollSpeed = boardState->speed;
        if (zero != scrollSpeed) {
            topLine = boardState->topLine;
            boardState->rowOffset = (f32)(boardState->rowOffset + (scrollSpeed * gRuntimeFrameSeconds));
            if (scrollSpeed > 0.0f) {
                if (boardState->rowOffset >= 16.0f) {
                    do {
                        topLine += 1;
                        boardState->rowOffset = (f32)(boardState->rowOffset - 16.0f);
                    } while (boardState->rowOffset >= 16.0f);
                }
                lastTopLine = boardState->textCount - boardState->model.stripCount;
                if (topLine >= lastTopLine) {
                    topLine = lastTopLine;
                    boardState->speed = 0.0f;
                    boardState->rowOffset = 0.0f;
                }
            } else {
                if (boardState->rowOffset < 0.0f) {
                    do {
                        topLine -= 1;
                        boardState->rowOffset = (f32)(boardState->rowOffset + 16.0f);
                    } while (boardState->rowOffset < 0.0f);
                }
                if (topLine < 0) {
                    topLine = 0;
                    boardState->speed = 0.0f;
                    boardState->rowOffset = 0.0f;
                }
            }
            if (topLine != boardState->topLine) {
                boardState->topLine = topLine;
                TRACE("newTextIdx:%d\n", topLine);
                TRACE("Error: textData_lines:%d < %d\n", boardState->textCount, topLine);
                flipbookSetText((u8 *)&boardState->model, boardState->text + topLine,
                                boardState->model.stripCount);
            }
        }
        ((RuntimeObjectPrefix *)object)->position[1] = (f32)boardState->rowOffset;
    }
}


/// Prepare the current scrolling text-board content for drawing.
/// \param object Initialized scrolling text board with valid model state.
/// \returns A borrowed display list owned by the board, or null when no text is assigned.
/// \note Original decompilation identifier: \c func_80025BA4 (0x80025BA4).
Gfx *scrollBoardPrepareDraw(u8 *object) {
    ScrollBoardState *boardState;

    boardState = ((RuntimeObjectPrefix *)object)->state.pointer;
    if (boardState->text != NULL) {
        return flipbookPrepareDraw((Flipbook *)((u8 *)(u8 *)&boardState->model));
    }
    return 0;
}

/// Object template used to create scrolling text boards.
/// \note Original identifier: \c D_80099920 (0x80099920).
extern u8 gScrollBoardObjectTemplate[];

/// Create a scrolling text-board object with the requested appearance.
/// \param columns Positive board width in twelve-pixel character columns.
/// \param lines Positive number of sixteen-pixel text lines displayed at once.
/// \param red Red color component, from 0 through 255.
/// \param green Green color component, from 0 through 255.
/// \param blue Blue color component, from 0 through 255.
/// \param alpha Opacity, from 0 transparent to 255 opaque.
/// \param leadingOutline Top and left outline palette index from 0 through 15, or -1 to disable it.
/// \param trailingOutline Bottom and right outline palette index from 0 through 15, or -1 to disable it.
/// \returns Address of an owned board object to release through object destruction.
/// \note Original decompilation identifier: \c func_80025BE0 (0x80025BE0).
s32 scrollBoardCreateObject(s32 columns, s32 lines, u8 red, u8 green, u8 blue, u8 alpha, s32 leadingOutline, s32 trailingOutline) {
    u8 *sp30;
    u8 *boardObject;
    s32 boardStateAddress;

    TRACE("sbdScrollBoardNew(%d, %d)\n", columns, lines);
    boardObject = objectCreateFromTemplate((ObjectTemplate *)gScrollBoardObjectTemplate);
    sp30 = boardObject;
    boardStateAddress = ((RuntimeObjectPrefix *)boardObject)->state.address;
    flipbookInitialize((u8 *)(boardStateAddress + 0x14), columns, lines, red, green, blue, alpha, leadingOutline,
                       trailingOutline);
    return (s32)boardObject;
}


/// Assign scrolling text-board content and return to the beginning of non-null text, preserving scrolling speed.
/// \param object Initialized scrolling text board with valid model state.
/// \param list Borrowed zero-terminated array with at least one entry per visible line; the array and null-terminated text must remain valid while assigned, or pass null to hide the text. Entries must be nonempty when outlines are enabled.
/// \param style Text configuration 0 through 2, or another value to select the default configuration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025C5C (0x80025C5C).
void scrollBoardSetText(u8 *object, u8 **list, s32 style) {
    u8 *v = ((RuntimeObjectPrefix *)object)->state.bytes;
    s32 n;
    u8 **p;

    if (list == NULL) {
        ((ScrollBoardState *)v)->text = list;
        ((ScrollBoardState *)v)->textCount = 0;
        return;
    }
    n = 0;
    for (p = list; *p != 0; n++) {
        p++;
    }
    ((ScrollBoardState *)v)->textCount = n;
    ((ScrollBoardState *)v)->text = list;
    ((ScrollBoardState *)v)->topLine = 0;
    ((ScrollBoardState *)v)->rowOffset = 0.0f;
    ((RuntimeObjectPrefix *)object)->position[1] = 0.0f;
    textPanelSetTextConfiguration(&((ScrollBoardState *)v)->model.panels[0], style);
    textPanelSetTextConfiguration(&((ScrollBoardState *)v)->model.panels[1], style);
    flipbookSetText((u8 *)&((ScrollBoardState *)v)->model, list, ((ScrollBoardState *)v)->model.stripCount);
}

/// Set the scrolling text-board speed.
/// \param object Initialized scrolling text board with valid model state.
/// \param speed Scroll speed in pixels per second: positive advances through the text, negative moves backward, and zero stops scrolling.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025D14 (0x80025D14).
void scrollBoardSetSpeed(u8 *object, f32 speed) {
    ScrollBoardState *boardState = ((RuntimeObjectPrefix *)object)->state.pointer;
    boardState->speed = speed;
}

s32 scrollBoardCreateObject(s32, s32, u8, u8, u8, u8, s32, s32); /* extern */

/// Create and register a hierarchy containing a scrolling text board.
/// \param columns Positive board width in twelve-pixel character columns.
/// \param lines Positive number of sixteen-pixel text lines displayed at once.
/// \param red Red color component, from 0 through 255.
/// \param green Green color component, from 0 through 255.
/// \param blue Blue color component, from 0 through 255.
/// \param alpha Opacity, from 0 transparent to 255 opaque.
/// \param leadingOutline Top and left outline palette index from 0 through 15, or -1 to disable it.
/// \param trailingOutline Bottom and right outline palette index from 0 through 15, or -1 to disable it.
/// \returns A registered hierarchy root to release through hierarchy destruction.
/// \note Original decompilation identifier: \c func_80025D24 (0x80025D24).
u8 *scrollBoardCreateHierarchy(s32 columns, s32 lines, u8 red, u8 green, u8 blue, u8 alpha, s32 leadingOutline, s32 trailingOutline) {
    u8 *hierarchyRoot;

    hierarchyRoot = objectCreateDefault();
    ((RuntimeObjectPrefix *)hierarchyRoot)->child.address =
        scrollBoardCreateObject(columns, lines, red, green, blue, alpha, leadingOutline, trailingOutline);
    objectMakeHierarchyRoot(hierarchyRoot);
    objectAllocateHierarchyMatrixBuffers(hierarchyRoot);
    objectRegisterActiveHierarchy(hierarchyRoot);
    return hierarchyRoot;
}

/// Assign text to the scrolling board in a hierarchy.
/// \param root Hierarchy root containing the scrolling text board.
/// \param list Borrowed zero-terminated array with at least one entry per visible line; the array and null-terminated text must remain valid while assigned, or pass null to hide the text. Entries must be nonempty when outlines are enabled.
/// \param style Text configuration 0 through 2, or another value to select the default configuration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025DAC (0x80025DAC).
void scrollBoardSetHierarchyText(u8 *root, u8 **list, s32 style) {
    scrollBoardSetText(((RuntimeObjectPrefix *)root)->child.bytes, list, style);
}

/// Set the scrolling speed of the board in a hierarchy.
/// \param root Hierarchy root containing the scrolling text board.
/// \param speed Scroll speed in pixels per second: positive advances through the text, negative moves backward, and zero stops scrolling.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025DCC (0x80025DCC).
void scrollBoardSetHierarchySpeed(u8 *root, f32 speed) {
    scrollBoardSetSpeed(((RuntimeObjectPrefix *)root)->child.bytes, speed);
}

/// Set an object position, orientation, and uniform scale.
/// \param object Object whose transform is set.
/// \param position Three-component position.
/// \param rotation Writable three-component rotation in degrees, normalized in place to 0 through 360 exclusive; magnitudes must permit normalization by 360-degree steps.
/// \param scale Uniform scale applied to the object.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025DF4 (0x80025DF4).
void objectSetUniformTransform(u8 *object, f32 *position, f32 *rotation, f32 scale) {
    if (rotation[0] < 0.0f) {
        do {
            rotation[0] = (f32)(rotation[0] + 360.0f);
        } while (rotation[0] < 0.0f);
    }
    if (rotation[0] >= 360.0f) {
        do {
            rotation[0] = (f32)(rotation[0] - 360.0f);
        } while (rotation[0] >= 360.0f);
    }
    if (rotation[1] < 0.0f) {
        do {
            rotation[1] = (f32)(rotation[1] + 360.0f);
        } while (rotation[1] < 0.0f);
    }
    if (rotation[1] >= 360.0f) {
        do {
            rotation[1] = (f32)(rotation[1] - 360.0f);
        } while (rotation[1] >= 360.0f);
    }
    if (rotation[2] < 0.0f) {
        do {
            rotation[2] = (f32)(rotation[2] + 360.0f);
        } while (rotation[2] < 0.0f);
    }
    if (rotation[2] >= 360.0f) {
        do {
            rotation[2] = (f32)(rotation[2] - 360.0f);
        } while (rotation[2] >= 360.0f);
    }
    ((RuntimeObjectPrefix *)object)->position[0] = (f32)position[0];
    ((RuntimeObjectPrefix *)object)->position[1] = (f32)position[1];
    ((RuntimeObjectPrefix *)object)->position[2] = (f32)position[2];
    ((RuntimeObjectPrefix *)object)->worldPosition[0] = (f32)position[0];
    ((RuntimeObjectPrefix *)object)->worldPosition[1] = (f32)position[1];
    ((RuntimeObjectPrefix *)object)->worldPosition[2] = (f32)position[2];
    ((RuntimeObjectPrefix *)object)->rotation[0] = (f32)rotation[0];
    ((RuntimeObjectPrefix *)object)->rotation[1] = (f32)rotation[1];
    ((RuntimeObjectPrefix *)object)->rotation[2] = (f32)rotation[2];
    ((RuntimeObjectPrefix *)object)->scale[2] = scale;
    ((RuntimeObjectPrefix *)object)->scale[1] = scale;
    ((RuntimeObjectPrefix *)object)->scale[0] = scale;
}

/// Query the actor contact mode used to handle messenger delivery.
/// \param actor Target actor to query.
/// \returns The actor contact mode, or zero for an unsupported object kind.
/// \note Original decompilation identifier: \c func_80025F80 (0x80025F80).
s32 messengerGetActorContactMode(u8 *actor) {
    u16 actorKind;
    u8 *actorState;

    actorKind = *(u16 *)(actor + 0x34);
    if (((s32)actorKind <= 0) || ((s32)actorKind >= 0x26)) {
        return 0;
    }
    actorState = *(u8 **)(actor + 0x54);
    return ((ActorScriptState *)actorState)->messengerContactMode;
}

/// Behaviors serviced by a messenger projectile's update callback.
typedef enum MessengerFlightMode {
    MESSENGER_LAUNCH = 0, ///< Initial homing-projectile launch interval.
    MESSENGER_HOMING = 1, ///< Guided flight toward the recipient.
    MESSENGER_DISSIPATING = 2, ///< Fading projectile awaiting removal.
    MESSENGER_REBOUND = 3, ///< Brief flight after recipient contact.
    MESSENGER_COLLISION_FLIGHT = 4 ///< Direct flight with collision delivery.
} MessengerFlightMode;

/// Owned messenger flight state, appearance animation and copied delivery payload.
typedef struct MessengerFlightState {
    s32 mode;              ///< Current messenger flight behavior.
    f32 remainingTime;     ///< Countdown controlling the current behavior transition.
    f32 phase;             ///< Homing orbit phase in degrees.
    s32 linkedId;          ///< Handle of the linked delivery target.
    ValueTrack animation;  ///< Appearance animation playback state.
    f32 velocity[3];       ///< Local-position velocity in units per second.
    ScriptMessage message; ///< Copied pending delivery message.
} MessengerFlightState;

typedef char MessengerFlightStateSizeCheck[(sizeof(MessengerFlightState) == 0x40) ? 1 : -1];
typedef char MessengerFlightTimerOffsetCheck[(((u32) & (((MessengerFlightState *)0)->remainingTime)) == 4) ? 1 : -1];
typedef char MessengerFlightPhaseOffsetCheck[(((u32) & (((MessengerFlightState *)0)->phase)) == 8) ? 1 : -1];
typedef char MessengerFlightTargetOffsetCheck[(((u32) & (((MessengerFlightState *)0)->linkedId)) == 0xC) ? 1 : -1];
typedef char MessengerFlightAnimationOffsetCheck[(((u32) & (((MessengerFlightState *)0)->animation)) == 0x10) ? 1 : -1];
typedef char MessengerFlightVelocityOffsetCheck[(((u32) & (((MessengerFlightState *)0)->velocity)) == 0x1C) ? 1 : -1];
typedef char MessengerFlightMessageOffsetCheck[(((u32) & (((MessengerFlightState *)0)->message)) == 0x28) ? 1 : -1];

/// Advance the messenger position using its current velocity and elapsed frame time.
/// \param object Messenger projectile with initialized state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80025FAC (0x80025FAC).
void messengerAdvancePosition(u8 *object) {
    MessengerFlightState *state = ((RuntimeObjectPrefix *)object)->state.pointer;
    s32 i;

    for (i = 0; i < 3; i++) {
        ((RuntimeObjectPrefix *)object)->position[i] += state->velocity[i] * gRuntimeFrameSeconds;
    }
}

/// Integer appearance animation used when messenger state is initialized.
/// \note Original identifier: \c D_8006B4E0 (0x8006B4E0).
extern s32 gMessengerFlightAnimation;


/// Initialize messenger projectile state and animation.
/// \param object Projectile receiving owned state; previous state must already be released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026024 (0x80026024).
void messengerInitializeObject(u8 *object) {
    MessengerFlightState *projectileState;
    MessengerFlightState *allocation;

    allocation = heapAllocLocal((HpHdr *)&gGameHeap, sizeof(MessengerFlightState), 0xCA);
    projectileState = allocation;
    _bzero(allocation, sizeof(MessengerFlightState));
    projectileState->remainingTime = 0.0f;
    projectileState->phase = 0.0f;
    animationIntegerTrackInitialize(&projectileState->animation, (ValueKey *)(gMessengerFlightAnimation));
    ((RuntimeObjectPrefix *)object)->state.pointer = projectileState;
}

/// Release messenger projectile state.
/// \param object Messenger projectile with initialized state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026090 (0x80026090).
void messengerDestroyObject(u8 *object) {
    MessengerFlightState *state = ((RuntimeObjectPrefix *)object)->state.pointer;

    heapFreeLocal(&gGameHeap, state);
}

/// Begin homing when the messenger launch period has ended.
/// \param object Messenger projectile with initialized state.
/// \param unused Unused animation status argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800260B8 (0x800260B8).
void messengerUpdateLaunch(u8 *object, s32 unused) {
    u8 *projectileState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((MessengerFlightState *)projectileState)->remainingTime < 0.0f) {
        messengerBeginHoming(object);
    }
}

void messengerBeginRebound(u8 *);
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern u8 gPlayerState[];
/// Deliver a messenger script message and trigger the recipient and projectile responses.
/// \param object Messenger projectile with initialized state.
/// \param source Contacted object part used to select recipient feedback.
/// \param target Actor receiving the message, or an unsupported object that causes the messenger to dissipate.
/// \param mode Recipient contact mode; mode 1 rebounds after delivery, while other modes consume the messenger.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800260F8 (0x800260F8).
/// \note Contact may alter the prepared command according to recipient mode.
/// The actor mailbox can reject delivery when an unread message is preserved.
/// \see voiceCommandLaunchEffect
/// \see actorPostScriptMessage
void messengerDeliver(u8 *object, u8 *source, u8 *target, s32 mode) {
    s32 kind;
    u8 *state;
    s32 force;
    s32 animation;
    s32 sourceKind;
    kind = ((RuntimeObjectPrefix *)target)->kind;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (kind <= 0 || kind >= 0x26) {
        messengerBeginDissipation((f32 *)object);
        return;
    }
    if (mode == 1)
        ((MessengerFlightState *)state)->message.type = 0;
    force = target == (u8 *)gPikachuActor ? 1 : 0;
    actorPostScriptMessage((s32)target, ((MessengerFlightState *)state)->message, force);
    if (target == (u8 *)gPikachuActor) {
        *(s32 *)(gPlayerState + 0xFC) += 1;
        actorAdjustSecondaryMeter((u8 *)gPikachuActor, 1.0f);
        sourceKind = ((RuntimeObjectPrefix *)source)->kind;
        if (sourceKind == 0x2C || sourceKind == 0x2E)
            animation = sourceKind;
        else {
            animation = randomNext() * 2.3283064365386963e-10 > 0.5 ? 0x2C : 0x2E;
        }
        actorStartPartShake((u8 *)gPikachuActor, animation);
    }
    if (mode == 1) {
        messengerBeginRebound(object);
        return;
    }
    objectPlaySpatialSound(object, 0xBB, 0.0f, 500.0f);
    objectRequestHierarchyDestruction(object);
    gTrackedMessengerHandle = 0;
}

f32 mathMapRangeClamped(f32, f32, f32, f32, f32);
/// Guide the messenger toward its linked recipient and handle delivery or loss of the target.
/// \pre Frame duration must be positive, and a live linked part must have a valid owning object.
/// \param object Messenger projectile with initialized state.
/// \param unused Unused animation status argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800262BC (0x800262BC).
void messengerUpdateHoming(u8 *object, s32 unused) {
    s32 property;
    u8 *linked;
    f32 delta[3];
    f32 distance;
    f32 contactDistance;
    s32 mode;
    u8 *target;
    f32 curve;
    f32 step;
    MessengerFlightState *state;
    s32 component;
    state = ((RuntimeObjectPrefix *)object)->state.pointer;
    if (state->remainingTime < 0.0f) {
        messengerBeginDissipation((f32 *)object);
    }
    linked = objectResolveAvailableHandle(state->linkedId);
    if (linked == 0) {
        messengerBeginDissipation((f32 *)object);
        return;
    }
    target = ((RuntimeObjectPrefix *)linked)->root.bytes;
    mode = messengerGetActorContactMode(target);
    delta[0] = (((RuntimeObjectPrefix *)linked)->worldPosition[0]) - (*((f32 *)(object + 0x18)));
    delta[1] = (((RuntimeObjectPrefix *)linked)->worldPosition[1]) - (*((f32 *)(object + 0x1C)));
    delta[2] = (((RuntimeObjectPrefix *)linked)->worldPosition[2]) - (*((f32 *)(object + 0x20)));
    contactDistance = -1.0f;
    property = ((RuntimeObjectPrefix *)target)->kind;
    if ((((property > 0) && (property < 0x26)) && ((mode == 3) || (mode == 2))) &&
        (((property = actionGetAttributes(target), property & 0x10000)) || ((mode == 3) && (!(property & 0x8000))))) {
        contactDistance = vector3Length((f32 *)delta);
        delta[0] += 2.0f * mathCosDegrees(state->phase);
        delta[1] += ((2.0f * mathSinDegrees(2.0f * state->phase)) * 0.4f) + 2.0f;
        delta[2] += 2.0f * mathSinDegrees(state->phase);
        state->phase += 200.0f * gRuntimeFrameSeconds;
        if (state->phase < 0.0f) {
            do {
                state->phase += 360.0f;
            } while (state->phase < 0.0f);
        }
        if (state->phase >= 360.0f) {
            do {
                state->phase -= 360.0f;
            } while (state->phase >= 360.0f);
        }
    }
    distance = vector3Normalize((f32 *)delta);
    if (contactDistance < 0.0f) {
        contactDistance = distance;
    }
    if (contactDistance < ((*((f32 *)(linked + 0x24))) + 1.0f)) {
        messengerDeliver(object, linked, target, mode);
        return;
    }
    if (distance > 200.0f) {
        messengerBeginDissipation((f32 *)object);
        return;
    }
    if (state->remainingTime <= 1.5f) {
        state->velocity[0] = 0.0f;
        state->velocity[1] = 0.0f;
        state->velocity[2] = 0.0f;
    }
    curve = mathMapRangeClamped(1.5f, 8.0f, 3.0f, 0.4f, state->remainingTime);
    step = ((curve < distance) ? (curve) : (distance)) / gRuntimeFrameSeconds;
    curve = mathMapRangeClamped(1.0f, 0, 30.0f, 0.99f, distance);
    for (component = 0; component < 3; component++) {
        state->velocity[component] = (state->velocity[component] * curve) + (delta[component] * step);
    }
}

/// Check a flying messenger for contact and begin dissipation when its lifetime expires.
/// \param object Messenger projectile with initialized state.
/// \param unused Unused animation status argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026668 (0x80026668).
void messengerUpdateCollisionFlight(u8 *object, s32 unused) {
    u8 *sp18;
    u8 *overlappingObject;
    u8 *projectileState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((MessengerFlightState *)projectileState)->remainingTime < 0.0f) {
        messengerBeginDissipation((f32 *)object);
    }
    overlappingObject = collisionGridFindOverlap(object);
    if (overlappingObject != NULL) {
        sp18 = overlappingObject;
        messengerDeliver(object, sp18, ((RuntimeObjectPrefix *)sp18)->root.bytes,
                         messengerGetActorContactMode(((RuntimeObjectPrefix *)overlappingObject)->root.bytes));
    }
}


/// Advance messenger dissipation and request removal when it finishes.
/// \param object Messenger projectile with initialized state.
/// \param animationStatus Animation completion status; 1 requests removal immediately.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800266DC (0x800266DC).
void messengerUpdateDissipation(u8 *object, s32 animationStatus) {
    MessengerFlightState *state = ((RuntimeObjectPrefix *)object)->state.pointer;
    f32 k = 0.8f;

    state->velocity[0] = state->velocity[0] * k;
    state->velocity[1] = state->velocity[1] + 0.1f;
    state->velocity[2] = state->velocity[2] * k;
    if (animationStatus == 1 || state->remainingTime < 0.0f) {
        objectRequestHierarchyDestruction(object);
    }
}

/// Begin dissipation when the messenger rebound period ends.
/// \param object Messenger projectile with initialized state.
/// \param unused Unused animation status argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026758 (0x80026758).
void messengerUpdateRebound(u8 *object, s32 unused) {
    u8 *projectileState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((MessengerFlightState *)projectileState)->remainingTime < 0.0f) {
        messengerBeginDissipation((f32 *)object);
    }
}

void messengerUpdateHoming(u8 *, s32);
void messengerUpdateDissipation(u8 *, s32);

/// Advance messenger lifetime, animation, motion, and delivery behavior.
/// \param object Messenger projectile with initialized state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026798 (0x80026798).
void messengerUpdate(u8 *object) {
    MessengerFlightState *flightState;
    s32 animationValue;
    s32 animationStatus;

    flightState = ((RuntimeObjectPrefix *)object)->state.pointer;
    flightState->remainingTime = flightState->remainingTime - gRuntimeFrameSeconds;
    animationStatus = animationIntegerTrackEvaluate(&flightState->animation, &animationValue, gRuntimeFrameSeconds);
    switch ((u32)flightState->mode) {
    case MESSENGER_LAUNCH:
        messengerUpdateLaunch(object, animationStatus);
        break;
    case MESSENGER_HOMING:
        messengerUpdateHoming(object, animationStatus);
        break;
    case MESSENGER_DISSIPATING:
        messengerUpdateDissipation(object, animationStatus);
        break;
    case MESSENGER_REBOUND:
        messengerUpdateRebound(object, animationStatus);
        break;
    case MESSENGER_COLLISION_FLIGHT:
        messengerUpdateCollisionFlight(object, animationStatus);
        break;
    default:
        TRACE("");
        TRACE("");
        TRACE("ERROR: Unknown status:%d\n", (u32)flightState->mode);
        break;
    }
    messengerAdvancePosition(object);
    objectOrientAlongCameraView(object);
}


/// Prepare messenger opacity for drawing.
/// \param object Messenger projectile with initialized state.
/// \returns A borrowed address of the messenger model display list.
/// \note Original decompilation identifier: \c func_80026870 (0x80026870).
s32 messengerPrepareDraw(u8 *object) {
    u8 *p = ((RuntimeObjectPrefix *)object)->state.bytes;

    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 0xFF, 0xFF, 0xFF, *(s16 *)(p + 0x18));
    return *(s32 *)(object + 0x30);
}

/// Start the messenger homing phase.
/// \param object Messenger projectile with initialized state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800268AC (0x800268AC).
void messengerBeginHoming(u8 *object) {
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    ((MessengerFlightState *)objectState)->mode = MESSENGER_HOMING;
    ((MessengerFlightState *)objectState)->phase = 0.0f;
    ((MessengerFlightState *)objectState)->remainingTime = 3.0f;
}

/// Begin the messenger dissipation animation, double its scale, and release its tracked status.
/// \param object Messenger projectile with initialized state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800268D0 (0x800268D0).
void messengerBeginDissipation(f32 *object) {
    f32 doubledScale;
    u8 *projectileState;
    projectileState = ((RuntimeObjectPrefix *)object)->state.bytes;
    ((MessengerFlightState *)projectileState)->mode = MESSENGER_DISSIPATING;
    ((MessengerFlightState *)projectileState)->remainingTime = 0.35f;
    animationIntegerTrackInitialize(&((MessengerFlightState *)projectileState)->animation, (ValueKey *)((s32)gMessengerDissipationAnimation));
    objectPlaySpatialSound((u8 *)object, 0xBD, 0.0f, 500.0f);
    if (gTrackedMessengerHandle == ((RuntimeObjectPrefix *)object)->handle) {
        gTrackedMessengerHandle = 0;
    }
    object[0] = 2.0f * object[0];
    doubledScale = object[0];
    object[2] = doubledScale;
    object[1] = doubledScale;
}

u8 *actorGetDefaultPart(s32);
/// Begin the messenger rebound after delivery, including applicable recipient feedback.
/// \param object Messenger projectile with initialized state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026964 (0x80026964).
void messengerBeginRebound(u8 *object) {
    s32 count;
    u8 *target;
    u8 *state;
    u8 *linked;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    ((MessengerFlightState *)state)->mode = MESSENGER_REBOUND;
    ((MessengerFlightState *)state)->remainingTime = 0.3f;
    objectPlaySpatialSound(object, 0xBC, 0.0f, 250.0f);
    ((MessengerFlightState *)state)->velocity[1] = -((MessengerFlightState *)state)->velocity[1];
    if (gTrackedMessengerHandle == ((RuntimeObjectPrefix *)object)->handle)
        gTrackedMessengerHandle = 0;
    linked = objectResolveAvailableHandle(((MessengerFlightState *)state)->linkedId);
    if (linked != NULL && gPikachuActor == *(s32 *)(linked + 0x50)) {
        target = actorGetDefaultPart((s32)gPikachuActor);
        count = (s32)mathMapRangeClamped(0.0f, 2.0f, 4000.0f, 6.0f, (f32) * (s32 *)(state + 0x34));
        linkedEffectCreateChain(LINKED_EFFECT_FALLING, ((RuntimeObjectPrefix *)object)->worldPosition[0], ((RuntimeObjectPrefix *)object)->worldPosition[1], ((RuntimeObjectPrefix *)object)->worldPosition[2], 0.05f,
                      ((RuntimeObjectPrefix *)target)->handle, 0.7f, 0.5f, 5.0f, 0.03f, 0.03f, 0xC2, 1, count);
    }
}


/// Reset the tracked messenger projectile reference.
/// \returns Nothing.
/// \note Original decompilation identifier: \c myfree (0x80026AAC).
void messengerResetTrackedObject(void) {
    gTrackedMessengerHandle = 0;
}

/// Hierarchy template used to create script-message projectiles.
/// \note Original identifier: \c D_80099964 (0x80099964).
extern u8 gMessengerObjectTemplate[];
/// Model display list used for direct collision-based messenger flight.
/// \note Original identifier: \c D_1003560 (0x01003560).
extern u8 gMessengerDirectFlightDisplayList[];
/// Model display list used for homing messenger flight.
/// \note Original identifier: \c D_10033E8 (0x010033E8).
extern u8 gMessengerHomingDisplayList[];
/// Launch a script-message projectile and begin dissipation of the previously tracked messenger.
/// \param id Handle of the recipient object part.
/// \param position Three-component launch position.
/// \param velocity Three-component initial velocity.
/// \param scale Uniform projectile scale.
/// \param message Script message to deliver to the recipient.
/// \param mode Zero for homing delivery after launch, or nonzero for collision-based flight.
/// \param flags Nonzero to enable drawing in map and demonstration modes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026AB8 (0x80026AB8).
void messengerCreate(s32 id, f32 *position, f32 *velocity, f32 scale, ScriptMessage message, s32 mode, s32 flags) {
    u8 *object;
    MessengerFlightState *state;
    u8 *previous;
    object = objectCreateHierarchyFromTemplate((ObjectTemplate *)gMessengerObjectTemplate);
    if (flags != 0)
        objectSetHierarchyFlags(object, 0x6000);
    ((RuntimeObjectPrefix *)object)->scale[0] = scale;
    ((RuntimeObjectPrefix *)object)->scale[1] = scale;
    ((RuntimeObjectPrefix *)object)->scale[2] = scale;
    state = ((RuntimeObjectPrefix *)object)->state.pointer;
    if (mode != 0) {
        state->mode = MESSENGER_COLLISION_FLIGHT;
        state->remainingTime = 5.0f;
        *(u8 **)(object + 0x30) = gMessengerDirectFlightDisplayList;
    } else {
        state->mode = MESSENGER_LAUNCH;
        state->remainingTime = 0.1f;
        *(u8 **)(object + 0x30) = gMessengerHomingDisplayList;
    }
    state->linkedId = id;
    state->message = message;
    state->velocity[0] = velocity[0];
    state->velocity[1] = velocity[1];
    state->velocity[2] = velocity[2];
    ((RuntimeObjectPrefix *)object)->position[0] = position[0];
    ((RuntimeObjectPrefix *)object)->position[1] = position[1];
    ((RuntimeObjectPrefix *)object)->position[2] = position[2];
    ((RuntimeObjectPrefix *)object)->worldPosition[0] = position[0];
    ((RuntimeObjectPrefix *)object)->worldPosition[1] = position[1];
    ((RuntimeObjectPrefix *)object)->worldPosition[2] = position[2];
    previous = objectFindLiveByHandle(gTrackedMessengerHandle);
    if (previous != NULL)
        messengerBeginDissipation((f32 *)previous);
    gTrackedMessengerHandle = ((RuntimeObjectPrefix *)object)->handle;
    objectScaleHierarchyBounds(object, 1.0f);
    objectRegisterActiveHierarchy(object);
    objectPlaySpatialSound(object, 0xBA, 0.0f, 500.0f);
    TRACE("sendMessenger(%d, (%.3f, %.3f, %.3f), (%.3f, %.3f, %.3f), %.3f,\n\t\t(%d, %d, 0x%x, %d, %d), %d, %d)\n", id,
          position[0], position[1], position[2], velocity[0], velocity[1], velocity[2], scale, message.type,
          message.arg0, message.arg1, message.arg2, message.arg3, mode, flags);
}

/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
void matrix33Transpose(f32 *);
void matrix33TransformVector(f32 *, f32 *, f32 *);

/* Store a constant vector in one expression. Every lane store comes from one
 * source line, which lets as1 order them as the ROM does. */
#define VEC3_SET(v, xv, yv, zv) ((v)[0] = (xv), (v)[1] = (yv), (v)[2] = (zv))

/// Choose a messenger launch position and velocity relative to the current camera.
/// \pre The current camera and its orientation must be initialized.
/// \param position Output three-component launch position in world coordinates.
/// \param velocity Output three-component launch velocity in world coordinates.
/// \param mode Zero for a curved homing launch, or nonzero for a faster direct launch.
/// \returns Nothing.
///
/// \note Portability: the compiled-out null check is retained for IDO register allocation.
/// \note Original decompilation identifier: \c func_80026C58 (0x80026C58).
void messengerGetCameraLaunch(f32 *position, f32 *velocity, s32 mode) {
    CameraMatrix matrix;
    f32 *basis;

    matrix = gRenderCameras[gRenderCameraIndex].view;
    basis = matrix.values;
    if (basis == NULL) {
    }
    matrix33Transpose(basis);
    VEC3_SET(position, 1.0f, -1.0f, -3.0f);
    matrix33TransformVector(basis, position, position);
    position[0] += gRenderCameras[gRenderCameraIndex].eye[0];
    position[1] += gRenderCameras[gRenderCameraIndex].eye[1];
    position[2] += gRenderCameras[gRenderCameraIndex].eye[2];
    if (mode != 0) {
        VEC3_SET(velocity, 0.0f, 1.0f, -75.0f);
    } else {
        VEC3_SET(velocity, -1.0f, 7.0f, -15.0f);
    }
    matrix33TransformVector(basis, velocity, velocity);
}

#undef VEC3_SET
