#include "render_state.h"
#include "game_heap.h"
#include "font_cache.h"
#include "graphics_tasks.h"
#include "heap_free.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "text_image_create.h"
#include "font_image.h"
#include "text_draw.h"
#include "heap_alloc.h"
#include "text_image.h"
#include "heap_types.h"
#include "scheduler_types.h"
#include "rom_cache.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on
/* The same trace in a leaf function: a dead call would still make IDO give
 * the function an outgoing-argument frame, so the arguments become a
 * discarded comma expression that keeps only the format string. */
#define TRACE_LEAF (void)

const u16 gDefaultTextPaletteRgba5551[16] = {
    0x0000, 0x003F, 0x07C1, 0x07FF, 0xF801, 0xF83F, 0xFFC1, 0xDEF7,
    0x0843, 0x18C7, 0x39CF, 0x5AD7, 0x7BDF, 0x9CE7, 0xBDEF, 0xFFFF,
};

/// Owned pool of reusable scheduler graphics-task records.
/// \note Original identifier: \c D_800F2980 (0x800F2980).
extern SchedulerTask *gGraphicsTaskPool;
/// Number of reusable graphics tasks in the pool.
/// \note Original identifier: \c D_800F2984 (0x800F2984).
extern s32 gGraphicsTaskCapacity;
/// Next reusable graphics-task slot for submission.
/// \note Original identifier: \c D_800F2988 (0x800F2988).
extern s32 gGraphicsNextTaskIndex;
/// Borrowed scheduler command queue address for graphics task submission.
/// \note Original identifier: \c D_800F2990 (0x800F2990).
extern OSMesgQueue *gGraphicsSchedulerQueue;
/// Borrowed framebuffer pair used for drawing and presentation.
/// \note Original identifier: \c D_800F2998 (0x800F2998).
extern void *gGraphicsFramebuffers[2];
/// Owned graphics microcode output-buffer address.
/// \note Original identifier: \c D_800F29A0 (0x800F29A0).
extern u64 *gGraphicsOutputBuffer;
/// Graphics output-buffer capacity in sixty-four-bit words.
/// \note Original identifier: \c D_800F29A4 (0x800F29A4).
extern s32 gGraphicsOutputCapacityWords;
/// Owned DRAM stack address supplied to graphics microcode.
/// \note Original identifier: \c D_800F29A8 (0x800F29A8).
extern u64 *gGraphicsMicrocodeStack;
/// Owned storage address for suspended graphics microcode state.
/// \note Original identifier: \c D_800F29AC (0x800F29AC).
extern u64 *gGraphicsYieldBuffer;

/// Initialize graphics task submission and its framebuffer pair.
/// \param schedulerQueue Borrowed scheduler graphics-task queue address; must remain valid while tasks can be submitted.
/// \param firstFramebuffer Borrowed first framebuffer address, valid throughout task submission and presentation.
/// \param secondFramebuffer Borrowed second framebuffer address, valid throughout task submission and presentation.
/// \param taskCount Positive reusable-task count whose allocation size fits an unsigned 32-bit value.
/// \param outputCapacity Positive graphics output-buffer capacity in 64-bit words whose byte size fits a signed 32-bit value.
/// \returns Borrowed pointer to the globally owned graphics task pool.
/// \note Original decompilation identifier: \c func_8003C500 (0x8003C500).
SchedulerTask *graphicsTaskInitialize(OSMesgQueue *schedulerQueue, void *firstFramebuffer, void *secondFramebuffer, s32 taskCount, s32 outputCapacity) {
    gGraphicsSchedulerQueue = schedulerQueue;
    gGraphicsTaskCapacity = taskCount;
    gGraphicsNextTaskIndex = 0;
    gGraphicsTaskPool = heapAllocLocal((HpHdr *)&gGameHeap, (u32)taskCount * sizeof(SchedulerTask), 0x4D);
    gGraphicsFramebufferIndex = 0;
    gGraphicsFramebuffers[0] = firstFramebuffer;
    gGraphicsFramebuffers[1] = secondFramebuffer;
    gGraphicsMicrocodeStack = heapAllocLocal((HpHdr *)&gGameHeap, 0x400, 0x55);
    gGraphicsYieldBuffer = heapAllocLocal((HpHdr *)&gGameHeap, 0xC00, 0x59);
    gGraphicsOutputCapacityWords = outputCapacity;
    gGraphicsOutputBuffer = heapAllocLocal((HpHdr *)&gGameHeap, (u32)outputCapacity * 8U, 0x5E);
    return gGraphicsTaskPool;
}

/// Borrowed graphics microcode resources and output-buffer policy.
typedef struct GfxMicrocode {
    u64 *text; ///< Microcode instructions retained through task completion.
    u64 *data; ///< Microcode data retained through task completion.
    s32 fifo; ///< Nonzero when the output descriptor carries the FIFO end address.
    u32 dataSize; ///< Retained descriptor metadata; submission uses the SDK fixed data size.
} GfxMicrocode;
typedef char GfxMicrocodeSizeCheck[(sizeof(GfxMicrocode) == 0x10) ? 1 : -1];
/// Graphics microcode text, data and output-policy descriptors.
/// \note Original identifier: \c D_8009D41C (0x8009D41C).
extern GfxMicrocode gGraphicsMicrocodeConfigurations[];
/// Audio RSP microcode text start; also marks the end of the RSP boot code.
/// \note Original identifier: \c D_80060120 (0x80060120).
extern u64 gAudioRspMicrocodeTextStart[];
/// Scheduler completion message for a graphics submission without frame presentation.
/// \note Original identifier: \c D_8009D414 (0x8009D414).
/// Scheduler completion message for the frame presentation submission.
/// \note Original identifier: \c D_8009D418 (0x8009D418).
extern s32 gGraphicsPartialCompletionMessage, gGraphicsFrameCompletionMessage;
/// Finish and submit a graphics command list to the scheduler.
/// \param microcode Valid graphics microcode table index.
/// \param flags Scheduler task flags controlling completion and framebuffer presentation.
/// \param start Command-list start, advanced to the next list after submission; command storage must remain valid until task completion.
/// \param end Command-list write cursor with room for two additional commands, advanced past the list terminator.
/// \param reply Borrowed completion-message queue, valid until task completion.
/// \pre Graphics submission is initialized and the next reusable task has completed its previous use.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003C5EC (0x8003C5EC).
void graphicsTaskSubmit(s32 microcode, s32 flags, Gfx **start, Gfx **end, OSMesgQueue *reply) {
    SchedulerTask *task;
    u32 size;
    gDPFullSync((*end)++);
    gSPEndDisplayList((*end)++);
    size = (u8 *)*end - (u8 *)*start;
    task = gGraphicsTaskPool + gGraphicsNextTaskIndex;
    task->task.t.data_ptr = (u64 *)*start;
    task->task.t.data_size = size;
    task->task.t.type = M_GFXTASK;
    task->task.t.flags = 0;
    task->task.t.ucode_boot = (u64 *)rspbootTextStart;
    task->task.t.ucode_boot_size = (u8 *)gAudioRspMicrocodeTextStart - (u8 *)rspbootTextStart;
    task->task.t.ucode = gGraphicsMicrocodeConfigurations[microcode].text;
    task->task.t.ucode_data = gGraphicsMicrocodeConfigurations[microcode].data;
    task->task.t.ucode_data_size = SP_UCODE_DATA_SIZE;
    task->task.t.dram_stack = (u64 *)gGraphicsMicrocodeStack;
    task->task.t.dram_stack_size = SP_DRAM_STACK_SIZE8;
    task->task.t.output_buff = (u64 *)gGraphicsOutputBuffer;
    if (gGraphicsMicrocodeConfigurations[microcode].fifo) {
        task->task.t.output_buff_size = gGraphicsOutputBuffer + gGraphicsOutputCapacityWords;
    } else {
        task->task.t.output_buff_size = (u64 *)gGraphicsOutputCapacityWords;
    }
    task->task.t.yield_data_ptr = (u64 *)gGraphicsYieldBuffer;
    task->task.t.yield_data_size = OS_YIELD_DATA_SIZE;
    task->next = NULL;
    task->flags = flags;
    task->reply = reply;
    size = flags & 0x40;
    if (size)
        task->message = &gGraphicsFrameCompletionMessage;
    else
        task->message = &gGraphicsPartialCompletionMessage;
    task->framebuffer = gGraphicsFramebuffers[gGraphicsFramebufferIndex];
    osWritebackDCacheAll();
    osSendMesg(gGraphicsSchedulerQueue, task, OS_MESG_BLOCK);
    if (size)
        gGraphicsFramebufferIndex ^= 1;
    if (++gGraphicsNextTaskIndex >= gGraphicsTaskCapacity)
        gGraphicsNextTaskIndex = 0;
    *start = *end;
}

/// Interior view beginning at a microcode descriptor's data pointer.
/// The trailing bytes preserve the table stride and overlap the next descriptor;
/// this view is borrowed and cannot be allocated as a complete descriptor.
typedef struct MicrocodeDataView {
    u8 *data; ///< Borrowed microcode data searched for a version identifier.
    u8 strideRemainder[12]; ///< Unaccessed bytes through the next data-pointer view.
} MicrocodeDataView;
typedef char MicrocodeDataViewSizeCheck[(sizeof(MicrocodeDataView) == 0x10) ? 1 : -1];
/// Microcode descriptor table viewed at its data pointer, with sixteen bytes per entry.
/// \note Original identifier: \c D_8009D420 (0x8009D420).
extern MicrocodeDataView gGraphicsMicrocodeDataEntries[];
/// Identification prefix searched for in graphics microcode data.
/// \note Original identifier: \c D_8009D43C (0x8009D43C).
extern u8 gGraphicsMicrocodeVersionPrefix[];
/// Fallback version text when no microcode identification is found.
/// \note Original identifier: \c D_8009D44C (0x8009D44C).
extern u8 gGraphicsUnknownMicrocodeVersion[];

/// Find the identifying version string for graphics microcode.
/// \param index Valid graphics microcode table index.
/// \returns Borrowed microcode identification string, or the static fallback string if none is found.
/// \note Original decompilation identifier: \c func_8003C808 (0x8003C808).
u8 *graphicsMicrocodeGetVersionString(s32 index) {
    u8 *p = gGraphicsMicrocodeDataEntries[index].data;
    s32 i;
    s32 j;

    for (i = 0; i < 0x1000; i++, p++) {
        for (j = 0; p[j] == gGraphicsMicrocodeVersionPrefix[j]; j++) {
        }
        if (gGraphicsMicrocodeVersionPrefix[j] == 0) {
            return p;
        }
    }
    return gGraphicsUnknownMicrocodeVersion;
}

/// Provide a graphics hook that performs no action.
/// \param unused Unused argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003C894 (0x8003C894).
void graphicsNoOp(s32 unused) {
}

/// Persistent video-mode copy configured for the active framebuffer dimensions.
/// \note Original identifier: \c D_800F29B0 (0x800F29B0).
extern OSViMode gVideoConfiguredMode;
/// Separate symbol for the configured video mode width register.
/// \note Original identifier: \c D_800F29B8 (0x800F29B8).
extern u32 gVideoConfiguredWidthRegister; /* D_800F29B0.comRegs.width */
/// Separate symbol for the configured video mode horizontal scale register.
/// \note Original identifier: \c D_800F29D0 (0x800F29D0).
extern u32 gVideoConfiguredHorizontalScaleRegister; /* D_800F29B0.comRegs.xScale */
/// Configured framebuffer width in pixels used by diagnostic text drawing.
/// \note Original identifier: \c D_8009D410 (0x8009D410).
extern s32 gVideoFramebufferWidth;

/// Configure the video mode for a framebuffer and blank the display until rendering begins.
/// \param mode Base video mode to configure.
/// \param width Positive framebuffer width in pixels; video-mode integer calculations must remain representable.
/// \param height Positive framebuffer height in pixels.
/// \param depth Supported framebuffer color depth, 16 or 32 bits per pixel.
/// \returns Nothing.
///
/// \note NONPORTABLE: comRegs.width and xScale are written through their own symbols (D_800F29B8,
/// D_800F29D0), aliases of D_800F29B0's fields
/// \note Original decompilation identifier: \c func_8003C89C (0x8003C89C).
void videoConfigureMode(OSViMode mode, s32 width, s32 height, s32 depth) {
    s32 lines = 240;
    s32 origin;

    gVideoConfiguredMode = mode;
    gVideoConfiguredWidthRegister = width * height / lines;
    gVideoConfiguredHorizontalScaleRegister = (width << 10) / 640;
    origin = depth / 8 * width;
    gVideoConfiguredMode.fldRegs[0].origin = origin;
    gVideoConfiguredMode.fldRegs[1].origin = origin * height / lines;
    osViSetMode(&gVideoConfiguredMode);
    osViBlack(1);
    gVideoFramebufferWidth = width;
}

/// Configure a viewport for an inclusive pixel rectangle and calculate its projection aspect ratio.
/// \param viewport Viewport to configure; rectangle-derived scale and translation must fit its signed 16-bit fields.
/// \param x0 Left pixel coordinate.
/// \param y0 Top pixel coordinate.
/// \param x1 Right pixel coordinate, inclusive; at least x0.
/// \param y1 Bottom pixel coordinate, inclusive; at least y0.
/// \param aspectDenominator Nonzero denominator of the aspect correction.
/// \param aspectNumerator Numerator of the aspect correction.
/// \param aspectScale Additional projection aspect scale.
/// \returns Corrected projection aspect ratio.
/// \note Original decompilation identifier: \c func_8003C9F0 (0x8003C9F0).
f32 viewportSetPixelRectangle(Vp *viewport, s32 x0, s32 y0, s32 x1, s32 y1, s32 aspectDenominator, s32 aspectNumerator, f32 aspectScale) {
    s32 w = x1 - x0;
    s32 h = y1 - y0;

    viewport->vp.vscale[0] = w * 2 + 2;
    viewport->vp.vscale[1] = h * 2 + 2;
    viewport->vp.vtrans[3] = 0;
    viewport->vp.vscale[2] = 0x1FF;
    // clang-format off
    viewport->vp.vtrans[0] = x0 * 4 + w * 2 + 2; viewport->vp.vtrans[1] = y0 * 4 + h * 2 + 2;
    // clang-format on
    viewport->vp.vtrans[2] = 0x1FF;
    viewport->vp.vscale[3] = viewport->vp.vtrans[3];
    return (f32)(w + 1) * aspectScale / (f32)(h + 1) * (f32)aspectNumerator / (f32)aspectDenominator;
}

/* 38 packed 5-byte glyphs (0-9, A-Z, blank, ':'), one bit per pixel per row. */
typedef struct {
    u16 data[95];
} DebugFont;
/// Packed diagnostic glyphs for digits, letters, a blank and colon.
/// \note Original identifier: \c D_8009D454 (0x8009D454).
extern DebugFont gDebugTextGlyphs;

/// Draw diagnostic text into both framebuffers.
/// \param text Null-terminated text; letters, digits, and colons are displayed, with other characters left blank. Each character occupies eight by ten pixels.
/// \param x Left screen coordinate in pixels.
/// \param y Top screen coordinate in pixels; the entire text rectangle must fit both initialized 16-bit framebuffers.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CA98 (0x8003CA98).
void debugTextDrawFramebuffers(u8 *text, s32 x, s32 y) {
    DebugFont font = gDebugTextGlyphs;
    s32 i;
    u8 *cursor;
    u8 ch;
    u8 bits;
    s32 row;
    s32 column;
    u16 *output;
    u16 *scanline;
    u16 color;

    if (gVideoFramebufferWidth != 0) {
        for (i = 0; i < 2; i++) {
            output = (u16 *)gGraphicsFramebuffers[i] + gVideoFramebufferWidth * y + x;
            cursor = text;
            if (*cursor != 0) {
                do {
                    ch = *cursor;
                    if (ch >= '0' && ch < ':')
                        ch -= '0';
                    else if (ch >= 'a' && ch < '{')
                        ch -= 'a' - 10;
                    else if (ch >= 'A' && ch < '[')
                        ch -= 'A' - 10;
                    else if (ch == ':')
                        ch = 37;
                    else
                        ch = 36;
                    for (row = 0, scanline = output; row < 5; row++) {
                        column = 0;
                        bits = ((u8(*)[5]) & font)[ch][row];
                        for (; column < 8; column++) {
                            if (bits & 0x80)
                                color = 0xFFFF;
                            else
                                color = 1;
                            scanline[column] = color;
                            (&scanline[column])[gVideoFramebufferWidth] = color;
                            bits <<= 1;
                        }
                        scanline += gVideoFramebufferWidth * 2;
                    }
                    cursor++;
                    output += 8;
                } while (*cursor != 0);
            }
        }
    }
}

/// Default text-area left pixel coordinate.
/// \note Original identifier: \c D_800F2A00 (0x800F2A00).
extern s32 gTextDrawDefaultLeft;
/// Default text-area top pixel coordinate.
/// \note Original identifier: \c D_800F2A04 (0x800F2A04).
extern s32 gTextDrawDefaultTop;
/// Default text-area inclusive right pixel coordinate.
/// \note Original identifier: \c D_800F2A08 (0x800F2A08).
extern s32 gTextDrawDefaultRight;
/// Default text-area inclusive bottom pixel coordinate.
/// \note Original identifier: \c D_800F2A0C (0x800F2A0C).
extern s32 gTextDrawDefaultBottom;
/// Screen width in pixels used to constrain text drawing.
/// \note Original identifier: \c D_800F2A10 (0x800F2A10).
extern s32 gTextDrawScreenWidth;
/// Screen height in pixels used to constrain text drawing.
/// \note Original identifier: \c D_800F2A14 (0x800F2A14).
extern s32 gTextDrawScreenHeight;
/// ROM cache supplying glyph bitmaps for text rendering.
/// \note Original identifier: \c D_800F2A60 (0x800F2A60).
extern RomCache gFontGlyphRomCache;


/// Initialize text drawing for a screen and establish its default text area.
/// \param width Screen width in pixels.
/// \param height Screen height in pixels.
/// \param cachedGlyphCount Positive wide-glyph cache capacity whose byte allocation fits a signed 32-bit value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CD30 (0x8003CD30).
void textDrawInitialize(s32 width, s32 height, s32 cachedGlyphCount) {
    romCacheInitialize(&gFontGlyphRomCache, ((cachedGlyphCount << 7) + 0xF) & ~0xF, 0xF1, 4);
    cachedGlyphCount = 10;
    gTextDrawScreenWidth = width;
    gTextDrawScreenHeight = height;
    gTextDrawDefaultLeft = width / cachedGlyphCount - 1;
    gTextDrawDefaultTop = height / cachedGlyphCount - 1;
    gTextDrawDefaultRight = width - width / cachedGlyphCount;
    gTextDrawDefaultBottom = height - height / cachedGlyphCount;
    textDrawReset();
}

/// Write cursor for the active text-rendering display list.
/// \note Original identifier: \c D_800F2A18 (0x800F2A18).
extern Gfx *gTextDrawDisplayListCursor;
/// Current text red, green, blue and alpha components.
/// \note Original identifier: \c D_800F2A20 (0x800F2A20).
extern s32 gTextDrawColorRgba[4];
/// Green component alias within the current text color.
/// \note Original identifier: \c D_800F2A24 (0x800F2A24).
extern s32 gTextDrawColorGreen;
/// Blue component alias within the current text color.
/// \note Original identifier: \c D_800F2A28 (0x800F2A28).
extern s32 gTextDrawColorBlue;
/// Alpha component alias within the current text color.
/// \note Original identifier: \c D_800F2A2C (0x800F2A2C).
extern s32 gTextDrawColorAlpha;
/// Borrowed palette used for color-indexed text images.
/// \note Original identifier: \c D_800F2A58 (0x800F2A58).
extern const u16 *gTextDrawPalette;
/// Begin appending text drawing commands to a display list using the current text color and default palette.
/// \param displayList Writable command storage with enough room for the requested text drawing, retained until another textDrawBegin call.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CE24 (0x8003CE24).
void textDrawBegin(Gfx *displayList) {
    gTextDrawDisplayListCursor = displayList;
    gDPPipeSync(gTextDrawDisplayListCursor++);
    gDPSetCycleType(gTextDrawDisplayListCursor++, G_CYC_1CYCLE);
    gDPSetTexturePersp(gTextDrawDisplayListCursor++, G_TP_NONE);
    gDPSetTextureConvert(gTextDrawDisplayListCursor++, G_TC_FILT);
    gDPSetTextureFilter(gTextDrawDisplayListCursor++, G_TF_BILERP);
    gDPSetTextureLOD(gTextDrawDisplayListCursor++, G_TL_TILE);
    gDPSetAlphaCompare(gTextDrawDisplayListCursor++, G_AC_NONE);
    gDPSetCombineMode(gTextDrawDisplayListCursor++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    textDrawSetColor(gTextDrawColorRgba[0], gTextDrawColorGreen, gTextDrawColorBlue, gTextDrawColorAlpha);
    gTextDrawPalette = gDefaultTextPaletteRgba5551;
}


/// Retrieve the current text drawing command position.
/// \param cursor Receives the display-list position after commands emitted so far.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CF68 (0x8003CF68).
void textDrawGetDisplayListCursor(Gfx **cursor) {
    *cursor = gTextDrawDisplayListCursor;
}

/// Active text-area left pixel coordinate.
/// \note Original identifier: \c D_800F2A30 (0x800F2A30).
extern s32 gTextDrawAreaLeft;
/// Active text-area top pixel coordinate.
/// \note Original identifier: \c D_800F2A34 (0x800F2A34).
extern s32 gTextDrawAreaTop;
/// Active text-area inclusive right pixel coordinate.
/// \note Original identifier: \c D_800F2A38 (0x800F2A38).
extern s32 gTextDrawAreaRight;
/// Active text-area inclusive bottom pixel coordinate.
/// \note Original identifier: \c D_800F2A3C (0x800F2A3C).
extern s32 gTextDrawAreaBottom;
/// Font-bank index used for single-byte text characters.
/// \note Original identifier: \c D_800F2A48 (0x800F2A48).
extern s32 gTextDrawFontIndex;
/// Text spacing mode: one selects proportional spacing; other values select fixed cells.
/// \note Original identifier: \c D_800F2A4C (0x800F2A4C).
extern s32 gTextDrawSpacingMode;
/// Horizontal scale applied to text rendering and cursor advances.
/// \note Original identifier: \c D_800F2A50 (0x800F2A50).
extern f32 gTextDrawScaleX;
/// Vertical scale applied to text rendering and row advances.
/// \note Original identifier: \c D_800F2A54 (0x800F2A54).
extern f32 gTextDrawScaleY;

/// Restore the default text area, cursor, color, font, spacing, scale, and palette.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CF78 (0x8003CF78).
void textDrawReset(void) {
    gTextDrawColorRgba[0] = 0xFF;
    gTextDrawColorRgba[1] = 0xFF;
    gTextDrawColorRgba[2] = 0xFF;
    gTextDrawColorRgba[3] = 0xFF;
    gTextDrawAreaLeft = gTextDrawDefaultLeft;
    gTextDrawAreaTop = gTextDrawDefaultTop;
    gTextDrawAreaRight = gTextDrawDefaultRight;
    gTextDrawAreaBottom = gTextDrawDefaultBottom;
    textDrawSetCursor(0, 0);
    gTextDrawFontIndex = 0;
    gTextDrawSpacingMode = 1;
    gTextDrawScaleX = 1.0f;
    gTextDrawScaleY = 1.0f;
    gTextDrawPalette = gDefaultTextPaletteRgba5551;
}

/// Current absolute horizontal text position in pixels.
/// \note Original identifier: \c D_800F2A40 (0x800F2A40).
extern s32 gTextDrawCursorX;
#include "libc/xstdio.h"


typedef struct {
    u8 bytes[512];
} TextFormatArgs;
void textDrawWideCharacter(s32);
void textDrawCharacter(u8);
s32 fontConvertJisToShiftJis(s32);
void textDrawFitGlyphAtCursor(s32, s32);

/// Draw formatted text at the current cursor, supporting single-byte characters, wide characters, and newlines.
/// \param format Printf-style format string; the result including its terminator must fit in 512 bytes and contain complete supported characters.
/// \param ... Values used by the format string.
/// \returns Nothing.
///
/// \note Portability/UB: Variadic forwarding reads 512 bytes after the format argument, relying on the original IDO stack layout instead of a portable va_list.
/// \note Original decompilation identifier: \c func_8003D030 (0x8003D030).
void textDrawPrintf(const char *format, ...) {
    u8 *cursor;
    u8 buffer[512];
    s32 wide;
    s32 code;
    s32 *right = &gTextDrawAreaRight;

    sprintf((char *)buffer, format, *(TextFormatArgs *)(&format + 1));
    gDPPipeSync(gTextDrawDisplayListCursor++);
    gDPSetTextureLUT(gTextDrawDisplayListCursor++, G_TT_NONE);
    cursor = buffer;
    while (*cursor) {
        wide = 0;
        if (*cursor & 0x80) {
            wide = 1;
            code = *cursor;
            cursor++;
            code <<= 8;
            code |= *cursor;
            code = fontConvertJisToShiftJis(code);
        }
        switch (wide) {
        case 1:
            textDrawWideCharacter(code);
            break;
        default:
            if (*cursor == '\n')
                gTextDrawCursorX = *right;
            else
                textDrawCharacter(*cursor);
            break;
        }
        cursor++;
    }
    textDrawFitGlyphAtCursor(16, 16);
}

/// Current absolute vertical text position in pixels.
/// \note Original identifier: \c D_800F2A44 (0x800F2A44).
extern s32 gTextDrawCursorY;

/// Adjust the text cursor so the next scaled glyph fits within the text area.
/// \param width Unscaled glyph width in pixels.
/// \param height Positive unscaled glyph height whose scaled row advance rounds to at least one pixel.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D1E0 (0x8003D1E0).
void textDrawFitGlyphAtCursor(s32 width, s32 height) {
    if ((s32)(width * gTextDrawScaleX + 0.5) + gTextDrawCursorX > gTextDrawAreaRight) {
        gTextDrawCursorX = gTextDrawAreaLeft;
        gTextDrawCursorY += (s32)(height * gTextDrawScaleY + 0.5);
    }
    while ((s32)((height - 1) * gTextDrawScaleY + 0.5) + gTextDrawCursorY > gTextDrawAreaBottom) {
        gTextDrawCursorY -= (s32)(height * gTextDrawScaleY + 0.5);
    }
}


/// Set the screen-bounded text area and reset the cursor to its upper-left corner.
/// \param x0 X coordinate of one corner.
/// \param y0 Y coordinate of one corner.
/// \param x1 X coordinate of the opposite corner, inclusive.
/// \param y1 Y coordinate of the opposite corner, inclusive.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D31C (0x8003D31C).
void textDrawSetArea(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 t;

    if (x1 < x0) {
        t = x1;
        x1 = x0;
        x0 = t;
    }
    if (y1 < y0) {
        t = y1;
        y1 = y0;
        y0 = t;
    }
    gTextDrawCursorX = gTextDrawAreaLeft = (x0 < 0) ? 0 : x0;
    gTextDrawCursorY = gTextDrawAreaTop = (y0 < 0) ? 0 : y0;
    gTextDrawAreaRight = (x1 >= gTextDrawScreenWidth) ? gTextDrawScreenWidth - 1 : x1;
    gTextDrawAreaBottom = (y1 >= gTextDrawScreenHeight) ? gTextDrawScreenHeight - 1 : y1;
}

/// Position the text cursor relative to the text area's origin, allowing room for the next glyph.
/// \param x Horizontal offset from the text area's left edge.
/// \param y Vertical offset from the text area's top edge.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D3E4 (0x8003D3E4).
void textDrawSetCursor(s32 x, s32 y) {
    gTextDrawCursorX = gTextDrawAreaLeft + x;
    gTextDrawCursorY = gTextDrawAreaTop + y;
    textDrawFitGlyphAtCursor(0x10, 0x10);
}


/// Set the text cursor's horizontal offset and adjust it to fit the next glyph.
/// \param x Horizontal offset from the text area's left edge.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D430 (0x8003D430).
void textDrawSetCursorX(s32 x) {
    gTextDrawCursorX = gTextDrawAreaLeft + x;
    textDrawFitGlyphAtCursor(0x10, 0x10);
}


/// Set the text cursor's vertical offset and adjust it to fit the next glyph.
/// \param y Vertical offset from the text area's top edge.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D468 (0x8003D468).
void textDrawSetCursorY(s32 y) {
    gTextDrawCursorY = gTextDrawAreaTop + y;
    textDrawFitGlyphAtCursor(0x10, 0x10);
}

/// Set the color and opacity used for text drawing.
/// \param red Red component from zero to 255.
/// \param green Green component from zero to 255.
/// \param blue Blue component from zero to 255.
/// \param alpha Opacity from zero to 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D4A0 (0x8003D4A0).
void textDrawSetColor(s32 red, s32 green, s32 blue, s32 alpha) {
    s32 *col = gTextDrawColorRgba;

    col[0] = red;
    col[1] = green;
    col[2] = blue;
    col[3] = alpha;
    gDPPipeSync(gTextDrawDisplayListCursor++);
    gDPSetPrimColor(gTextDrawDisplayListCursor++, 0, 0, col[0], col[1], col[2], col[3]);
    if (alpha == 0xFF) {
        gDPSetRenderMode(gTextDrawDisplayListCursor++, G_RM_AA_TEX_EDGE, G_RM_AA_TEX_EDGE2);
    } else {
        gDPSetRenderMode(gTextDrawDisplayListCursor++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
    }
}

/// Select the palette used for indexed text images.
/// \param palette Borrowed sixteen-entry RGBA5551 palette, valid until all drawing commands that use it finish.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D580 (0x8003D580).
void textDrawSetPalette(const u16 *palette) {
    gTextDrawPalette = palette;
}


/// Select the font used for single-byte text characters.
/// \param font Valid single-byte font-bank index for the characters to be drawn.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D58C (0x8003D58C).
void textDrawSetFont(s32 font) {
    gTextDrawFontIndex = font;
}


/// Set the horizontal and vertical scale used for text drawing.
/// \param scaleX Finite positive horizontal text scale compatible with representable drawing coordinates and texture steps.
/// \param scaleY Finite positive vertical text scale whose sixteen-pixel row advance rounds to at least one pixel.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D598 (0x8003D598).
void textDrawSetScale(f32 scaleX, f32 scaleY) {
    gTextDrawScaleX = scaleX;
    gTextDrawScaleY = scaleY;
}


/// Select proportional or fixed-cell text spacing.
/// \param mode One for proportional spacing; other values select fixed-cell spacing.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D5AC (0x8003D5AC).
void textDrawSetSpacingMode(s32 mode) {
    gTextDrawSpacingMode = mode;
}


/// Retrieve the default text area's inclusive screen bounds.
/// \param left Receives the left pixel coordinate.
/// \param top Receives the top pixel coordinate.
/// \param right Receives the inclusive right pixel coordinate.
/// \param bottom Receives the inclusive bottom pixel coordinate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D5B8 (0x8003D5B8).
void textDrawGetDefaultArea(s32 *left, s32 *top, s32 *right, s32 *bottom) {
    *left = gTextDrawDefaultLeft;
    *top = gTextDrawDefaultTop;
    *right = gTextDrawDefaultRight;
    *bottom = gTextDrawDefaultBottom;
}

/// Draw an image at the text cursor using the current text scale.
/// \param image Text image to draw.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D5EC (0x8003D5EC).
void textDrawImageAtCursor(TextImage *image) {
    s32 width = image->width;
    s32 height = image->height;
    s32 dx, dy;
    textImageLoadTexture(&gTextDrawDisplayListCursor, image);
    dx = (s32)(1024.0f / gTextDrawScaleX + 0.5);
    dy = (s32)(1024.0f / gTextDrawScaleY + 0.5);
    gSPTextureRectangle(gTextDrawDisplayListCursor++, gTextDrawCursorX * 4, gTextDrawCursorY * 4,
                        ((s32)(width * gTextDrawScaleX) + gTextDrawCursorX) * 4,
                        (gTextDrawCursorY + (s32)(height * gTextDrawScaleY)) * 4, G_TX_RENDERTILE, 0, 0, dx, dy);
}

/// Draw part of a text image in a screen rectangle with scaling and clipping.
/// \param gfx Display-list cursor to append to and advance.
/// \param image Text image to draw.
/// \param x0 Left screen coordinate of the destination rectangle.
/// \param y0 Top screen coordinate of the destination rectangle.
/// \param x1 Inclusive right screen coordinate of the destination rectangle.
/// \param y1 Inclusive bottom screen coordinate of the destination rectangle.
/// \param s Nonnegative horizontal starting coordinate in the source image.
/// \param t Nonnegative vertical starting coordinate in the source image.
/// \param scaleX Positive horizontal image scale.
/// \param scaleY Positive vertical image scale.
/// \returns Nothing.
/// \pre The drawable source width is at most 4096 pixels and scaled texture steps fit signed integers.
/// \note Original decompilation identifier: \c func_8003D764 (0x8003D764).
void textImageDrawRectangle(Gfx **gfx, TextImage *image, s32 x0, s32 y0, s32 x1, s32 y1, s32 s, s32 t, f32 scaleX, f32 scaleY) {
    s32 dsdx;
    s32 dtdy;
    s32 rows;
    s32 height;
    s32 width;

    TRACE("_drawSprite(%p, %p, (%d,%d)-(%d,%d) (%d,%d) (%f,%f)\n", gfx, image, x0, y0, x1, y1, s, t, scaleX, scaleY);
    if (x0 < 0) {
        s = -x0;
        x0 = 0;
    }
    if (y0 < 0) {
        t = -y0;
        y0 = 0;
    }
    if (x1 > gTextDrawScreenWidth) {
        x1 = gTextDrawScreenWidth;
    }
    if (y1 > gTextDrawScreenHeight) {
        y1 = gTextDrawScreenHeight;
    }
    if (s >= image->width || t >= image->height) {
        return;
    }
    width = x1 - x0 + 1;
    if (image->width - s < width) {
        width = image->width - s;
    }
    height = y1 - y0 + 1;
    if (image->height - t < height) {
        height = image->height - t;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    rows = 0x1000 / width;
    dsdx = (s32)(1024.0f / scaleX + 0.5f);
    dtdy = (s32)(1024.0f / scaleY + 0.5f);
    while (height > 0) {
        if (height < rows) {
            rows = height;
        }
        TRACE(" width:%d height:%d ht:%d  uly:%d ult:%d\n", width, height, rows, y0, t);
        textImageLoadRectangle(gfx, image, s, t, s + width - 1, t + rows - 1);
        height -= rows;
        gSPTextureRectangle((*gfx)++, x0 << 2, y0 << 2, (x0 + width) << 2, (y0 + rows) << 2, G_TX_RENDERTILE, s << 5,
                            t << 5, dsdx, dtdy);
        y0 = (s32)(y0 + rows * scaleY);
        t += rows;
    }
}

/// Prepare a rectangular portion of a text image for drawing.
/// \param dl Display-list cursor to append to and advance.
/// \param image Text image containing the source rectangle.
/// \param x0 Horizontal coordinate of the first source corner.
/// \param y0 Vertical coordinate of the first source corner.
/// \param x1 Inclusive horizontal coordinate of the opposite source corner.
/// \param y1 Inclusive vertical coordinate of the opposite source corner.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003DA3C (0x8003DA3C).
void textImageLoadRectangle(Gfx **dl, TextImage *image, s32 x0, s32 y0, s32 x1, s32 y1) {
    TextImage *img = image;
    s32 t;

    if (x1 < x0) {
        t = x1;
        x1 = x0;
        x0 = t;
    }
    if (y1 < y0) {
        t = y0;
        y0 = y1;
        y1 = t;
    }
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > img->width - 1)
        x1 = img->width - 1;
    if (y1 > img->height - 1)
        y1 = img->height - 1;
    if (x1 >= x0 && y1 >= y0) {
        gDPPipeSync((*dl)++);
        if (img != NULL) {
            if (img->format == G_IM_FMT_CI) {
                gDPSetTextureLUT((*dl)++, G_TT_RGBA16);
                gDPLoadTLUT_pal16((*dl)++, 0, gTextDrawPalette);
            } else {
                gDPSetTextureLUT((*dl)++, G_TT_NONE);
            }
            gDPLoadTextureTile_4b((*dl)++, img->pixels, img->format, img->width, img->height, x0, y0, x1, y1, 0,
                                  G_TX_WRAP, G_TX_WRAP, 0, 0, 0, 0);
        }
    }
}


/// Prepare a complete text image for drawing.
/// \param cursor Display-list cursor to append to and advance.
/// \param image Text image to prepare.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003DD84 (0x8003DD84).
void textImageLoadTexture(Gfx **cursor, TextImage *image) {
    textImageLoadRectangle(cursor, image, 0, 0, image->width - 1, image->height - 1);
}

void textImageCreateSystemFont(TextImage **, u8 *, s32);
s32 textImageCreateCustomFont(TextImage **, u8 *, FontImage *);

/// Create a text image using the system font or a supplied font.
/// \param out Receives an owned image to release with textImageDestroy, or null for empty text.
/// \param text Null-terminated encoded text to render.
/// \param font Borrowed custom font, or NULL to use the system font.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003DDC0 (0x8003DDC0).
void textImageCreateFromText(TextImage **out, u8 *text, FontImage *font) {
    if (font == NULL) {
        textImageCreateSystemFont(out, text, (s32)font);
    }
    if (font != NULL) {
        textImageCreateCustomFont(out, text, font);
    }
}

void _bzero(void *, s32);
s32 fontCacheReadGlyph(s32, s32 *);
s32 fontGetGlyphMetrics(s32, s32 *, s32 *, s32 *);
s32 fontGetShiftJisBitmapOffset(s32);

/// Create a text image using the current system font.
/// \param out Receives an owned image to release with textImageDestroy, or null for empty text.
/// \param text Null-terminated text with complete supported characters; at most the first 128 bytes are rendered.
/// \param font Unused font argument.
/// \returns Nothing.
/// \note Portability: The unused local preserves the stack-frame layout required by the ROM.
/// \note Original decompilation identifier: \c func_8003DE00 (0x8003DE00).
void textImageCreateSystemFont(TextImage **out, u8 *text, s32 font) {
    s32 size;
    s32 rows;
    s32 columns;
    s32 code;
    s32 copyBytes;
    s32 clamped;
    s32 maxBytes;
    s32 consumed;
    s32 length;
    s32 row;
    s32 x;
    s32 cellBytes;
    s32 unused;
    s32 glyph;
    u8 *pixels;
    s32 lead;
    u8 *src;
    u8 *dest;
    u8 *cursor;
    TextImage *image;

    rows = 0x10;
    length = strlen(text);
    clamped = length;
    maxBytes = 0x80;
    if (length >= 0x81) {
        clamped = 0x80;
        TRACE("%s\n", text);
        TRACE("Warning:Too Long String(len > %d)\n", 0x80);
    }
    if (clamped == 0) {
        *out = NULL;
        return;
    }
    columns = clamped;
    image = (TextImage *)heapAllocLocal((HpHdr *)&gGameHeap, sizeof(TextImage), 0x27F);
    size = columns * 0x40;
    pixels = (u8 *)heapAllocLocal((HpHdr *)&gGameHeap, size, 0x282);
    _bzero(pixels, size);
    cursor = text;
    x = 0;
    consumed = 0;
    if (*text != 0) {
        do {
            s32 width, height, baseline;
            s32 offset;
            s32 status;
            lead = *cursor;
            TRACE("%s\n", cursor);
            if (lead & 0x80) {
                consumed += 1;
                if (consumed < 0x80) {
                    glyph = *cursor;
                    consumed += 1;
                    cellBytes = 8;
                    code = glyph;
                    cursor += 1;
                    code <<= 8;
                    code |= *cursor;
                    offset = fontGetShiftJisBitmapOffset(fontConvertJisToShiftJis(code));
                } else {
                    cellBytes = 4;
                    offset = fontGetGlyphMetrics(gTextDrawFontIndex * 0xC0, &width, &height, &baseline);
                    TRACE("Warning:Too Long String");
                }
            } else {
                cellBytes = 4;
                consumed += 1;
                offset =
                    fontGetGlyphMetrics((*cursor + (gTextDrawFontIndex * 0xC0)) - 0x20, &width, &height, &baseline);
            }
            TRACE("FONTDATA(0x%04X) NOT EXISTANCE!\n", code);
            glyph = fontCacheReadGlyph(offset, &status);
            if (status == 4) {
                fontCacheWaitForReads();
            }
            dest = pixels + x;
            for (src = (u8 *)glyph, row = 0; row != rows; row++, dest += columns * 4) {
                if (cellBytes == 8) {
                    bcopy(src, dest, 8);
                    src += 8;
                } else {
                    copyBytes = (s32)(width + 1) / 2;
                    _bzero(dest, 4);
                    if ((row >= (0xC - baseline)) && (row < ((height - baseline) + 0xC))) {
                        bcopy(src, ((s32)(7 - width) / 4) + dest, copyBytes);
                        src += copyBytes;
                    }
                }
            }
            cursor += 1, x += cellBytes;
        } while ((*cursor != 0) && (consumed < maxBytes));
        TRACE("COLUMN RANGE ERROR(%d <= %d)!\n", columns * 4, x);
    }
    image->format = 4;
    image->width = (s32)(columns * 8);
    image->height = 0x10;
    image->pixels = pixels;
    *out = image;
}

extern u8 D_7B7A10[];

/// Read glyph image data through the font cache.
/// \param offset Byte offset of the glyph within the font image data.
/// \param status Optional output receiving three for cached data or four when read completion must be synchronized.
/// \returns Borrowed glyph data address, retained until fontCacheReleaseUses; synchronize pending reads before CPU access.
/// \note Original decompilation identifier: \c func_8003E120 (0x8003E120).
s32 fontCacheReadGlyph(s32 offset, s32 *status) {
    s32 sp1C;
    s32 cacheStatus;

    sp1C = 0;
    cacheStatus = romCacheRead(&gFontGlyphRomCache, offset + (s32)D_7B7A10, 0x80, &sp1C);
    if (status != NULL) {
        *status = cacheStatus;
    }
    return sp1C;
}

/// Draw a wide character at the current text cursor and advance the cursor.
/// \param code Valid Shift-JIS character code supported by the system font.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003E17C (0x8003E17C).
void textDrawWideCharacter(s32 code) {
    s32 offset;
    s32 dx, dy;
    offset = fontGetShiftJisBitmapOffset(code);
    gDPLoadTextureBlock_4b(gTextDrawDisplayListCursor++, fontCacheReadGlyph(offset, NULL), G_IM_FMT_I, 16, 16, 0,
                           G_TX_WRAP, G_TX_WRAP, 0, 0, 0, 0);
    textDrawFitGlyphAtCursor(16, 16);
    dx = (s32)(1024.0f / gTextDrawScaleX + 0.5);
    dy = (s32)(1024.0f / gTextDrawScaleY + 0.5);
    gSPTextureRectangle(gTextDrawDisplayListCursor++, gTextDrawCursorX * 4, gTextDrawCursorY * 4,
                        ((s32)(16.0f * gTextDrawScaleX) + gTextDrawCursorX) * 4,
                        (gTextDrawCursorY + (s32)(16.0f * gTextDrawScaleY)) * 4, 0, 0, 0, dx, dy);
    gTextDrawCursorX += (s32)(16.0f * gTextDrawScaleX + 0.5);
}

/// Draw a single-byte character using the current font and spacing, advancing the text cursor.
/// \param character Single-byte character with a valid glyph in the selected font bank.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003E3D8 (0x8003E3D8).
void textDrawCharacter(u8 character) {
    s32 margin;
    s32 paddedWidth;
    s32 width;
    s32 height;
    s32 offset;
    s32 baseline;
    s32 cellWidth;
    s32 dx;
    s32 dy;
    offset = fontGetGlyphMetrics(character + gTextDrawFontIndex * 0xC0 - 0x20, &width, &height, &baseline);
    if (gTextDrawSpacingMode == 1)
        cellWidth = 0;
    else
        cellWidth = 8;
    paddedWidth = (width + 1) & 0xFE;
    gDPLoadTextureTile_4b(gTextDrawDisplayListCursor++, fontCacheReadGlyph(offset, NULL), G_IM_FMT_I, paddedWidth,
                          height, 0, 0, (paddedWidth - 1) << 2, (height - 1) << 2, 0, G_TX_WRAP, G_TX_WRAP, 0, 0, 0, 0);
    if (gTextDrawSpacingMode == 1 && width < 3)
        width = 3;
    textDrawFitGlyphAtCursor(gTextDrawSpacingMode == 1 ? width + 1 : cellWidth, 16);
    margin = (cellWidth - width) / 2;
    if (gTextDrawSpacingMode != 1)
        gTextDrawCursorX += (s32)(margin * gTextDrawScaleX + 0.5);
    baseline -= 12;
    dx = (s32)(1024.0f / gTextDrawScaleX + 0.5);
    dy = (s32)(1024.0f / gTextDrawScaleY + 0.5);
    gSPTextureRectangle(gTextDrawDisplayListCursor++, gTextDrawCursorX * 4,
                        (gTextDrawCursorY + (s32)(-baseline * gTextDrawScaleY + 0.5)) * 4,
                        ((s32)(width * gTextDrawScaleX + 0.5) + gTextDrawCursorX) * 4,
                        (gTextDrawCursorY + (s32)((height - baseline) * gTextDrawScaleY + 0.5)) * 4, 0, 0, 0, dx, dy);
    if (gTextDrawSpacingMode != 1)
        gTextDrawCursorX += (s32)((cellWidth - margin) * gTextDrawScaleX + 0.5);
    else
        gTextDrawCursorX += (s32)((width + 1) * gTextDrawScaleX + 0.5);
}

s32 fontImageMeasureText(u8 *, FontImage *, s32);
s32 fontImageGetCharacterAdvance(s32, FontImage *, s32);
s32 fontImageGetCharacterImageWidth(s32, FontImage *, s32);
s32 textDecodeNextCharacter(u8 **, s32 *);
u8 *fontImageReadGlyph(s32, FontImage *);

/// Create a text image using a supplied font and the current spacing mode.
/// \param out Receives an owned image to release with textImageDestroy, or null for empty text.
/// \param text Null-terminated text containing only complete supported characters and optional position/color controls.
/// \param font Font with positive dimensions and valid glyph data and metrics for every character in text.
/// \returns Address of the owned generated image, or zero for empty text.
/// \pre Position controls and glyph extents must fit the final measured width; image allocation calculations must remain representable.
/// \note Original decompilation identifier: \c func_8003E894 (0x8003E894).
s32 textImageCreateCustomFont(TextImage **out, u8 *text, FontImage *font) {
    s32 glyphWidth = font->width;
    s32 height = font->height;
    s32 color = 15;
    s32 x;
    s32 rowBytes;
    s32 left;
    s32 width;
    s32 bytes;
    s32 advance;
    s32 inkWidth;
    s32 code;
    u8 *pixels;
    u8 *glyph;
    u8 *dest;
    u8 *scanline;
    u8 *cursor;
    s32 row = 0;
    s32 i;
    s32 value;
    TextImage *bitmap;

    bytes = strlen(text);
    if (bytes == 0) {
        *out = NULL;
        return bytes;
    }
    width = fontImageMeasureText(text, font, gTextDrawSpacingMode);
    width = width % 4 + width;
    bytes = width * height * 4 / 8;
    if (bytes > 0x1000) {
        TRACE("ERROR : Too large sprite!\n");
    }
    pixels = (u8 *)heapAllocLocal((HpHdr *)&gGameHeap, bytes, 0x34D);
    _bzero(pixels, bytes);
    x = 0;
    cursor = text;
    if (*text != 0) {
        do {
            code = textDecodeNextCharacter(&cursor, &value);
            if (code == 0xA2A0) {
                x = value * 4 / 8;
            } else if (code == 0xA3A0) {
                color = value;
            } else {
                left = fontImageGetCharacterLeft(code, (FontImage *)font, gTextDrawSpacingMode);
                rowBytes = glyphWidth * 4 / 8;
                inkWidth = fontImageGetCharacterImageWidth(code, font, gTextDrawSpacingMode) * 4 / 8;
                advance = fontImageGetCharacterAdvance(code, font, gTextDrawSpacingMode) * 4 / 8;
                if (code != 0xA1A0) {
                    glyph = fontImageReadGlyph(code, font);
                    row = 0;
                    dest = pixels + x;
                    if (height > 0) {
                        s32 cropOffset = left * 4 / 8;
                        s32 destinationStride = width * 4 / 8;
                        do {
                            scanline = (u8 *)heapAllocLocal((HpHdr *)&gGameHeap, rowBytes, 0x372);
                            bcopy(glyph, scanline, rowBytes);
                            if (color != 15) {
                                for (i = 0; i < rowBytes; i++)
                                    scanline[i] &= ((u32)color << 4) + (u32)color;
                            }
                            bcopy(scanline + cropOffset, dest, inkWidth);
                            heapFreeLocal(&gGameHeap, scanline);
                            row++;
                            dest += destinationStride;
                            glyph += rowBytes;
                        } while (row != height);
                    }
                }
                x += advance;
            }
        } while (*cursor != 0);
    }
    bitmap = (TextImage *)heapAllocLocal((HpHdr *)&gGameHeap, sizeof(TextImage), 0x383);
    bitmap->format = G_IM_FMT_CI;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->pixels = pixels;
    *out = bitmap;
    return (s32)bitmap;
}


s32 fontImageGetGlyphIndex();

/* Acquire glyph texture data and synchronize pending reads before CPU use. */
#define FONT_CACHE_LOAD(src, size, out)                                                                                \
    do {                                                                                                               \
        if (romCacheRead(&gFontGlyphRomCache, (s32)(src), (size), (out)) == 4) {                                       \
            fontCacheWaitForReads();                                                                                   \
        }                                                                                                              \
    } while (0)

/// Obtain the image data for a character in a supplied font.
/// \param code Encoded character to look up.
/// \param font Font containing the character's image data.
/// \returns Borrowed, ready-to-read glyph data address, retained until fontCacheReleaseUses.
/// \note Original decompilation identifier: \c func_8003EC40 (0x8003EC40).
u8 *fontImageReadGlyph(s32 code, FontImage *font) {
    s32 bytes;
    u8 *pixels;
    s32 glyph;
    s32 output;
    s32 offset;

    bytes = (font->width * font->height * 4) / 8;
    pixels = font->pixels;
    glyph = fontImageGetGlyphIndex(code);
    offset = font->glyphs[glyph].offset;
    FONT_CACHE_LOAD(pixels + offset, bytes, &output);
    return (u8 *)output;
}

/// Diagnostic format for text reaching or exceeding the supplied font column range.
/// \note Original identifier: \c D_800A712C (0x800A712C).
extern char gFontColumnRangeErrorFormat[];

/// Measure the horizontal extent of text using a supplied font.
/// \param text Null-terminated text containing only complete supported characters and optional position/color controls.
/// \param fontData Font used to measure character advances.
/// \param spacingMode Spacing mode, with one selecting proportional spacing.
/// \returns Final horizontal position in pixels after processing the text.
/// \note Original decompilation identifier: \c func_8003ECD8 (0x8003ECD8).
s32 fontImageMeasureText(u8 *text, FontImage *fontData, s32 spacingMode) {
    s32 mode = spacingMode;
    FontImage *font = fontData;
    s32 code;
    s32 total = 0;
    s32 *decoded;
    u8 *cursor = text;
    s32 value;

    decoded = &value;
    if (*text != 0) {
        do {
            code = textDecodeNextCharacter(&cursor, decoded);
            if (code == 0xA2A0) {
                total = *decoded;
            } else if (code != 0xA3A0) {
                total += fontImageGetCharacterAdvance(code, font, mode);
            }
        } while (*cursor != 0);
    }
    if (font->width <= total) {
        TRACE(gFontColumnRangeErrorFormat, font->width, total);
    }
    return total;
}

s32 fontImageCharacterHasSideMargins(s32, FontImage *);

/// Determine how far to advance after a character in a supplied font.
/// \param code Encoded character to measure.
/// \param font Font containing character dimensions and metrics.
/// \param spacingMode Spacing mode, with one selecting proportional spacing.
/// \returns Horizontal character advance in pixels.
/// \note Original decompilation identifier: \c func_8003ED90 (0x8003ED90).
s32 fontImageGetCharacterAdvance(s32 code, FontImage *font, s32 spacingMode) {
    s32 limit = font->width;
    s32 extra;
    s32 r;

    if (fontImageCharacterHasSideMargins(code, font) == 1 || spacingMode == 1) {
        extra = 2;
    } else {
        extra = 0;
    }
    r = fontImageGetCharacterImageWidth(code, font, spacingMode) + extra;
    if (limit < r) {
        r = limit;
    }
    return r;
}


/// Check whether a character leaves horizontal margins within its font cell.
/// \param code Encoded character to inspect.
/// \param font Font containing character dimensions and metrics.
/// \returns One if a non-space character has side margins, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003EE04 (0x8003EE04).
s32 fontImageCharacterHasSideMargins(s32 code, FontImage *font) {
    if (code == 0xA1A0) {
        return 0;
    }
    if ((fontImageGetCharacterLeft(code, (FontImage *)font, 1) == 0) && ((fontImageGetCharacterRight(code, (FontImage *)font, 1) + 1) == font->width)) {
        return 0;
    }
    return 1;
}


/// Determine the image width used to draw a character in a supplied font.
/// \param code Encoded character to measure.
/// \param font Borrowed font containing character dimensions and metrics.
/// \param spacingMode Spacing mode, with one selecting proportional spacing.
/// \returns Character image width in pixels.
/// \note Original decompilation identifier: \c func_8003EE74 (0x8003EE74).
s32 fontImageGetCharacterImageWidth(s32 code, FontImage *font, s32 spacingMode) {
    s32 leftBoundary;

    leftBoundary = (fontImageGetCharacterLeft(code, font, spacingMode) / 2) * 2;
    return (((fontImageGetCharacterRight(code, font, spacingMode) / 2) * 2) - leftBoundary) + 2;
}

s32 textDecodeTwoByteCharacter(u8 **, s32 *);
s32 textDecodeAsciiCharacter(u8 **, s32 *);
s32 textDecodeAlternateCharacter(u8 **, s32 *);
s32 textDecodePositionControl(u8 **, s32 *, s32 *);
s32 textDecodeColorControl(u8 **, s32 *, s32 *);

/// Read the next character or formatting control from encoded text.
/// \param input Readable text cursor, advanced for recognized characters or control introducers; unsupported bytes can leave it unchanged.
/// \param style Receives the position or color value when a formatting control is decoded.
/// \returns Decoded character or control code, with a question mark used for unsupported input.
/// \note Original decompilation identifier: \c func_8003EEE0 (0x8003EEE0).
s32 textDecodeNextCharacter(u8 **input, s32 *style) {
    s32 code = 0;

    if (code == 0) {
        textDecodeTwoByteCharacter(input, &code);
    }
    if (code == 0) {
        textDecodeAsciiCharacter(input, &code);
    }
    if (code == 0) {
        textDecodeAlternateCharacter(input, &code);
    }
    if (code == 0) {
        textDecodePositionControl(input, &code, style);
    }
    if (code == 0) {
        textDecodeColorControl(input, &code, style);
    }
    if (code == 0) {
        code = 0xA1A9;
    }
    return code;
}

/// Decode a two-byte character from a text cursor.
/// \param input Readable text cursor with a complete character, advanced past a decoded two-byte character.
/// \param output Receives the character code on success; otherwise unchanged.
/// \returns One if a two-byte character was decoded, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003EF88 (0x8003EF88).
s32 textDecodeTwoByteCharacter(u8 **input, s32 *output) {
    s32 **result = &output;
    u8 *cursor = *input;
    s32 byte;
    s32 value;
    s32 matched;
    byte = *cursor;
    value = 0;
    matched = value;
    if ((byte & 0x80) != 0) {
        value = byte;
        byte = cursor[1];
        value = (value << 8) | byte;
        cursor += 2;
        *(*result) = value;
        matched = 1;
    }
    *input = cursor;
    return matched;
}

s32 textEncodeAsciiDigit(u8, s32 *);
s32 textEncodeAsciiLowercase(u8, s32 *);
s32 textEncodeAsciiUppercase(u8, s32 *);
s32 textEncodeAsciiPunctuation(u8, s32 *);

/// Decode an ASCII letter, digit, space, or supported punctuation character.
/// \param input Text cursor, advanced by one character on success.
/// \param code Receives the encoded character on success; otherwise unchanged.
/// \returns One if the character was recognized, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003EFCC (0x8003EFCC).
s32 textDecodeAsciiCharacter(u8 **input, s32 *code) {
    s32 r = 0;
    u8 c = **input;

    if (textEncodeAsciiDigit(c, code) == 1) {
        r = 1;
    } else if (textEncodeAsciiLowercase(c, code) == 1) {
        r = 1;
    } else if (textEncodeAsciiUppercase(c, code) == 1) {
        r = 1;
    } else if (textEncodeAsciiPunctuation(c, code) == 1) {
        r = 1;
    }
    if (r == 1) {
        (*input)++;
    }
    return r;
}

/// Convert an ASCII digit to the text character encoding.
/// \param character ASCII character to convert.
/// \param code Receives the encoded character on success; otherwise unchanged.
/// \returns One if the character was recognized, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003F088 (0x8003F088).
s32 textEncodeAsciiDigit(u8 character, s32 *code) {
    if ((character >= 0x30) && (character < 0x3A)) {
        *code = character + 0xA380;
        return 1;
    }
    return 0;
}

/// Convert an ASCII lowercase letter to the text character encoding.
/// \param character ASCII character to convert.
/// \param code Receives the encoded character on success; otherwise unchanged.
/// \returns One if the character was recognized, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003F0C0 (0x8003F0C0).
s32 textEncodeAsciiLowercase(u8 character, s32 *code) {
    if ((character >= 0x61) && (character < 0x7B)) {
        *code = character + 0xA380;
        return 1;
    }
    return 0;
}

/// Convert an ASCII uppercase letter to the text character encoding.
/// \param character ASCII character to convert.
/// \param code Receives the encoded character on success; otherwise unchanged.
/// \returns One if the character was recognized, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003F0F8 (0x8003F0F8).
s32 textEncodeAsciiUppercase(u8 character, s32 *code) {
    if ((character >= 0x41) && (character < 0x5B)) {
        *code = character + 0xA380;
        return 1;
    }
    return 0;
}

/// Convert an ASCII space or supported punctuation character to the text character encoding.
/// \param character ASCII character to convert.
/// \param code Receives the encoded character on success; otherwise unchanged.
/// \returns One if the character was recognized, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003F130 (0x8003F130).
s32 textEncodeAsciiPunctuation(u8 character, s32 *code) {
    switch (character) {
    case 0x20:
        *code = 0xA1A0;
        break;
    case 0x2C:
        *code = 0xA1A4;
        break;
    case 0x2E:
        *code = 0xA1A5;
        break;
    case 0x3A:
        *code = 0xA1A7;
        break;
    case 0x3B:
        *code = 0xA1A8;
        break;
    case 0x3F:
        *code = 0xA1A9;
        break;
    case 0x21:
        *code = 0xA1AA;
        break;
    case 0x27:
        *code = 0xA1C7;
        break;
    case 0x22:
        *code = 0xA1C9;
        break;
    case 0x28:
        *code = 0xA1CA;
        break;
    case 0x29:
        *code = 0xA1CB;
        break;
    case 0x2B:
        *code = 0xA1DC;
        break;
    case 0x2D:
        *code = 0xA1DD;
        break;
    case 0x3D:
        *code = 0xA1E1;
        break;
    case 0x3C:
        *code = 0xA1E3;
        break;
    case 0x3E:
        *code = 0xA1E4;
        break;
    case 0x24:
        *code = 0xA1F0;
        break;
    case 0x25:
        *code = 0xA1F3;
        break;
    case 0x26:
        *code = 0xA1F5;
        break;
    case 0x2A:
        *code = 0xA1F6;
        break;
    default:
        return 0;
    }
    return 1;
}

/// Decode an alternate character introduced by an at-sign escape.
/// \param input Text cursor, advanced past a valid escape or only its introducer when invalid.
/// \param code Receives the encoded character, or an at sign for an invalid escape; unchanged without an introducer.
/// \returns One for an escaped at sign or alphanumeric character, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003F260 (0x8003F260).
s32 textDecodeAlternateCharacter(u8 **input, s32 *code) {
    s32 ok;
    u8 c;
    u8 *p;

    p = *input;
    ok = 0;
    c = *p;
    if (c == '@') {
        c = p[1];
        p++;
        if (c == '@') {
            *code = 0xA1F7;
            ok = 1;
        } else if (c >= '0' && c <= '9') {
            *code = c + 0xA280;
            ok = 1;
        } else if (c >= 'A' && c <= 'Z') {
            *code = c + 0xA280;
            ok = 1;
        } else if (c >= 'a' && c <= 'z') {
            *code = c + 0xA280;
            ok = 1;
        } else {
            TRACE_LEAF("EXTRA CODE ERROR!\n");
            *code = 0xA1F7;
        }
        if (ok == 1) {
            p++;
        }
    }
    *input = p;
    return ok;
}

/// Decode a horizontal text position control or an escaped number sign.
/// \param input Text cursor, advanced past a number-sign escape and its optional decimal value.
/// \param code Receives the position control or number-sign character code when an escape is present.
/// \param position Receives the horizontal position in pixels for a position control; zero if no digits follow.
/// \returns One if a number-sign escape is present, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003F328 (0x8003F328).
s32 textDecodePositionControl(u8 **input, s32 *code, s32 *position) {
    u8 *p;
    s32 ok;
    s32 n;
    s32 i;
    u8 c;

    p = *input;
    ok = 0;
    c = *p;
    if (c == '#') {
        c = p[1];
        n = 0;
        p++;
        if (c == '#') {
            *code = 0xA1F4;
            p++;
            ok = 1;
        } else {
            for (i = 0; i < 3; i++) {
                c = *p;
                if (c < '0' || c >= ':') {
                    break;
                }
                p++;
                n = n * 10 + c - '0';
            }
            if (i == 0) {
                TRACE_LEAF("COLUMN CODE ERROR!\n");
            }
            ok = 1;
            *position = n;
            *code = 0xA2A0;
        }
    }
    *input = p;
    return ok;
}

s32 textParseHexDigit(u8, s32 *);

/// Decode a text color control introduced by a caret.
/// \param input Text cursor, advanced past a valid color control or only its introducer when invalid.
/// \param code Receives the color control code when a caret is present; otherwise unchanged.
/// \param color Receives a color value from zero to fifteen; a doubled caret or invalid value selects fifteen.
/// \returns One for a hexadecimal color or doubled caret, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003F3D0 (0x8003F3D0).
s32 textDecodeColorControl(u8 **input, s32 *code, s32 *color) {
    u8 *p;
    u8 c;
    s32 v;
    s32 ok;

    p = *input;
    ok = 0;
    c = *p;
    if (c == '^') {
        c = p[1];
        p++;
        if (c == '^') {
            v = 15;
            ok = 1;
        } else if (textParseHexDigit(c, &v) == 0) {
            TRACE("COLOR CODE ERROR!\n");
            v = 15;
        } else {
            ok = 1;
        }
        *color = v;
        *code = 0xA3A0;
        if (ok == 1) {
            p++;
        }
        *input = p;
    }
    return ok;
}

/// Convert a hexadecimal digit to its numeric value.
/// \param character ASCII hexadecimal digit to convert.
/// \param value Receives the value from zero to fifteen on success; otherwise unchanged.
/// \returns One if the character is a hexadecimal digit, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003F488 (0x8003F488).
s32 textParseHexDigit(u8 character, s32 *value) {
    if (character >= 0x30 && character < 0x3A) {
        *value = character - 0x30;
    } else if (character >= 0x41 && character < 0x47) {
        *value = character - 0x37;
    } else if (character >= 0x61 && character < 0x67) {
        *value = character - 0x57;
    } else {
        return 0;
    }
    return 1;
}

s32 fontImageGetGlyphIndex(s32 arg0, s32 arg1);
/// Get the left boundary used to draw a character in a supplied font.
/// \param code Encoded character to measure.
/// \param font Font containing character metrics.
/// \param spacingMode Spacing mode, with one selecting proportional spacing.
/// \returns Left boundary in pixels within the character cell.
/// \note Original decompilation identifier: \c func_8003F4FC (0x8003F4FC).
s32 fontImageGetCharacterLeft(s32 code, FontImage *font, s32 spacingMode) {
    if (code == 0xA1A0) {
        return 0;
    }
    if (spacingMode == 1) {
        return font->glyphs[fontImageGetGlyphIndex(code, (s32)font)].left;
    }
    return 0;
}

/// Get the right boundary used to draw a character in a supplied font.
/// \param code Encoded character to measure.
/// \param font Borrowed font containing character dimensions and metrics.
/// \param spacingMode Spacing mode, with one selecting proportional spacing.
/// \returns Inclusive right boundary in pixels within the character cell.
/// \note Original decompilation identifier: \c func_8003F554 (0x8003F554).
s32 fontImageGetCharacterRight(s32 code, FontImage *font, s32 spacingMode) {
    s32 v = font->width;

    if (code == 0xA1A0) {
        if (spacingMode == 1) {
            return 1;
        }
        return v / 2 - 1;
    }
    if (spacingMode == 1) {
        return font->glyphs[fontImageGetGlyphIndex(code, (s32)font)].right;
    }
    return v - 1;
}

/// Find the font glyph index for a supported encoded character.
/// \param code Character code from a supported font character range.
/// \param unused Unused font argument.
/// \returns Index of the character's glyph within the font.
/// \note Portability/UB: Unsupported character ranges can return an uninitialized local value.
/// \note Original decompilation identifier: \c func_8003F5D0 (0x8003F5D0).
s32 fontImageGetGlyphIndex(s32 code, s32 unused) {
    s32 maskedCode;
    s32 sp0;

    maskedCode = code & 0x7F7F;
    if ((maskedCode < 0x2580) && (maskedCode >= 0x2520)) {
        sp0 = maskedCode - 0x23B0;
    } else if (maskedCode >= 0x2420) {
        sp0 = maskedCode - 0x2310;
    } else if (maskedCode >= 0x2330) {
        sp0 = maskedCode - 0x2270;
    } else if (maskedCode >= 0x2220) {
        sp0 = maskedCode - 0x21C0;
    } else if (maskedCode >= 0x2120) {
        sp0 = maskedCode - 0x2120;
    }
    return sp0;
}

/// Release a generated text image and its image data.
/// \param image Owned generated image to release, or null; pending drawing commands must have finished using its pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F650 (0x8003F650).
void textImageDestroy(TextImage *image) {
    if (image != NULL) {
        heapFreeLocal(&gGameHeap, image->pixels);
        heapFreeLocal(&gGameHeap, image);
    }
}

/// Wait for font reads and release the current uses of cached glyphs.
/// \pre Drawing commands that reference the released glyphs have finished.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F694 (0x8003F694).
void fontCacheReleaseUses(void) {
    romCacheReleaseUses(&gFontGlyphRomCache);
}

/// Wait for outstanding font image reads to finish.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F6B8 (0x8003F6B8).
void fontCacheWaitForReads(void) {
    romCacheWaitForReads(&gFontGlyphRomCache);
}


/// Measure text using a supplied font and spacing mode.
/// \param text Null-terminated text containing only complete supported characters and optional position/color controls.
/// \param font Font used to measure character advances.
/// \param mode Spacing mode, with one selecting proportional spacing.
/// \returns Final horizontal position in pixels after processing the text.
/// \note Original decompilation identifier: \c func_8003F6DC (0x8003F6DC).
s32 textMeasureWithFont(u8 *text, FontImage *font, s32 mode) {
    return fontImageMeasureText(text, font, mode);
}
