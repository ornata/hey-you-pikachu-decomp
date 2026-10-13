#include "camera_types.h"
#include "backdrop_draw.h"
#include "heap_free.h"
#include "heap_tail.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "asset_loader.h"
#include "heap_types.h"
/// Backdrop sources, fill colors, dimensions and scrolling settings indexed by backdrop identifier.
/// \note Original identifier: \c D_8009B048 (0x8009B048).
extern u8 gBackdropDescriptors[];
/// Separate image-width view of backdrop descriptors, with a 32-byte stride.
/// \note Original identifier: \c D_8009B058 (0x8009B058).
extern u8 gBackdropWidthEntries[];
/// Separate image-height view of backdrop descriptors, with a 32-byte stride.
/// \note Original identifier: \c D_8009B05C (0x8009B05C).
extern u8 gBackdropHeightEntries[];

/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern u8 gRenderCameras[];
/// Framebuffer and command-buffer index for the current frame.
/// \note Original identifier: \c D_800F298C (0x800F298C).
extern s32 gGraphicsFramebufferIndex;
/// Identifier of the backdrop whose display settings are currently active.
/// \note Original identifier: \c D_800F2550 (0x800F2550).
extern s32 gActiveBackdropId;
/// First color framebuffer storage used for drawing and presentation.
/// \note Original identifier: \c D_803B5000 (0x803B5000).
extern u8 gColorFramebuffer0[];
/// Separate camera backdrop-image view, with a 224-byte camera stride.
/// \note Original identifier: \c D_800ABD7C (0x800ABD7C).
extern s32 gRenderCameraBackdropImages[][0xE0 / 4];
void textureCopyRgba16Region(Gfx **, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
/// Draw the active backdrop as a fixed image in the scene viewport.
/// \param list Display-list cursor, advanced by the appended drawing commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032FA0 (0x80032FA0).
void backdropDrawFixed(Gfx **list) {
    if (((MatrixCamera *)gRenderCameras)->backdropImage != NULL) {
        gSPSegment((*list)++, 9, osVirtualToPhysical(((MatrixCamera *)gRenderCameras)->backdropImage));
        gDPPipeSync((*list)++);
        gDPSetCycleType((*list)++, G_CYC_1CYCLE);
        gDPSetColorImage((*list)++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320,
                         osVirtualToPhysical(gColorFramebuffer0 + gGraphicsFramebufferIndex * 0x25800));
        gDPPipeSync((*list)++);
        textureCopyRgba16Region(list, gRenderCameraBackdropImages[0][0], 20, 20,
                                *(s32 *)(gBackdropWidthEntries + gActiveBackdropId * 32), 280, 200, 0, 0, 2, 0);
        gDPPipeSync((*list)++);
    }
}

/// Separate view of the primary camera backdrop image width.
/// \note Original identifier: \c D_800ABD80 (0x800ABD80).
extern s32 gPrimaryBackdropWidth;
/// Separate view of accumulated primary camera horizontal backdrop scroll.
/// \note Original identifier: \c D_800ABD90 (0x800ABD90).
extern f32 gPrimaryBackdropScroll;

/// Advance the active backdrop horizontal scroll within the image.
/// Requires an active backdrop with a positive image width.
/// \param delta Horizontal scroll change in pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033118 (0x80033118).
void backdropAdvanceScroll(f32 delta) {
    s32 n;
    f32 a;
    f32 *cur = &gPrimaryBackdropScroll;

    ((MatrixCamera *)gRenderCameras)->backdropScroll += delta;
    if (((MatrixCamera *)gRenderCameras)->backdropScroll < 0.0f) {
        n = gPrimaryBackdropWidth;
        a = *cur;
        do {
            a += n;
        } while (a < 0.0f);
        gPrimaryBackdropScroll = a;
    }
    n = gPrimaryBackdropWidth;
    a = *cur;
    if (n <= a) {
        do {
            a -= n;
        } while (n <= a);
        gPrimaryBackdropScroll = a;
    }
}

f32 mathUnitDirectionDegrees(f32, f32);
f32 mathWrapSignedDegrees(f32);
/// Minimum camera direction length used to derive backdrop pitch.
/// \note Original identifier: \c D_800A61C0 (0x800A61C0).
/// Scale applied to camera pitch when deriving backdrop vertical framing.
/// \note Original identifier: \c D_800A61C4 (0x800A61C4).
/// Scale converting camera yaw into horizontal backdrop pixel offset.
/// \note Original identifier: \c D_800A61C8 (0x800A61C8).
extern f32 gBackdropDirectionLengthTolerance, gBackdropPitchToScreenScale, gBackdropYawToPixelScale;
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
/// Separate view of the primary camera eye Z coordinate.
/// \note Original identifier: \c D_800ABCF8 (0x800ABCF8).
/// Separate view of the primary camera look-at X coordinate.
/// \note Original identifier: \c D_800ABCFC (0x800ABCFC).
/// Separate view of the primary camera look-at Z coordinate.
/// \note Original identifier: \c D_800ABD04 (0x800ABD04).
extern f32 gPrimaryCameraEyeZ, gPrimaryCameraTargetX, gPrimaryCameraTargetZ, gRuntimeFrameSeconds;
/// Separate horizontal-scroll-rate view of backdrop descriptors, with a 32-byte stride.
/// \note Original identifier: \c D_8009B064 (0x8009B064).
extern u8 gBackdropScrollRateEntries[];
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Update backdrop offsets from the camera direction and active scrolling settings.
/// \param view View record receiving the backdrop offsets; supplies vertical framing and accumulated horizontal scroll.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800331C4 (0x800331C4).
void backdropUpdateViewOffsets(u8 *view) {
    f32 horizontalSquared;
    f32 length;
    f32 deltaY;
    f32 pitch;
    f32 yaw;
    f32 zero = 0.0f;
    s32 offsetY;
    MatrixCamera *camera = (MatrixCamera *)gRenderCameras;
    horizontalSquared = (camera->target[0] - camera->eye[0]) * (camera->target[0] - camera->eye[0]) +
                        (camera->target[2] - camera->eye[2]) * (camera->target[2] - camera->eye[2]);
    deltaY = camera->target[1] - camera->eye[1];
    length = sqrtf(deltaY * deltaY + horizontalSquared);
    if (length < gBackdropDirectionLengthTolerance) {
        pitch = zero;
    } else {
        deltaY /= length;
        pitch = mathWrapSignedDegrees(360.0f - mathUnitDirectionDegrees(sqrtf(horizontalSquared) / length, deltaY));
        if (pitch < -90.0f)
            pitch = -90.0f;
        else if (pitch > 90.0f)
            pitch = 90.0f;
    }
    yaw = mathUnitDirectionDegrees(gPrimaryCameraTargetX - camera->eye[0], gPrimaryCameraTargetZ - gPrimaryCameraEyeZ);
    offsetY = (s32)(*(f32 *)(gCurrentAreaConfiguration + 0x68) +
                    (((s32)(240.0f * pitch * gBackdropPitchToScreenScale) + 40) - *(f32 *)(view + 4) * 1.5f));
    *(s32 *)(view + 0x98) = (s32)(yaw * gBackdropYawToPixelScale);
    *(s32 *)(view + 0x9C) = offsetY;
    if (zero != *(f32 *)(gBackdropScrollRateEntries + gActiveBackdropId * 32))
        backdropAdvanceScroll(*(f32 *)(gBackdropScrollRateEntries + gActiveBackdropId * 32) * gRuntimeFrameSeconds);
    *(s32 *)(view + 0x98) = (s32)(*(f32 *)(view + 0xA0) + *(s32 *)(view + 0x98));
}

/* The scrolling view is the first five words of D_800ABD7C's record:
 * image address, image width/height and the horizontal/vertical scroll. */
typedef struct {
    s32 image, width, height, x, y;
} ScrollView333;
void backdropUpdateViewOffsets(u8 *);
void renderFillFramebufferRectangle(u32, u32, u32, u32, u8, u8, u8, u8);

/// Draw the active scrolling backdrop and its upper and lower fill bands.
/// \param list Display-list cursor, advanced by the appended drawing commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800333D4 (0x800333D4).
void backdropDrawScrolling(Gfx **list) {
    ScrollView333 *view = (ScrollView333 *)gRenderCameraBackdropImages;
    s32 sourceY;
    s32 top;
    s32 bottom;

    if (((MatrixCamera *)gRenderCameras)->backdropImage == NULL) {
        return;
    }
    backdropUpdateViewOffsets(gRenderCameras);
    gSPSegment((*list)++, 9, osVirtualToPhysical((void *)view->image));
    gDPPipeSync((*list)++);
    gDPSetCycleType((*list)++, G_CYC_1CYCLE);
    gDPSetColorImage((*list)++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320,
                     osVirtualToPhysical(gColorFramebuffer0 + gGraphicsFramebufferIndex * 0x25800));
    while (view->x < 0) {
        view->x += view->width;
    }
    while (view->x >= view->width) {
        view->x -= view->width;
    }

    /* Image scrolled down: the rows above it show the top band color. */
    top = 0;
    sourceY = view->y;
    if (view->y < 0) {
        u8 *color;

        top = -view->y;
        color = gBackdropDescriptors + gActiveBackdropId * 32 + 8;
        sourceY = 0;
        if (top >= 220) {
            renderFillFramebufferRectangle(20, 20, 299, 219, color[0], color[1], color[2], color[3]);
            return;
        }
        if (top > 20) {
            renderFillFramebufferRectangle(20, 20, 299, top - 1, color[0], color[1], color[2], color[3]);
        }
    }

    /* Image ends above the viewport bottom: fill below with the bottom band. */
    if (view->height + top - sourceY < 219) {
        u8 *color;

        color = gBackdropDescriptors + gActiveBackdropId * 32 + 12;
        bottom = view->height + top - sourceY;
        if (bottom <= 20) {
            renderFillFramebufferRectangle(20, 20, 299, 219, color[0], color[1], color[2], color[3]);
            return;
        }
        if (bottom < 220) {
            renderFillFramebufferRectangle(20, bottom, 299, 219, color[0], color[1], color[2], color[3]);
        }
    }

    {
        s32 height;

        if (view->y >= 0) {
            height = view->height - sourceY;
        } else if (view->height + top >= 220) {
            height = 220 - top;
        } else {
            height = view->height;
        }
        /* Clip the rows that would land above the viewport top. */
        bottom = 20 - top;
        if (bottom > 0) {
            sourceY += bottom;
            height -= bottom;
            top = 20;
        }
        textureCopyRgba16Region(list, view->image, 20, top, view->width, 280, height, view->x, sourceY, 0, 1);
    }
}

void *heapAllocLocal(HpHdr *, u32, u16, s32);
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;


/// Allocate a backdrop image buffer for a view.
/// \param viewIndex Valid view index receiving the buffer.
/// \param width Requested image width in pixels.
/// \param height Requested image height in pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033784 (0x80033784).
void backdropAllocateViewBuffer(s32 viewIndex, u32 width, u32 height) {
    u32 size = width * height * 2;

    if (size < 0x1B580) {
        size = 0x1B580;
    }
    ((MatrixCamera *)gRenderCameras)[viewIndex].backdropImage = heapAllocLocal(&gGameHeap, size ^ 0, 0x280, size);
    ((MatrixCamera *)gRenderCameras)[viewIndex].useBackdrop = 1;
}

void romDmaReadBlocking(); /* extern, 3 or 4 args by caller */

/// Allocate memory and load a ROM range for backdrop data.
/// \param romStart First ROM byte to load.
/// \param romEnd Exclusive end address, at or after romStart.
/// \returns The loaded buffer address; the caller owns the allocation.
/// \note Original decompilation identifier: \c func_800337FC (0x800337FC).
s32 backdropLoadRomRange(s32 romStart, s32 romEnd) {
    s32 buf;
    s32 len;

    len = romStart;
    len = romEnd - len;
    buf = (s32)heapAllocLocal(&gGameHeap, len ^ 0, 0x29A, len);
    romDmaReadBlocking(romStart, buf, len);
    return buf;
}


/// Load a backdrop asset into an allocated view buffer.
/// \param idx Valid view index with a backdrop buffer large enough for the decoded asset.
/// \param info Asset descriptor supplying the ROM source, encoded and decoded sizes, and correction metadata.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033854 (0x80033854).
void backdropLoadAsset(s32 idx, AssetDescriptor *info) {
    s32 pad; /* unused, sizes the frame */
    u8 *buf;

    buf = heapAllocTailLocal(&gGameHeap, info->encodedEnd - info->rangeBase, 0x2C8);
    assetLoadAndPatchFromRom(info->romAddress, (u8 *)gRenderCameraBackdropImages[idx][0], buf,
                             info->encodedEnd - info->rangeBase, info->decodedEnd - info->rangeBase,
                             info->patchStart - info->rangeBase, info->patchCountValue - info->rangeBase);
    heapFreeLocal(&gGameHeap, buf);
}

/// Depth-buffer storage; its start also bounds the game heap and is used as asset-loading scratch.
/// \note Original identifier: \c D_8038F800 (0x8038F800).
extern u8 gDepthBuffer[];

/// Load a backdrop asset into a view buffer using shared loading workspace.
/// \param idx Valid view index with a backdrop buffer large enough for the decoded asset.
/// \param info Asset descriptor supplying the ROM source, encoded and decoded sizes, and correction metadata.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033904 (0x80033904).
void backdropLoadAssetWithSharedScratch(s32 idx, AssetDescriptor *info) {
    s32 pad[2]; /* unused, sizes the frame */
    u64 deadline;
    s32 i;

    deadline = osGetTime() + 0x7270E;
    for (i = 0; osGetTime() < deadline && i != 10000; i++) {
        osYieldThread();
    }
    assetLoadAndPatchFromRom(info->romAddress, (u8 *)gRenderCameraBackdropImages[idx][0], gDepthBuffer,
                             info->encodedEnd - info->rangeBase, info->decodedEnd - info->rangeBase,
                             info->patchStart - info->rangeBase, info->patchCountValue - info->rangeBase);
}

/// Load a raw backdrop ROM image into an allocated view buffer.
/// \param viewIndex Valid view index with an allocated backdrop buffer.
/// \param romStart First ROM byte to load.
/// \param romEnd Exclusive end address, at or after romStart; the entire image
/// must fit in the allocated view buffer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033A24 (0x80033A24).
void backdropLoadRawImage(s32 viewIndex, s32 romStart, s32 romEnd) {
    romDmaReadBlocking(romStart, gRenderCameraBackdropImages[viewIndex][0], romEnd - romStart, romStart);
}

void backdropLoadAsset(s32, AssetDescriptor *);
void backdropLoadAssetWithSharedScratch(s32, AssetDescriptor *);
void backdropLoadRawImage(s32, s32, s32); /* extern */

/// Raw backdrop data stored in an uncompressed ROM interval.
typedef struct BackdropRomRange {
    s32 start;
    s32 end;
} BackdropRomRange;

/// Backdrop source, fill colors, dimensions and horizontal scrolling rate.
/// A zero encoding selects a ROM range; other values select an asset descriptor.
typedef struct BackdropDescriptor {
    s32 encoding;
    u8 *source;
    u8 upperColor[4];
    u8 lowerColor[4];
    s32 width;
    s32 height;
    u8 unknown18[4];
    f32 scrollRate;
} BackdropDescriptor;

typedef char BackdropDescriptorSizeCheck[(sizeof(BackdropDescriptor) == 0x20) ? 1 : -1];

/// Load a backdrop for a view and select its display settings.
/// \param viewIndex Valid view index with an allocated backdrop buffer.
/// \param backdropId Valid backdrop descriptor index.
/// \param sharedScratch Nonzero for shared-workspace asset loading, or zero for ordinary loading.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033A70 (0x80033A70).
void backdropSelectForView(s32 viewIndex, s32 backdropId, s32 sharedScratch) {
    AssetDescriptor *assetDescriptor;
    u8 *viewState;
    u8 *romRange;
    u8 *backdropDescriptor;

    backdropDescriptor = (backdropId << 5) + gBackdropDescriptors;
    viewState = (viewIndex * 0xE0) + gRenderCameras;
    ((MatrixCamera *)viewState)->backdropWidth = (s32)((BackdropDescriptor *)backdropDescriptor)->width;
    ((MatrixCamera *)viewState)->backdropHeight = (s32)((BackdropDescriptor *)backdropDescriptor)->height;
    ((MatrixCamera *)viewState)->backdropScroll = 0.0f;
    if (((BackdropDescriptor *)backdropDescriptor)->encoding == 0) {
        romRange = ((BackdropDescriptor *)backdropDescriptor)->source;
        backdropLoadRawImage(viewIndex, ((BackdropRomRange *)romRange)->start, ((BackdropRomRange *)romRange)->end);
    } else {
        assetDescriptor = (AssetDescriptor *)((BackdropDescriptor *)backdropDescriptor)->source;
        if (sharedScratch != 0) {
            backdropLoadAssetWithSharedScratch(viewIndex, assetDescriptor);
        } else {
            backdropLoadAsset(viewIndex, assetDescriptor);
        }
    }
    gActiveBackdropId = backdropId;
}

/// Get a backdrop descriptor's upper fill-band color.
/// \param backdropId Valid backdrop descriptor index.
/// \returns A borrowed pointer to the descriptor's four-byte RGBA color.
/// \note Original decompilation identifier: \c func_80033B14 (0x80033B14).
u8 *backdropGetUpperBandColor(s32 backdropId) {
    return (backdropId << 5) + 8 + gBackdropDescriptors;
}

/// Get a backdrop descriptor's lower fill-band color.
/// \param backdropId Valid backdrop descriptor index.
/// \returns A borrowed pointer to the descriptor's four-byte RGBA color.
/// \note Original decompilation identifier: \c func_80033B2C (0x80033B2C).
u8 *backdropGetLowerBandColor(s32 backdropId) {
    return (backdropId << 5) + 0xC + gBackdropDescriptors;
}

/// Get a backdrop descriptor's image width.
/// \param backdropId Valid backdrop descriptor index.
/// \returns The backdrop width in pixels.
/// \note Original decompilation identifier: \c func_80033B44 (0x80033B44).
s32 backdropGetWidth(s32 backdropId) {
    return *(s32 *)(gBackdropWidthEntries + (backdropId << 5));
}

/// Get a backdrop descriptor's image height.
/// \param backdropId Valid backdrop descriptor index.
/// \returns The backdrop height in pixels.
/// \note Original decompilation identifier: \c func_80033B58 (0x80033B58).
s32 backdropGetHeight(s32 backdropId) {
    return *(s32 *)(gBackdropHeightEntries + (backdropId << 5));
}


/// Get the currently selected backdrop descriptor index.
/// \returns The active backdrop identifier.
/// \note Original decompilation identifier: \c func_80033B6C (0x80033B6C).
s32 backdropGetActiveId(void) {
    return gActiveBackdropId;
}

/// Separate display-mode view of backdrop descriptors; one selects fixed-image drawing.
/// \note Original identifier: \c D_8009B060 (0x8009B060).
extern u8 gBackdropDisplayModeEntries[];

void backdropDrawScrolling(Gfx **);

/// Draw the active backdrop using its selected display mode.
/// \param list Display-list cursor, advanced by the appended drawing commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033B78 (0x80033B78).
void backdropDraw(Gfx **list) {
    if (*(s32 *)(gBackdropDisplayModeEntries + (gActiveBackdropId << 5)) == 1) {
        backdropDrawFixed(list);
        return;
    }
    backdropDrawScrolling(list);
}
