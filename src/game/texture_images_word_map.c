#include "common.h"
#include "heap_types.h"
#include "word_map_types.h"

/* Tile loads used by textureUploadTile. Like the SDK gDPLoadTextureTile(_4b), but the tile line size is written as a
 * texel-count shift, ((texels << siz##_LINE_SHIFT) >> 1) bytes, instead of a multiply by siz##_LINE_BYTES. */
#define G_IM_SIZ_4b_LINE_SHIFT 0
#define G_IM_SIZ_8b_LINE_SHIFT 1
#define G_IM_SIZ_16b_LINE_SHIFT 2
#define G_IM_SIZ_32b_LINE_SHIFT 2
#define gDPTileLineShift(shift, uls, lrs) (((((((lrs) - (uls) + 1) << (shift)) >> 1) + 7) >> 3))

#define gDPLoadTextureTileShift(pkt, timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt,   \
                                shifts, shiftt)                                                                        \
    {                                                                                                                  \
        gDPSetTextureImage(pkt, fmt, siz, width, timg);                                                                \
        gDPSetTile(pkt, fmt, siz, gDPTileLineShift(siz##_LINE_SHIFT, uls, lrs), 0, G_TX_LOADTILE, 0, cmt, maskt,       \
                   shiftt, cms, masks, shifts);                                                                        \
        gDPLoadSync(pkt);                                                                                              \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,                  \
                    (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                     \
        gDPPipeSync(pkt);                                                                                              \
        gDPSetTile(pkt, fmt, siz, gDPTileLineShift(siz##_LINE_SHIFT, uls, lrs), 0, G_TX_RENDERTILE, pal, cmt, maskt,   \
                   shiftt, cms, masks, shifts);                                                                        \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,             \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                  \
    }

#define gDPLoadTextureTileShift_4b(pkt, timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt,     \
                                   shifts, shiftt)                                                                     \
    {                                                                                                                  \
        gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width) >> 1), timg);                                               \
        gDPSetTile(pkt, fmt, G_IM_SIZ_8b, gDPTileLineShift(G_IM_SIZ_4b_LINE_SHIFT, uls, lrs), 0, G_TX_LOADTILE, 0,     \
                   cmt, maskt, shiftt, cms, masks, shifts);                                                            \
        gDPLoadSync(pkt);                                                                                              \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << (G_TEXTURE_IMAGE_FRAC - 1), (ult) << G_TEXTURE_IMAGE_FRAC,            \
                    (lrs) << (G_TEXTURE_IMAGE_FRAC - 1), (lrt) << G_TEXTURE_IMAGE_FRAC);                               \
        gDPPipeSync(pkt);                                                                                              \
        gDPSetTile(pkt, fmt, G_IM_SIZ_4b, gDPTileLineShift(G_IM_SIZ_4b_LINE_SHIFT, uls, lrs), 0, G_TX_RENDERTILE, pal, \
                   cmt, maskt, shiftt, cms, masks, shifts);                                                            \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,             \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                  \
    }

/// Make an image and any required palette ready for textured drawing.
/// \param displayList Display-list cursor with sufficient capacity; receives the texture setup commands and advances past them.
/// \param image RDP-readable texture data with padded rows; borrowed until these commands finish rendering.
/// \param palette RDP-readable palette for indexed images: 16 entries for CI4 or 256 for CI8; borrowed until rendering finishes.
/// \param format N64 image format.
/// \param size N64 texel-size identifier from zero through three.
/// \param width Positive image width in texels; the padded image must fit available texture memory.
/// \param height Positive image height in texels; the full image must fit available texture memory.
/// \param wrapS Texture addressing mode along the S axis.
/// \param wrapT Texture addressing mode along the T axis.
/// \param useMaskS Nonzero to enable a width-based S addressing mask.
/// \param useMaskT Nonzero to enable a height-based T addressing mask.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80042840 (0x80042840).
void textureUploadTile(Gfx **displayList, s32 image, s32 palette, s32 format, s32 size, u32 width, u32 height, s32 wrapS,
                   s32 wrapT, s32 useMaskS, s32 useMaskT) {
    s32 maskS;
    s32 maskT;
    Gfx *g = *displayList;
    s32 count;
    u32 dimension;
    u32 alignedWidth;
    s32 remainder;

    if (useMaskS) {
        dimension = width;
        count = 0;
        for (;;) {
            dimension >>= 1;
            if (dimension == 0)
                break;
            count++;
        }
        maskS = count;
    } else {
        maskS = 0;
    }
    if (useMaskT) {
        dimension = height;
        count = 0;
        for (;;) {
            dimension >>= 1;
            if (dimension == 0)
                break;
            count++;
        }
        maskT = count;
    } else {
        maskT = 0;
    }
    gDPPipeSync(g++);
    switch (size) {
    case G_IM_SIZ_4b:
        remainder = width & 15;
        alignedWidth = remainder ? width - remainder + 16 : width;
        gDPLoadTextureTileShift_4b(g++, image, format, (s32)alignedWidth, height, 0, 0, (s32)alignedWidth - 1,
                                   height - 1, 0, wrapS, wrapT, maskS, maskT, 0, 0);
        break;
    case G_IM_SIZ_8b:
        remainder = width & 7;
        alignedWidth = remainder ? width - remainder + 8 : width;
        gDPLoadTextureTileShift(g++, image, format, G_IM_SIZ_8b, alignedWidth, height, 0, 0, (s32)alignedWidth - 1,
                                height - 1, 0, wrapS, wrapT, maskS, maskT, 0, 0);
        break;
    case G_IM_SIZ_16b:
        remainder = width & 3;
        alignedWidth = remainder ? width - remainder + 4 : width;
        gDPLoadTextureTileShift(g++, image, format, G_IM_SIZ_16b, alignedWidth, height, 0, 0, (s32)alignedWidth - 1,
                                height - 1, 0, wrapS, wrapT, maskS, maskT, 0, 0);
        break;
    case G_IM_SIZ_32b:
        remainder = width % 4;
        alignedWidth = remainder ? width - remainder + 4 : width;
        gDPLoadTextureTileShift(g++, image, G_IM_FMT_RGBA, G_IM_SIZ_32b, alignedWidth, height, 0, 0,
                                (s32)alignedWidth - 1, height - 1, 0, wrapS, wrapT, maskS, maskT, 0, 0);
        break;
    }
    if (format == G_IM_FMT_CI) {
        if (size == G_IM_SIZ_4b) {
            gDPLoadTLUT_pal16(g++, 0, palette);
        } else {
            gDPLoadTLUT_pal256(g++, palette);
        }
    }
    *displayList = g;
}

/// Initialize an image descriptor for texture rendering.
/// \param descriptor Word-aligned storage for at least seven words; any previous borrowed image references are replaced.
/// \param image Borrowed texture data with rows padded for the selected texel size; must remain valid while used for rendering.
/// \param palette Borrowed palette for indexed images; must remain valid while used for rendering.
/// \param format N64 image format.
/// \param size N64 texel-size identifier from zero through three.
/// \param width Positive image width in texels whose padded width fits in signed 32 bits.
/// \param height Positive image height in texels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80043000 (0x80043000).
void textureImageInitialize(s32 *descriptor, s32 image, s32 palette, s32 format, s32 size, s32 width, s32 height) {
    s32 r;

    descriptor[0] = image;
    descriptor[1] = palette;
    descriptor[2] = format;
    descriptor[3] = size;
    descriptor[4] = width;
    descriptor[5] = height;
    descriptor[6] = width;
    switch (size) {
    case 0:
        r = width & 0xF;
        if (r != 0) {
            descriptor[6] = width - r + 16;
        }
        break;
    case 1:
        r = width & 7;
        if (r != 0) {
            descriptor[6] = width - r + 8;
        }
        break;
    case 2:
        r = width & 3;
        if (r != 0) {
            descriptor[6] = width - r + 4;
        }
        break;
    case 3:
        r = width & 3;
        if (r != 0) {
            descriptor[6] = width - r + 4;
        }
        break;
    }
}

/* Tile-line bytes for a texel-size shift ((texels << siz) >> 1 bytes, rounded up to 8-byte lines). */
#define gDPTileLineLrs(shift, uls, lrs) (((((((u32)((lrs) - (uls))) << (shift)) + (1 << (shift))) >> 1) + 7) >> 3)
#define gDPTileLineWidth(shift, width) (((((u32)(width) << (shift)) >> 1) + 7) >> 3)
#define gDPLoadTextureTileCopyLrs_4b(pkt, timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt,   \
                                     shifts, shiftt)                                                                   \
    {                                                                                                                  \
        gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width) >> 1), timg);                                               \
        gDPSetTile(pkt, fmt, G_IM_SIZ_8b, gDPTileLineWidth(G_IM_SIZ_4b_LINE_SHIFT, width), 0, G_TX_LOADTILE, 0, cmt,   \
                   maskt, shiftt, cms, masks, shifts);                                                                 \
        gDPLoadSync(pkt);                                                                                              \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << (G_TEXTURE_IMAGE_FRAC - 1), (ult) << G_TEXTURE_IMAGE_FRAC,            \
                    (lrs) << (G_TEXTURE_IMAGE_FRAC - 1), (lrt) << G_TEXTURE_IMAGE_FRAC);                               \
        gDPPipeSync(pkt);                                                                                              \
        gDPSetTile(pkt, fmt, G_IM_SIZ_4b, gDPTileLineLrs(G_IM_SIZ_4b_LINE_SHIFT, uls, lrs), 0, G_TX_RENDERTILE, pal,   \
                   cmt, maskt, shiftt, cms, masks, shifts);                                                            \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,             \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                  \
    }

#define gDPLoadTextureTileCopyLrs(pkt, timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, \
                                  shifts, shiftt)                                                                      \
    {                                                                                                                  \
        gDPSetTextureImage(pkt, fmt, siz, width, timg);                                                                \
        gDPSetTile(pkt, fmt, siz, gDPTileLineWidth(siz##_LINE_SHIFT, width), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt,  \
                   cms, masks, shifts);                                                                                \
        gDPLoadSync(pkt);                                                                                              \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,                  \
                    (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                     \
        gDPPipeSync(pkt);                                                                                              \
        gDPSetTile(pkt, fmt, siz, gDPTileLineLrs(siz##_LINE_SHIFT, uls, lrs), 0, G_TX_RENDERTILE, pal, cmt, maskt,     \
                   shiftt, cms, masks, shifts);                                                                        \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,             \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                  \
    }

typedef struct {
    s32 image, palette;
    s32 format, size;
    u32 width, height, alignedWidth;
} Texture430;
/// Render a scaled texture with optional repetition and flipping.
/// \param out Display-list cursor with sufficient capacity; receives the drawing commands and advances past them.
/// \param texture Borrowed, initialized texture image descriptor.
/// \param x Destination screen X coordinate in pixels.
/// \param y Destination screen Y coordinate in pixels.
/// \param wrapS Texture addressing mode along the S axis.
/// \param wrapT Texture addressing mode along the T axis.
/// \param repeatX Horizontal repetition count with width-based wrapping; zero draws once without the wrapping mask.
/// \param repeatY Vertical repetition count with height-based wrapping; zero draws once without the wrapping mask.
/// \param scaleX Positive finite horizontal scale; the texture step and resulting coordinates must be representable.
/// \param scaleY Positive finite vertical scale; the texture step and resulting coordinates must be representable.
/// \param flipX Nonzero to flip the texture horizontally.
/// \param flipY Nonzero to flip the texture vertically.
/// \returns Nothing.
/// \pre Texture and palette data must remain RDP-readable until rendering finishes; source row padding must match the descriptor.
/// \pre Width and height must be positive. At least two source rows must fit texture memory unless the image has only one row.
/// \pre Four- and eight-bit images must use CI, I, or IA format; wrapped dimensions must be powers of two for whole-image repetition.
/// \note Original decompilation identifier: \c func_800430C4 (0x800430C4).
void textureDrawScaledRepeated(Gfx **out, Texture430 *texture, u32 x, u32 y, s32 wrapS, s32 wrapT, u32 repeatX, u32 repeatY,
                   f32 scaleX, f32 scaleY, s32 flipX, s32 flipY) {
    Gfx *dl;
    u32 maskS, maskT;
    s32 step;
    s32 s, t, adjustedY;
    s32 top, bottom;
    f32 texelStepX, texelStepY;
    s32 ds, dt;
    u32 dimension;
    s32 image, palette;
    s32 format, size;
    u32 width, height, alignedWidth;
    s32 index;
    u32 remaining;

    s = 0;
    t = 0;
    adjustedY = y;
    dl = *out;
    image = texture->image;
    palette = texture->palette;
    format = texture->format;
    size = texture->size;
    width = texture->width;
    height = texture->height;
    alignedWidth = texture->alignedWidth;
    texelStepX = 1024.0f / scaleX;
    texelStepY = 1024.0f / scaleY;
    ds = (s32)((f64)texelStepX + 0.5);
    dt = (s32)((f64)texelStepY + 0.5);
    gDPPipeSync(dl++);
    gDPSetCycleType(dl++, G_CYC_1CYCLE);
    gDPSetTexturePersp(dl++, G_TP_NONE);
    gDPSetTextureLOD(dl++, G_TL_TILE);
    gDPSetTextureFilter(dl++, G_TF_BILERP);
    gDPSetTextureConvert(dl++, G_TC_FILT);
    gDPSetTextureDetail(dl++, G_TD_CLAMP);
    if (repeatX) {
        dimension = width;
        index = 0;
        for (;;) {
            dimension >>= 1;
            if (dimension == 0) {
                maskS = index;
                break;
            }
            index++;
        }
    } else {
        repeatX = 1;
        maskS = 0;
    }
    if (repeatY) {
        dimension = height;
        index = 0;
        for (;;) {
            dimension >>= 1;
            if (dimension == 0) {
                maskT = index;
                break;
            }
            index++;
        }
    } else {
        repeatY = 1;
        maskT = 0;
    }
    switch (size) {
    case 0:
        switch (format) {
        case G_IM_FMT_CI:
            gDPLoadTLUT_pal16(dl++, 0, palette);
            gDPSetTextureLUT(dl++, G_TT_RGBA16);
            remaining = 4096 / alignedWidth;
            step = (remaining < height) ? remaining : height;
            break;
        case G_IM_FMT_I:
        case G_IM_FMT_IA:
            gDPSetTextureLUT(dl++, G_TT_NONE);
            remaining = 8192 / alignedWidth;
            step = (remaining < height) ? remaining : height;
            break;
        }
        step = step - 1;
        if (flipX) {
            s = width << 5;
            ds = (s32)(0U - (u32)ds);
        }
        if (flipY) {
            adjustedY = (s32)(((f64)scaleY - 1.0) * (f64)(f32)height + 0.5) + y;
            t = step << 5;
            dt = (s32)(0U - (u32)dt);
        }
        if (format == G_IM_FMT_CI) {
            gDPLoadTLUT_pal16(dl++, 0, palette);
        }
        for (index = 0; index * step < height - step - 1; index++) {

            if (!flipY) {
                top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
                bottom = (s32)((u32)((s32)((f32)y + (f32)((index + 1) * step) * scaleY * (f32)repeatY)) << 2);
            } else {
                top = (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)((index + 1) * step) * scaleY)) << 2);
                bottom =
                    (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)(index * step) * scaleY * (f32)repeatY))
                          << 2);
            }
            gDPLoadTextureTileCopyLrs_4b(dl++, image + ((alignedWidth * index * step) >> 1), format, alignedWidth,
                                         height, 0, 0, (s32)alignedWidth - 1, step, 0, wrapS, wrapT, maskS, maskT, 0,
                                         0);
            gSPTextureRectangle(dl++, x * 4, top, (s32)((u32)((s32)((f32)x + (f32)width * scaleX * (f32)repeatX)) << 2),
                                bottom, 0, s, t, ds, dt);
        }

        if (!flipY) {
            top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
            bottom = (s32)((u32)((s32)((f32)y + (f32)height * scaleY * (f32)repeatY)) << 2);
        } else {
            top = y * 4;
            bottom =
                (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)(index * step) * scaleY * (f32)repeatY)) << 2);
            t = (height - index * step) << 5;
        }

        gDPLoadTextureTileCopyLrs_4b(dl++, image + ((alignedWidth * index * step) >> 1), format, alignedWidth, height,
                                     0, 0, (s32)alignedWidth - 1, height - index * step - 1, 0, wrapS, wrapT, maskS,
                                     maskT, 0, 0);
        gSPTextureRectangle(dl++, x * 4, top, (s32)((u32)((s32)((f32)x + (f32)width * scaleX * (f32)repeatX)) << 2),
                            bottom, 0, s, t, ds, dt);
        break;
    case 1:
        switch (format) {
        case G_IM_FMT_CI:
            gDPLoadTLUT_pal256(dl++, palette);
            gDPSetTextureLUT(dl++, G_TT_RGBA16);
            remaining = 2048 / alignedWidth;
            step = (remaining < height) ? remaining : height;
            break;
        case G_IM_FMT_I:
        case G_IM_FMT_IA:
            gDPSetTextureLUT(dl++, G_TT_NONE);
            remaining = 4096 / alignedWidth;
            step = (remaining < height) ? remaining : height;
            break;
        }
        step = step - 1;
        if (flipX) {
            s = width << 5;
            ds = (s32)(0U - (u32)ds);
        }
        if (flipY) {
            adjustedY = (s32)(((f64)scaleY - 1.0) * (f64)(f32)height + 0.5) + y;
            t = step << 5;
            dt = (s32)(0U - (u32)dt);
        }
        for (index = 0; index * step < height - step - 1; index++) {

            if (!flipY) {
                top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
                bottom = (s32)((u32)((s32)((f32)y + (f32)((index + 1) * step) * scaleY * (f32)repeatY)) << 2);
            } else {
                top = (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)((index + 1) * step) * scaleY)) << 2);
                bottom =
                    (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)(index * step) * scaleY * (f32)repeatY))
                          << 2);
            }
            gDPLoadTextureTileCopyLrs(dl++, image + (alignedWidth * index * step), format, G_IM_SIZ_8b, alignedWidth,
                                      height, 0, 0, (s32)alignedWidth - 1, step, 0, wrapS, wrapT, maskS, maskT, 0, 0);
            gSPTextureRectangle(dl++, x * 4, top, (s32)((u32)((s32)((f32)x + (f32)width * scaleX * (f32)repeatX)) << 2),
                                bottom, 0, s, t, ds, dt);
        }

        if (!flipY) {
            top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
            bottom = (s32)((u32)((s32)((f32)y + (f32)height * scaleY * (f32)repeatY)) << 2);
        } else {
            top = y * 4;
            bottom =
                (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)(index * step) * scaleY * (f32)repeatY)) << 2);
            t = (height - index * step) << 5;
        }

        gDPLoadTextureTileCopyLrs(dl++, image + (alignedWidth * index * step), format, G_IM_SIZ_8b, alignedWidth,
                                  height, 0, 0, (s32)alignedWidth - 1, height - index * step - 1, 0, wrapS, wrapT,
                                  maskS, maskT, 0, 0);
        gSPTextureRectangle(dl++, x * 4, top, (s32)((u32)((s32)((f32)x + (f32)width * scaleX * (f32)repeatX)) << 2),
                            bottom, 0, s, t, ds, dt);
        break;
    case 2:
        gDPSetTextureLUT(dl++, G_TT_NONE);
        remaining = 2048 / alignedWidth;
        step = (remaining < height) ? remaining : height;
        step = step - 1;
        if (flipX) {
            s = width << 5;
            ds = (s32)(0U - (u32)ds);
        }
        if (flipY) {
            adjustedY = (s32)(((f64)scaleY - 1.0) * (f64)(f32)height + 0.5) + y;
            t = step << 5;
            dt = (s32)(0U - (u32)dt);
        }
        for (index = 0; index * step < height - step - 1; index++) {

            if (!flipY) {
                top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
                bottom = (s32)((u32)((s32)((f32)y + (f32)((index + 1) * step) * scaleY * (f32)repeatY)) << 2);
            } else {
                top = (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)((index + 1) * step) * scaleY)) << 2);
                bottom =
                    (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)(index * step) * scaleY * (f32)repeatY))
                          << 2);
            }
            gDPLoadTextureTileCopyLrs(dl++, image + (alignedWidth * index * step * 2), format, G_IM_SIZ_16b,
                                      alignedWidth, height, 0, 0, (s32)alignedWidth - 1, step, 0, wrapS, wrapT, maskS,
                                      maskT, 0, 0);
            gSPTextureRectangle(dl++, x * 4, top,
                                (s32)((u32)((s32)((f32)x + (f32)(width + 1) * scaleX * (f32)repeatX)) << 2), bottom, 0,
                                s, t, ds, dt);
        }

        if (!flipY) {
            top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
            bottom = (s32)((u32)((s32)((f32)y + (f32)height * scaleY * (f32)repeatY)) << 2);
        } else {
            top = y * 4;
            bottom =
                (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)(index * step) * scaleY * (f32)repeatY)) << 2);
            t = (height - index * step) << 5;
        }

        gDPLoadTextureTileCopyLrs(dl++, image + (alignedWidth * index * step * 2), format, G_IM_SIZ_16b, alignedWidth,
                                  height, 0, 0, (s32)alignedWidth - 1, height - index * step - 1, 0, wrapS, wrapT,
                                  maskS, maskT, 0, 0);
        gSPTextureRectangle(dl++, x * 4, top, (s32)((u32)((s32)((f32)x + (f32)width * scaleX * (f32)repeatX)) << 2),
                            bottom, 0, s, t, ds, dt);
        break;
    case 3:
        gDPSetTextureLUT(dl++, G_TT_NONE);
        remaining = 1024 / alignedWidth;
        step = (remaining < height) ? remaining : height;
        step = step - 1;
        if (flipX) {
            s = width << 5;
            ds = (s32)(0U - (u32)ds);
        }
        if (flipY) {
            adjustedY = (s32)(((f64)scaleY - 1.0) * (f64)(f32)height + 0.5) + y;
            t = step << 5;
            dt = (s32)(0U - (u32)dt);
        }
        for (index = 0; index * step < height - step - 1; index++) {

            if (!flipY) {
                top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
                bottom = (s32)((u32)((s32)((f32)y + (f32)((index + 1) * step) * scaleY * (f32)repeatY)) << 2);
            } else {
                top = (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)((index + 1) * step) * scaleY)) << 2);
                bottom =
                    (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)(index * step) * scaleY * (f32)repeatY))
                          << 2);
            }
            gDPLoadTextureTileCopyLrs(dl++, image + (alignedWidth * index * step * 4), G_IM_FMT_RGBA, G_IM_SIZ_32b,
                                      alignedWidth, height, 0, 0, (s32)alignedWidth - 1, step, 0, wrapS, wrapT, maskS,
                                      maskT, 0, 0);
            gSPTextureRectangle(dl++, x * 4, top, (s32)((u32)((s32)((f32)x + (f32)width * scaleX * (f32)repeatX)) << 2),
                                bottom, 0, s, t, ds, dt);
        }

        if (!flipY) {
            top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
            bottom = (s32)((u32)((s32)((f32)y + (f32)height * scaleY * (f32)repeatY)) << 2);
        } else {
            top = y * 4;
            bottom =
                (s32)((u32)((s32)((f32)(u32)(adjustedY + height) - (f32)(index * step) * scaleY * (f32)repeatY)) << 2);
            t = (height - index * step) << 5;
        }

        gDPLoadTextureTileCopyLrs(dl++, image + (alignedWidth * index * step * 4), G_IM_FMT_RGBA, G_IM_SIZ_32b,
                                  alignedWidth, height, 0, 0, (s32)alignedWidth - 1, height - index * step - 1, 0,
                                  wrapS, wrapT, maskS, maskT, 0, 0);
        gSPTextureRectangle(dl++, x * 4, top, (s32)((u32)((s32)((f32)x + (f32)width * scaleX * (f32)repeatX)) << 2),
                            bottom, 0, s, t, ds, dt);
        break;
    }
    *out = dl;
}

/// Borrowed image and palette references for a pair of equally sized textures.
typedef struct TexturePair {
    s32 image0, palette0, format0, size0, image1, palette1, format1, size1;
    u32 width, height;
} TexturePair;

/// Initialize a pair of texture images for combined rendering.
/// \param descriptor Word-aligned storage for at least ten words; replaces any previous borrowed image references.
/// \param image0 Borrowed RDP-readable image data with rows padded for its texel size; retained while used for rendering.
/// \param palette0 Borrowed RDP-readable palette for an indexed image; retained while used for rendering.
/// \param format0 N64 image format.
/// \param size0 N64 texel-size identifier from zero through three.
/// \param image1 Borrowed RDP-readable image data with rows padded for its texel size; retained while used for rendering.
/// \param palette1 Borrowed RDP-readable palette for an indexed image; retained while used for rendering.
/// \param format1 N64 image format.
/// \param size1 N64 texel-size identifier from zero through three.
/// \param width Positive shared image width in texels.
/// \param height Positive shared image height in texels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8004592C (0x8004592C).
void texturePairInitialize(u8 *descriptor, s32 image0, s32 palette0, s32 format0, s32 size0, s32 image1, s32 palette1, s32 format1, s32 size1, s32 width,
                   s32 height) {
    ((TexturePair *)descriptor)->image0 = image0;
    ((TexturePair *)descriptor)->palette0 = palette0;
    ((TexturePair *)descriptor)->format0 = format0;
    ((TexturePair *)descriptor)->size0 = size0;
    ((TexturePair *)descriptor)->image1 = image1;
    ((TexturePair *)descriptor)->palette1 = palette1;
    ((TexturePair *)descriptor)->format1 = format1;
    ((TexturePair *)descriptor)->size1 = size1;
    ((TexturePair *)descriptor)->width = width;
    ((TexturePair *)descriptor)->height = height;
}

/// Determine the texture-memory layout and row capacity for paired images.
/// \param stripRows Positive available row capacity on entry; receives the capacity for paired rendering. Intermediate scaled capacities must fit in signed 32 bits.
/// \param secondTmem Separate output receiving the second image texture-memory base in N64 TMEM units.
/// \param format0 First image N64 format.
/// \param size0 First image N64 texel-size identifier from zero through three.
/// \param format1 Second image N64 format.
/// \param size1 Second image N64 texel-size identifier from zero through three.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80045974 (0x80045974).
void texturePairGetMemoryLayout(s32 *stripRows, s32 *secondTmem, u32 format0, u32 size0, u32 format1, u32 size1) {
    s32 var_v0;
    s32 var_v1;

    if (format0 == 2) {
        *stripRows -= 1;
        *secondTmem = 0x100;
        return;
    }
    if (format1 == 2) {
        *stripRows -= 1;
        *secondTmem = 0;
        return;
    }
    switch (size0) {
    case 0:
        var_v0 = 8;
        break;
    case 1:
        var_v0 = 4;
        break;
    case 2:
        var_v0 = 2;
        break;
    case 3:
        var_v0 = 1;
        break;
    }
    switch (size1) {
    case 0:
        var_v1 = 8;
        break;
    case 1:
        var_v1 = 4;
        break;
    case 2:
        var_v1 = 2;
        break;
    case 3:
        var_v1 = 1;
        break;
    }
    if ((var_v0 != 1) && (var_v1 != 1)) {
        if (var_v0 == var_v1) {
            *stripRows = (*stripRows / 2) - 1;
            *secondTmem = 0x100;
            return;
        }
        if (var_v1 < var_v0) {
            *stripRows = (s32)((((f64)((*stripRows * var_v0) / (var_v0 + var_v1)) - 1.5) < 1.0)
                              ? 1.0
                              : ((f64)((*stripRows * var_v0) / (var_v0 + var_v1)) - 1.5));
            if ((((var_v1 << 9) / (var_v0 + var_v1)) % 16) != 0) {
                *secondTmem = (((var_v1 << 9) / (var_v0 + var_v1)) - (((var_v1 << 9) / (var_v0 + var_v1)) % 16)) + 0x10;
                return;
            }
            *secondTmem = (var_v1 << 9) / (var_v0 + var_v1);
            return;
        }
        *stripRows = (s32)((((f64)((*stripRows * var_v1) / (var_v0 + var_v1)) - 1.5) < 1.0)
                          ? 1.0
                          : ((f64)((*stripRows * var_v1) / (var_v0 + var_v1)) - 1.5));
        *secondTmem = ((var_v1 << 9) / (var_v0 + var_v1)) - (((var_v1 << 9) / (var_v0 + var_v1)) % 16);
        return;
    }
    if (var_v0 == var_v1) {
        *stripRows = (*stripRows / 2) - 1;
        *secondTmem = 0x80;
        return;
    }
    if (var_v1 < var_v0) {
        *stripRows = (((*stripRows * var_v0) / ((var_v0 + var_v1) * 2)) < 1) ? 1 : ((*stripRows * var_v0) / ((var_v0 + var_v1) * 2));
        if ((((var_v1 << 9) / (var_v0 + var_v1)) % 16) != 0) {
            *secondTmem = (((var_v1 << 9) / (var_v0 + var_v1)) - (((var_v1 << 9) / (var_v0 + var_v1)) % 16)) + 0x10;
            return;
        }
        *secondTmem = (var_v1 << 9) / (var_v0 + var_v1);
        return;
    }
    *stripRows = (((*stripRows * var_v1) / ((var_v0 + var_v1) * 2)) < 1) ? 1 : ((*stripRows * var_v1) / ((var_v0 + var_v1) * 2));
    *secondTmem = ((var_v1 << 8) / (var_v0 + var_v1)) - (((var_v1 << 8) / (var_v0 + var_v1)) % 16);
}

/// Upload an image strip for paired-texture rendering.
/// \param displayList Display-list cursor with sufficient capacity; receives upload commands and advances past them.
/// \param stripIndex Nonnegative strip index; all requested source rows must be readable.
/// \param image Borrowed RDP-readable image data with rows padded for its texel size; retained until rendering finishes.
/// \param palette Borrowed 256-entry RDP-readable palette for an indexed image; retained until rendering finishes.
/// \param tile Destination rendering tile, zero for the first image or one to finish the pair.
/// \param tmem Destination texture-memory base in N64 TMEM units.
/// \param format N64 image format.
/// \param size N64 texel-size identifier from zero through three.
/// \param width Positive image width in texels; the upload must fit its assigned texture-memory area.
/// \param stripRows Positive source row stride between strips.
/// \param lastRow Nonnegative inclusive final row within the uploaded strip.
/// \param wrapS Texture addressing mode along S.
/// \param wrapT Texture addressing mode along T.
/// \param maskS Texture addressing mask along S.
/// \param maskT Texture addressing mask along T.
/// \returns Nothing.
/// \pre Upload tile zero followed by tile one without interleaving another pair; indexed images in a pair must share their palette.
/// \note Original decompilation identifier: \c func_80045DFC (0x80045DFC).
void texturePairUploadStrip(Gfx **displayList, s32 stripIndex, s32 image, s32 palette, s32 tile, s32 tmem, s32 format, s32 size,
                   s32 width, s32 stripRows, s32 lastRow, s32 wrapS, s32 wrapT, s32 maskS, s32 maskT) {
    static s32 pendingPalette = 0;
    Gfx *g = *displayList;
    s32 aligned;
    if (format == G_IM_FMT_CI)
        pendingPalette = palette;
    switch (size) {
    case G_IM_SIZ_4b:
        if (width & 15)
            aligned = width - (width & 15) + 16;
        else
            aligned = width;
        gDPLoadMultiTile_4b(g++, (u32)image + (((u32)aligned * (u32)stripIndex * (u32)stripRows) >> 1), tmem, tile,
                            format, aligned, stripRows, 0, 0, aligned - 1, lastRow, 0, wrapS, wrapT, maskS, maskT, 0,
                            0);
        break;
    case G_IM_SIZ_8b:
        if (width & 7)
            aligned = width - (width & 7) + 8;
        else
            aligned = width;
        gDPLoadMultiTile(g++, (u32)image + ((u32)aligned * (u32)stripIndex * (u32)stripRows), tmem, tile, format,
                         G_IM_SIZ_8b, aligned, stripRows, 0, 0, aligned - 1, lastRow, 0, wrapS, wrapT, maskS, maskT, 0,
                         0);
        break;
    case G_IM_SIZ_16b:
        if (width & 3)
            aligned = width - (width & 3) + 4;
        else
            aligned = width;
        gDPLoadMultiTile(g++, (u32)image + ((u32)aligned * (u32)stripIndex * (u32)stripRows * 2), tmem, tile, format,
                         G_IM_SIZ_16b, aligned, stripRows, 0, 0, aligned - 1, lastRow, 0, wrapS, wrapT, maskS, maskT, 0,
                         0);
        break;
    case G_IM_SIZ_32b:
        if (width & 3)
            aligned = width - (width & 3) + 4;
        else
            aligned = width;
        gDPLoadMultiTile(g++, (u32)image + ((u32)aligned * (u32)stripIndex * (u32)stripRows * 4), tmem, tile, format,
                         G_IM_SIZ_32b, aligned, stripRows, 0, 0, aligned - 1, lastRow, 0, wrapS, wrapT, maskS, maskT, 0,
                         0);
        break;
    }
    if (tile == 1 && pendingPalette != 0) {
        gDPLoadTLUT_pal256(g++, pendingPalette);
        gDPSetTextureLUT(g++, G_TT_RGBA16);
        pendingPalette = 0;
    } else {
        gDPSetTextureLUT(g++, G_TT_NONE);
    }
    *displayList = g;
}

void texturePairGetMemoryLayout(s32 *, s32 *, u32, u32, u32, u32);
void texturePairUploadStrip(Gfx **, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
/// Render two combined textures with scaling, repetition, and optional flipping.
/// \param displayListOut Display-list cursor with sufficient capacity; receives drawing commands and advances past them.
/// \param texture Borrowed, initialized paired-image descriptor.
/// \param x Destination screen X coordinate in pixels.
/// \param y Destination screen Y coordinate in pixels.
/// \param wrapS Texture addressing mode along S.
/// \param wrapT Texture addressing mode along T.
/// \param repeatS Horizontal repetition count with width-based wrapping; zero draws once without the wrapping mask.
/// \param repeatT Vertical repetition count with height-based wrapping; zero draws once without the wrapping mask.
/// \param scaleX Positive finite horizontal scale; the texture step and resulting coordinates must be representable.
/// \param scaleY Positive finite vertical scale; the texture step and resulting coordinates must be representable.
/// \param flipX Nonzero to flip horizontally.
/// \param flipY Nonzero to flip vertically.
/// \returns Nothing.
/// \pre Texture and palette data must remain RDP-readable until rendering finishes; source row padding must match the descriptor.
/// \pre Positive dimensions must yield a positive paired strip capacity smaller than the image height; each upload must fit its assigned texture memory.
/// \pre Four- and eight-bit images must use CI, I, or IA format. Indexed images must share a 256-entry palette; whole-image wrapping requires power-of-two dimensions.
/// \note Portability: The local declaration order, repeated row expressions, and empty debug check preserve the original IDO register allocation.
/// \note Original decompilation identifier: \c func_80046634 (0x80046634).
void texturePairDrawScaledRepeated(Gfx **displayListOut, TexturePair *texture, u32 x, u32 y, s32 wrapS, s32 wrapT,
                                   u32 repeatS, u32 repeatT, f32 scaleX, f32 scaleY, s32 flipX, s32 flipY) {
    u32 width;
    s32 maskS;
    s32 maskT;
    u32 dimension;
    s32 stripRows;
    s32 sourceS;
    s32 sourceT;
    s32 flippedY;
    s32 rectangleLeft, upperY, rectangleRight, lowerY;
    s32 dsdx;
    s32 dtdy;
    Gfx *displayList;
    s32 image0;
    s32 palette0;
    s32 selectedFormat;
    s32 selectedSize;
    s32 format0;
    s32 size0;
    s32 image1;
    s32 palette1;
    s32 format1;
    s32 size1;
    s32 remainder;
    u32 height;
    s32 tmem1;
    s32 alignedWidth;
    s32 stripIndex;

    sourceS = 0;
    sourceT = 0;
    flippedY = y;
    displayList = *displayListOut;
    image0 = texture->image0;
    palette0 = texture->palette0;
    selectedFormat = texture->format0;
    selectedSize = texture->size0;
    size0 = texture->size0;
    format0 = texture->format0;
    image1 = texture->image1;
    palette1 = texture->palette1;
    format1 = texture->format1;
    size1 = texture->size1;
    width = texture->width;
    height = texture->height;
    dsdx = (s32)((f64)(1024.0f / scaleX) + 0.5);
    dtdy = (s32)((f64)(1024.0f / scaleY) + 0.5);
    gDPPipeSync(displayList++);
    gDPSetCycleType(displayList++, G_CYC_2CYCLE);
    gDPSetTextureLOD(displayList++, G_TL_TILE);
    gDPSetTexturePersp(displayList++, G_TP_NONE);
    gDPSetTextureFilter(displayList++, G_TF_BILERP);
    gDPSetTextureConvert(displayList++, G_TC_FILT);
    gDPSetTextureDetail(displayList++, G_TD_CLAMP);
    if (repeatS != 0) {
        dimension = width;
        stripIndex = 0;
        for (;;) {
            dimension >>= 1;
            if (dimension == 0) {
                maskS = stripIndex;
                break;
            }
            stripIndex++;
        }
    } else {
        repeatS = 1;
        maskS = 0;
    }
    stripIndex = 0;
    if (repeatT != 0) {
        dimension = height;
        for (;;) {
            dimension >>= 1;
            if (dimension == 0) {
                maskT = stripIndex;
                break;
            }
            stripIndex++;
        }
    } else {
        repeatT = 1;
        maskT = 0;
    }
    switch (size0) {
    case 0:
        switch (size1) {
        case 0:
            if (format1 != G_IM_FMT_CI)
                break;
        case 1:
        case 2:
        case 3:
            selectedSize = size1;
            selectedFormat = format1;
            break;
        }
        break;
    case 1:
        switch (size1) {
        case 1:
            if (format1 != G_IM_FMT_CI)
                break;
        case 2:
        case 3:
            selectedSize = size1;
            selectedFormat = format1;
            break;
        }
        break;
    case 2:
        switch (size1) {
        case 3:
            selectedSize = size1;
            selectedFormat = format1;
            break;
        }
        break;
    }
    switch (selectedSize) {
    case 0:
        remainder = width & 15;
        alignedWidth = remainder != 0 ? (width - remainder) + 16 : width;
        switch (selectedFormat) {
        case G_IM_FMT_CI:
            dimension = 4096 / alignedWidth;
            stripRows = (dimension < height) ? dimension : height;
            break;
        case G_IM_FMT_I:
        case G_IM_FMT_IA:
            dimension = 8192 / alignedWidth;
            stripRows = (dimension < height) ? dimension : height;
            break;
        }
        break;
    case 1:
        remainder = width & 7;
        alignedWidth = remainder != 0 ? (width - remainder) + 8 : width;
        switch (selectedFormat) {
        case G_IM_FMT_CI:
            dimension = 2048 / alignedWidth;
            stripRows = (dimension < height) ? dimension : height;
            break;
        case G_IM_FMT_I:
        case G_IM_FMT_IA:
            dimension = 4096 / alignedWidth;
            stripRows = (dimension < height) ? dimension : height;
            break;
        }
        break;
    case 2:
        remainder = width & 3;
        alignedWidth = remainder != 0 ? (width - remainder) + 4 : width;
        dimension = 2048 / alignedWidth;
        gDPSetTextureLUT(displayList++, G_TT_NONE);
        stripRows = (dimension < height) ? dimension : height;
        break;
    case 3:
        remainder = width & 3;
        alignedWidth = remainder != 0 ? (width - remainder) + 4 : width;
        dimension = 1024 / alignedWidth;
        stripRows = (dimension < height) ? dimension : height;
        break;
    }
    texturePairGetMemoryLayout(&stripRows, &tmem1, format0, size0, format1, size1);
    if (flipX != 0) {
        sourceS = width << 5;
        dsdx = (s32)(0U - (u32)dsdx);
    }
    if (flipY != 0) {
        flippedY = (s32)((((f64)scaleY - 1.0) * (f64)(f32)height) + 0.5) + y;
        sourceT = (s32)((u32)stripRows << 5);
        dtdy = (s32)(0U - (u32)dtdy);
    }
    for (stripIndex = 0; stripIndex * stripRows < height - stripRows - 1; stripIndex++) {
        if (displayList == NULL) {
        }
        if (flipY == 0) {
            upperY = (s32)((u32)((s32)(((f32)y) + ((f32)(stripIndex * stripRows) * scaleY))) << 2);
            lowerY =
                (s32)((u32)((s32)(((f32)y) + ((f32)((stripIndex + 1) * stripRows) * scaleY * ((f32)repeatT)))) << 2);
        } else {
            upperY =
                (s32)((u32)((s32)((f32)(flippedY + height) - ((f32)((stripIndex + 1) * stripRows) * scaleY))) << 2);
            lowerY =
                (s32)((u32)((s32)((f32)(flippedY + height) - ((f32)(stripIndex * stripRows) * scaleY * ((f32)repeatT))))
                      << 2);
        }
        texturePairUploadStrip(&displayList, stripIndex, image0, palette0, 0, 0, format0, size0, width, stripRows, stripRows,
                      wrapS, wrapT, maskS, maskT);
        texturePairUploadStrip(&displayList, stripIndex, image1, palette1, 1, tmem1, format1, size1, width, stripRows, stripRows,
                      wrapS, wrapT, maskS, maskT);
        rectangleLeft = x * 4;
        rectangleRight = (s32)((u32)((s32)(((f32)x) + ((f32)width * scaleX * ((f32)repeatS)))) << 2);
        gSPTextureRectangle(displayList++, rectangleLeft, upperY, rectangleRight, lowerY, 0, sourceS, sourceT, dsdx,
                            dtdy);
    }
    if (flipY == 0) {
        upperY = (s32)((u32)((s32)(((f32)y) + ((f32)(stripIndex * stripRows) * scaleY))) << 2);
        lowerY = (s32)((u32)((s32)(((f32)y) + (((f32)height) * scaleY * ((f32)repeatT)))) << 2);
    } else {
        upperY = y * 4;
        lowerY =
            (s32)((u32)((s32)((f32)(flippedY + height) - ((f32)(stripIndex * stripRows) * scaleY * ((f32)repeatT))))
                  << 2);
        sourceT = (height - stripIndex * stripRows) << 5;
    }
    texturePairUploadStrip(&displayList, stripIndex, image0, palette0, 0, 0, format0, size0, width, stripRows,
                  height - stripIndex * stripRows - 1, wrapS, wrapT, maskS, maskT);
    texturePairUploadStrip(&displayList, stripIndex, image1, palette1, 1, tmem1, format1, size1, width, stripRows,
                  height - stripIndex * stripRows - 1, wrapS, wrapT, maskS, maskT);
    rectangleLeft = x * 4;
    rectangleRight = (s32)((u32)((s32)(((f32)x) + ((f32)width * scaleX * ((f32)repeatS)))) << 2);
    gSPTextureRectangle(displayList++, rectangleLeft, upperY, rectangleRight, lowerY, 0, sourceS, sourceT, dsdx, dtdy);
    *displayListOut = displayList;
}

/* RGBA16 tile load whose line size is the 16-bit texel-count shift ((texels << 2) >> 1 bytes), used by the copy-mode
 * column drawer below. */
#define gDPLoadTextureTileL(pkt, timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt,       \
                            shifts, shiftt)                                                                            \
    {                                                                                                                  \
        gDPSetTextureImage(pkt, fmt, siz, width, timg);                                                                \
        gDPSetTile(pkt, fmt, siz, ((((((lrs) - (uls) + 1) << 2) >> 1) + 7) >> 3), 0, G_TX_LOADTILE, 0, cmt, maskt,     \
                   shiftt, cms, masks, shifts);                                                                        \
        gDPLoadSync(pkt);                                                                                              \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,                  \
                    (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                     \
        gDPPipeSync(pkt);                                                                                              \
        gDPSetTile(pkt, fmt, siz, ((((((lrs) - (uls) + 1) << 2) >> 1) + 7) >> 3), 0, G_TX_RENDERTILE, pal, cmt, maskt, \
                   shiftt, cms, masks, shifts);                                                                        \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,             \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                  \
    }

/// Copy a region of an RGBA16 image to the screen.
/// \param displayList Display-list cursor with sufficient capacity; receives drawing commands and advances past them.
/// \param image Borrowed RDP-readable RGBA16 image with padded rows and a readable requested region; retained until rendering finishes.
/// \param x Destination screen X coordinate in pixels.
/// \param y Destination screen Y coordinate in pixels.
/// \param imageWidth Positive source width in texels; at least two padded rows must fit texture memory unless height is one.
/// \param drawWidth Positive destination rectangle width in pixels.
/// \param height Positive number of readable source rows to draw.
/// \param sourceX Source X offset in texels.
/// \param sourceY Source Y offset in texels.
/// \param wrapS Texture addressing mode along S.
/// \param maskWidth Nonzero to enable width-based horizontal addressing.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800473B4 (0x800473B4).
void textureCopyRgba16Region(Gfx **displayList, s32 image, s32 x, s32 y, u32 imageWidth, s32 drawWidth, u32 height, s32 sourceX,
                   s32 sourceY, s32 wrapS, s32 maskWidth) {
    Gfx *g = *displayList;
    s32 maskS;
    u32 dimension;
    u32 alignedWidth;
    s32 remainder;
    s32 capacity;
    s32 count;
    u32 quotient;
    s32 nextRow;
    s32 uly;
    s32 lry;

    gDPPipeSync(g++);
    gDPSetCycleType(g++, G_CYC_COPY);
    gDPSetRenderMode(g++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetTexturePersp(g++, G_TP_NONE);
    gDPSetTextureLUT(g++, G_TT_NONE);
    gDPSetCombineMode(g++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    if (maskWidth != 0) {
        count = 0;
        dimension = imageWidth;
        for (;;) {
            dimension >>= 1;
            if (dimension == 0)
                break;
            count++;
        }
        maskS = count;
    } else {
        maskS = 0;
    }
    remainder = (s32)imageWidth & 3;
    if (remainder != 0)
        alignedWidth = imageWidth - remainder + 4;
    else
        alignedWidth = imageWidth;
    quotient = 0x800 / (s32)alignedWidth;
    capacity = (quotient < height) ? quotient : height;
    capacity -= 1;
    for (count = 0; count * capacity < height - capacity - 1; count++) {
        uly = ((u32)(count * capacity) + (u32)y) << 2;
        nextRow = (count + 1) * capacity;
        lry = ((u32)nextRow + (u32)y - 1U) << 2;
        gDPLoadTextureTileL(g++, image + alignedWidth * ((u32)(count * capacity) + sourceY) * 2, G_IM_FMT_RGBA,
                            G_IM_SIZ_16b, alignedWidth, height, 0, 0, (s32)alignedWidth - 1, capacity, 0, wrapS,
                            G_TX_CLAMP, maskS, 0, 0, 0);
        gSPTextureRectangle(g++, (u32)x << 2, uly, ((u32)x + (u32)drawWidth - 1U) << 2, lry, G_TX_RENDERTILE,
                            (u32)sourceX << 5, 0, 0x1000, 0x400);
    }
    uly = ((u32)(count * capacity) + (u32)y) << 2;
    gDPLoadTextureTileL(g++, image + alignedWidth * ((u32)(count * capacity) + sourceY) * 2, G_IM_FMT_RGBA,
                        G_IM_SIZ_16b, alignedWidth, height, 0, 0, (s32)alignedWidth - 1, height - count * capacity - 1,
                        0, wrapS, G_TX_WRAP, maskS, 0, 0, 0);
    gSPTextureRectangle(g++, (u32)x << 2, uly, ((u32)x + (u32)drawWidth - 1U) << 2, (y + height - 1) << 2,
                        G_TX_RENDERTILE, (u32)sourceX << 5, 0, 0x1000, 0x400);
    gSPTexture(g++, 0x4000, 0, 0, 0, G_OFF);
    gMoveWd(g++, G_MW_FOG, 0x10, 0x4000);
    *displayList = g;
}

/* Copy-mode tile load. Line sizes use the texel-size shift ((texels << siz) >> 1 bytes); tile coordinates are
 * signed 10.2 fixed point. */
#define gDPLoadTextureTileCopy(pkt, timg, fmt, siz, width, height, uls, ult, lrs, lrt, cms, cmt, masks, maskt)         \
    {                                                                                                                  \
        gDPSetTextureImage(pkt, fmt, siz, width, timg);                                                                \
        gDPSetTile(pkt, fmt, siz, (((((u32)(width) << (siz)) >> 1) + 7) >> 3), 0, G_TX_LOADTILE, 0, cmt, maskt, 0,     \
                   cms, masks, 0);                                                                                     \
        gDPLoadSync(pkt);                                                                                              \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,                  \
                    (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                     \
        gDPPipeSync(pkt);                                                                                              \
        gDPSetTile(pkt, fmt, siz, (((((((u32)((lrs) - (uls))) << (siz)) + (1 << (siz))) >> 1) + 7) >> 3), 0,           \
                   G_TX_RENDERTILE, 0, cmt, maskt, 0, cms, masks, 0);                                                  \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,             \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                  \
    }

/// Copy a wrapped RGBA16 texture into a screen rectangle.
/// \param displayList Display-list cursor with sufficient capacity; receives drawing commands and advances past them.
/// \param image Borrowed RDP-readable RGBA16 image fitting in texture memory; retained until rendering finishes.
/// \param x Destination screen X coordinate in pixels.
/// \param y Destination screen Y coordinate in pixels.
/// \param width Positive power-of-two source image width in texels.
/// \param height Positive power-of-two source image height in texels.
/// \param drawWidth Destination rectangle width in pixels.
/// \param drawHeight Destination rectangle height in pixels.
/// \param sourceS Starting horizontal texture coordinate in texels.
/// \param sourceT Starting vertical texture coordinate in texels.
/// \returns Nothing.
/// \pre Destination extents must be positive; screen and starting texture coordinates must be nonnegative and representable in the RDP coordinate fields.
/// \note Original decompilation identifier: \c func_80047934 (0x80047934).
void textureCopyWrappedRgba16(Gfx **displayList, void *image, s32 x, s32 y, u32 width, u32 height, s32 drawWidth, s32 drawHeight,
                   s32 sourceS, s32 sourceT) {
    Gfx *g = *displayList;
    s32 masks;
    s32 maskt;
    s32 count;
    u32 dimension;
    s32 xl;
    s32 yl;
    s32 xh;
    s32 yh;

    gDPPipeSync(g++);
    gDPSetCycleType(g++, G_CYC_COPY);
    gDPSetRenderMode(g++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetTexturePersp(g++, G_TP_NONE);
    gDPSetTextureLUT(g++, G_TT_NONE);
    gDPSetCombineMode(g++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    for (count = 0, dimension = width;; count++) {
        dimension >>= 1;
        if (dimension == 0)
            break;
    }
    masks = count;
    for (count = 0, dimension = height;; count++) {
        dimension >>= 1;
        if (dimension == 0)
            break;
    }
    maskt = count;
    gDPLoadTextureTileCopy(g++, image, G_IM_FMT_RGBA, G_IM_SIZ_16b, width, height, 0, 0, (s32)width - 1,
                           (s32)height - 1, G_TX_WRAP, G_TX_WRAP, masks, maskt);
    xl = x << 2;
    yl = y << 2;
    xh = (x + drawWidth - 1) << 2;
    yh = (y + drawHeight) << 2;
    gSPTextureRectangle(g++, xl, yl, xh, yh, G_TX_RENDERTILE, sourceS << 5, sourceT << 5, 0x1000, 0x400);
    *displayList = g;
}

#define gDPLoadTextureTileLrs(pkt, timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt,     \
                              shifts, shiftt)                                                                          \
    {                                                                                                                  \
        gDPSetTextureImage(pkt, fmt, siz, width, timg);                                                                \
        gDPSetTile(pkt, fmt, siz, gDPTileLineLrs(siz##_LINE_SHIFT, uls, lrs), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, \
                   cms, masks, shifts);                                                                                \
        gDPLoadSync(pkt);                                                                                              \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,                  \
                    (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                     \
        gDPPipeSync(pkt);                                                                                              \
        gDPSetTile(pkt, fmt, siz, gDPTileLineLrs(siz##_LINE_SHIFT, uls, lrs), 0, G_TX_RENDERTILE, pal, cmt, maskt,     \
                   shiftt, cms, masks, shifts);                                                                        \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,             \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                  \
    }
#define gDPLoadTextureTileLrs_4b(pkt, timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt,       \
                                 shifts, shiftt)                                                                       \
    {                                                                                                                  \
        gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width) >> 1), timg);                                               \
        gDPSetTile(pkt, fmt, G_IM_SIZ_8b, gDPTileLineLrs(G_IM_SIZ_4b_LINE_SHIFT, uls, lrs), 0, G_TX_LOADTILE, 0, cmt,  \
                   maskt, shiftt, cms, masks, shifts);                                                                 \
        gDPLoadSync(pkt);                                                                                              \
        gDPLoadTile(pkt, G_TX_LOADTILE, (uls) << (G_TEXTURE_IMAGE_FRAC - 1), (ult) << G_TEXTURE_IMAGE_FRAC,            \
                    (lrs) << (G_TEXTURE_IMAGE_FRAC - 1), (lrt) << G_TEXTURE_IMAGE_FRAC);                               \
        gDPPipeSync(pkt);                                                                                              \
        gDPSetTile(pkt, fmt, G_IM_SIZ_4b, gDPTileLineLrs(G_IM_SIZ_4b_LINE_SHIFT, uls, lrs), 0, G_TX_RENDERTILE, pal,   \
                   cmt, maskt, shiftt, cms, masks, shifts);                                                            \
        gDPSetTileSize(pkt, G_TX_RENDERTILE, (uls) << G_TEXTURE_IMAGE_FRAC, (ult) << G_TEXTURE_IMAGE_FRAC,             \
                       (lrs) << G_TEXTURE_IMAGE_FRAC, (lrt) << G_TEXTURE_IMAGE_FRAC);                                  \
    }
typedef struct {
    s32 image, palette, format, size;
    u32 width, height, alignedWidth;
} Texture47C;
/// Render a scaled region of a CI4 or RGBA16 image.
/// \param out Display-list cursor with sufficient capacity; receives drawing commands and advances past them.
/// \param texture Borrowed, initialized CI4 or RGBA16 image descriptor.
/// \param x Destination screen X coordinate in pixels.
/// \param y Destination screen Y coordinate in pixels.
/// \param drawWidth Requested screen width in pixels, or zero to use the image width.
/// \param drawHeight Requested screen height in pixels, or zero to use the image height.
/// \param sourceX Source X offset in texels.
/// \param sourceY Source Y offset in texels.
/// \param wrapS Texture addressing mode along S.
/// \param repeat Nonzero to permit horizontal texture repetition.
/// \param scaleX Positive finite horizontal scale; the texture step and resulting coordinates must be representable.
/// \param scaleY Positive finite vertical scale; the texture step and resulting coordinates must be representable.
/// \returns Nothing.
/// \pre Texture and palette data must remain RDP-readable until rendering finishes; source row padding must match the descriptor.
/// \pre The effective source height must be positive; source offsets and extents must remain within readable image data.
/// \pre At least two padded source rows must fit texture memory unless the effective source height is one.
/// \note Portability: The row-expression spellings preserve the original IDO register allocation.
/// \note Original decompilation identifier: \c func_80047C48 (0x80047C48).
void textureDrawScaledRegion(Gfx **out, Texture47C *texture, u32 x, u32 y, u32 drawWidth, u32 drawHeight, u32 sourceX,
                   u32 sourceY, u32 wrapS, u32 repeat, f32 scaleX, f32 scaleY) {
    Gfx *dl = *out;
    u32 mask;
    u32 dimension;
    s32 step;
    s32 row;
    s32 index;
    s32 top;
    s32 bottom;
    s32 ds;
    s32 dt;
    s32 image = texture->image;
    s32 palette = texture->palette;
    s32 size = texture->size;
    u32 alignedWidth = texture->alignedWidth;
    s32 nextRow;
    if (drawWidth == 0)
        drawWidth = texture->width;
    if (drawHeight == 0)
        drawHeight = texture->height;
    drawHeight = (u32)((f32)drawHeight / scaleY);
    index = 0;
    drawHeight = (drawHeight > texture->height) ? texture->height : drawHeight;
    dimension = drawWidth;
    if (repeat) {
        for (;;) {
            dimension >>= 1;
            if (dimension == 0)
                break;
            index++;
        }
        mask = index;
    } else {
        if ((f32)texture->width * scaleX < (f32)drawWidth)
            drawWidth = (u32)((f32)texture->width * scaleX);
        else
            drawWidth = (u32)(f32)drawWidth;
        mask = 0;
    }
    dt = (s32)((f64)(1024.0f / scaleY) + 0.5);
    ds = (s32)((f64)(1024.0f / scaleX) + 0.5);
    gDPPipeSync(dl++);
    gDPSetCycleType(dl++, G_CYC_1CYCLE);
    gDPSetTexturePersp(dl++, G_TP_NONE);
    gDPSetTextureLOD(dl++, G_TL_TILE);
    gDPSetTextureFilter(dl++, G_TF_BILERP);
    gDPSetTextureConvert(dl++, G_TC_FILT);
    gDPSetTextureDetail(dl++, G_TD_CLAMP);
    switch (size) {
    case 0:
        gDPLoadTLUT_pal16(dl++, 0, palette);
        gDPSetTextureLUT(dl++, G_TT_RGBA16);
        gDPLoadTLUT_pal16(dl++, 0, palette);
        dimension = 4096 / alignedWidth;
        step = (dimension < drawHeight) ? dimension : drawHeight;
        step = step - 1;
        for (index = 0; index * step < drawHeight - step - 1; index++) {
            top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
            nextRow = (index + 1) * step;
            bottom = (s32)((u32)((s32)((f32)y + (f32)nextRow * scaleY)) << 2);
            gDPLoadTextureTileLrs_4b(dl++, image + ((alignedWidth * (index * step + sourceY)) >> 1), G_IM_FMT_CI,
                                     alignedWidth, drawHeight, 0, 0, (s32)alignedWidth - 1, step, 0, wrapS, G_TX_CLAMP,
                                     mask, 0, 0, 0);
            gSPTextureRectangle(dl++, x * 4, top, (x + drawWidth) * 4, bottom, 0, sourceX << 5, 0, ds, dt);
        }
        row = index * step;
        top = (s32)((u32)((s32)((f32)y + (f32)row * scaleY)) << 2);
        bottom = (s32)((u32)((s32)((f32)y + (f32)drawHeight * scaleY)) << 2);
        gDPLoadTextureTileCopyLrs_4b(dl++, image + ((alignedWidth * (index * step + sourceY)) >> 1), G_IM_FMT_CI,
                                     alignedWidth, drawHeight, 0, 0, (s32)alignedWidth - 1,
                                     drawHeight - index * step - 1, 0, wrapS, G_TX_CLAMP, mask, 0, 0, 0);
        gSPTextureRectangle(dl++, x * 4, top, (x + drawWidth) * 4, bottom, 0, sourceX << 5, 0, ds, dt);
        break;
    case 2:
        gDPSetTextureLUT(dl++, G_TT_NONE);
        dimension = 2048 / alignedWidth;
        step = (dimension < drawHeight) ? dimension : drawHeight;
        step = step - 1;
        for (index = 0; index * step < drawHeight - step - 1; index++) {
            top = (s32)((u32)((s32)((f32)y + (f32)(index * step) * scaleY)) << 2);
            nextRow = (index + 1) * step;
            bottom = (s32)((u32)((s32)((f32)y + (f32)nextRow * scaleY)) << 2);
            gDPLoadTextureTileLrs(dl++, image + (alignedWidth * (index * step + sourceY) * 2), G_IM_FMT_RGBA,
                                  G_IM_SIZ_16b, alignedWidth, drawHeight, 0, 0, (s32)alignedWidth - 1, step, 0, wrapS,
                                  G_TX_CLAMP, mask, 0, 0, 0);
            gSPTextureRectangle(dl++, x * 4, top, (x + drawWidth) * 4, bottom, 0, sourceX << 5, 0, ds, dt);
        }
        row = index * step;
        top = (s32)((u32)((s32)((f32)y + (f32)row * scaleY)) << 2);
        bottom = (s32)((u32)((s32)((f32)y + (f32)drawHeight * scaleY)) << 2);
        gDPLoadTextureTileCopyLrs(dl++, image + alignedWidth * (index * step + sourceY) * 2, G_IM_FMT_RGBA,
                                  G_IM_SIZ_16b, alignedWidth, drawHeight, 0, 0, (s32)alignedWidth - 1,
                                  drawHeight - index * step - 1, 0, wrapS, G_TX_CLAMP, mask, 0, 0, 0);
        gSPTextureRectangle(dl++, x * 4, top, (x + drawWidth) * 4, bottom, 0, sourceX << 5, 0, ds, dt);
        break;
    }
    *out = dl;
}

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
void *heapAllocLocal(HpHdr *, s32, s32, void *);
void _bzero(void *, s32);

/// Initialize an empty map of integer keys to nonzero word values.
/// \param map Caller-owned map storage without an unreleased backing allocation.
/// \param capacity Positive entry capacity whose total allocation size fits in signed 32 bits.
/// \returns Nothing.
/// \note Portability: The allocator call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_80048CE0 (0x80048CE0).
void wordMapInitialize(WordMap *map, s32 capacity) {
    s32 len;

    map->count = 0;
    map->size = capacity;
    len = capacity * 8;
    map->slots = heapAllocLocal(&gGameHeap, len, 0x52, map);
    _bzero(map->slots, len);
}

void heapFreeLocal(HpHdr *, void *, u8 *);

/// Release a word map and reset it to an empty unallocated state.
/// \param map Initialized map whose storage is released.
/// \returns Nothing.
/// \note Portability: The deallocator call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_80048D44 (0x80048D44).
void wordMapDestroy(u8 *map) {
    heapFreeLocal(&gGameHeap, (void *)((WordMap *)map)->slots, map);
    ((WordMap *)map)->slots = NULL;
    ((WordMap *)map)->size = 0;
    ((WordMap *)map)->count = 0;
}

/// Add a key and nonzero word value to an initialized map.
/// \param map Initialized map with positive capacity.
/// \param value Nonzero word value to store.
/// \param key Integer lookup key; duplicate keys add entries and do not replace existing values.
/// \returns One when inserted, or zero when no reachable free entry is available.
/// \note Original decompilation identifier: \c func_80048D84 (0x80048D84).
s32 wordMapInsert(WordMap *map, s32 value, u32 key) {
    u32 h;
    u32 i;
    WordMapEntry *p;
    u32 step;

    if (map->count >= map->size) {
        return 0;
    }
    h = (((~(key >> 12) + (key >> 8)) ^ ~(key >> 4)) + key * 0x2E7) % map->size;
    i = h;
    p = &map->slots[i];
    while (p->val != 0) {
        step = 8 - (key & 7);
        i = (i + step) % map->size;
        p = &map->slots[i];
        if (i == h) {
            return 0;
        }
    }
    p->key = key;
    p->val = value;
    map->count++;
    return 1;
}

/// Look up a word value by its integer key.
/// \param map Initialized map with positive capacity.
/// \param key Integer key to find.
/// \returns The stored word value, or zero when the key is absent.
/// \note Original decompilation identifier: \c func_80048E78 (0x80048E78).
s32 wordMapLookup(WordMap *map, u32 key) {
    u32 h = (((~(key >> 12) + (key >> 8)) ^ ~(key >> 4)) + key * 0x2E7) % map->size;
    u32 i = h;
    WordMapEntry *p = &map->slots[i];
    u32 step;

    while (key != p->key) {
        if (p->val == 0) {
            return 0;
        }
        step = 8 - (key & 7);
        i = (i + step) % map->size;
        p = &map->slots[i];
        if (i == h) {
            return 0;
        }
    }
    return p->val;
}
