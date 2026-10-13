#include "texture_descriptor.h"
#include "texture_setup.h"
#include "texture_rectangles.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "texture_cache.h"
#include "texture_draw_types.h"


/// Initialize the shared texture cache.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C140 (0x8000C140).
void textureCacheInitialize(void) {
    romCacheInitialize(&gTextureRomCache, 0x1E000, 0x3D, 0x20);
}


/// Release the shared texture cache.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C174 (0x8000C174).
void textureCacheDestroy(void) {
    romCacheDestroy(&gTextureRomCache);
}


/// Texture descriptor tables indexed by texture bank.
/// \note Original identifier: \c D_8006B520 (0x8006B520).
extern TextureDescriptor *gTextureDescriptorTables[];

/// Number of texture descriptors in each bank.
/// \note Original identifier: \c D_8006B54C (0x8006B54C).
extern s32 gTextureDescriptorCounts[];
s32 textureCacheResolve(TextureDescriptor, s32 *, s32 *);

/// Get a texture descriptor with RAM addresses for its image and palette.
/// \param table Texture-table index.
/// \param index Valid texture index within the selected table.
/// \param descriptor Receives the resolved descriptor; pending cache reads must
/// complete before its image or palette is used.
/// \returns Nothing.
///
/// \note NONPORTABLE: the empty `if` is a compiled-out bounds check; the ROM still evaluates it.
/// \note Original decompilation identifier: \c func_8000C198 (0x8000C198).
void textureGetDescriptor(s32 table, s32 index, TextureDescriptor *descriptor) {
    s32 image;
    s32 palette;

    if (index < 0 || index >= gTextureDescriptorCounts[table]) {
    }
    *descriptor = gTextureDescriptorTables[table][index];
    textureCacheResolve(*descriptor, &image, &palette);
    descriptor->image = image;
    descriptor->palette = palette;
}

#include "texture_tiles.h"

/// Prepare a table texture for rendering.
/// \param displayList Display-list cursor advanced by the texture setup commands.
/// \param table Texture-table index.
/// \param index Valid texture index within the selected table.
/// \returns The display-list cursor before texture setup.
/// \note Original decompilation identifier: \c func_8000C26C (0x8000C26C).
Gfx *textureLoadForDraw(Gfx **displayList, s32 table, s32 index) {
    Gfx *head;
    TextureDescriptor t;

    head = *displayList;
    textureGetDescriptor(table, index, &t);
    gDPPipeSync((*displayList)++);
    if (t.format == G_IM_FMT_CI) {
        gDPSetTextureLUT((*displayList)++, G_TT_RGBA16);
    } else {
        gDPSetTextureLUT((*displayList)++, G_TT_NONE);
    }
    textureUploadTile(displayList, t.image, t.palette, t.format, t.size, t.width, t.height, 2, 2, 0, 0);
    return head;
}

/// Segmented shared display list preparing textured screen rectangle drawing.
/// \note Original identifier: \c D_100C8C8 (0x100C8C8).
extern Gfx gTextureRectangleSetupDisplayList[];
/// Positive near-zero scale threshold for textured rectangle drawing.
/// \note Original identifier: \c D_800A0CBC (0x800A0CBC).
/// Negative near-zero horizontal scale threshold for textured rectangle drawing.
/// \note Original identifier: \c D_800A0CC0 (0x800A0CC0).
/// Negative near-zero vertical scale threshold for textured rectangle drawing.
/// \note Original identifier: \c D_800A0CC4 (0x800A0CC4).
extern f32 gTextureScalePositiveTolerance, gTextureScaleXNegativeTolerance, gTextureScaleYNegativeTolerance;
/// Render a list of textured screen rectangles.
/// \param table Texture-table index used by the rectangle records.
/// \param draw Rectangle records terminated by a negative texture index; scales must be nonzero.
/// \param gdl Display-list cursor receiving the drawing commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C354 (0x8000C354).
void textureDrawRectangles(s32 table, TextureDrawC354 *draw, Gfx **gdl) {
    s32 width;
    TextureDescriptor texture;
    s32 height;
    s32 useMaskS, useMaskT;
    f32 ds;
    f32 dt;
    if (gdl == NULL) {
    }
    if (draw == NULL) {
    }
    if (draw->texture < 0) {
    }
    if (draw->scaleX < gTextureScalePositiveTolerance && gTextureScaleXNegativeTolerance < draw->scaleX)
        return;
    if (draw->scaleY < gTextureScalePositiveTolerance && gTextureScaleYNegativeTolerance < draw->scaleY)
        return;
    gSPDisplayList((*gdl)++, gTextureRectangleSetupDisplayList);
    while (draw->texture >= 0) {
        textureGetDescriptor(table, draw->texture, &texture);
        gDPPipeSync((*gdl)++);
        if (texture.format == G_IM_FMT_CI) {
            gDPSetTextureLUT((*gdl)++, G_TT_RGBA16);
        } else {
            gDPSetTextureLUT((*gdl)++, G_TT_NONE);
        }
        useMaskS = (texture.width & (texture.width - 1)) ? 0 : 1;
        useMaskT = (texture.height & (texture.height - 1)) ? 0 : 1;
        textureUploadTile(gdl, texture.image, texture.palette, texture.format, texture.size, texture.width, texture.height, 0, 0, useMaskS,
                      useMaskT);
        width = draw->width >= 0 ? draw->width : texture.width;
        height = draw->height >= 0 ? draw->height : texture.height;
        gSPTextureRectangle((*gdl)++, (u32)draw->x << 2, (u32)draw->y << 2,
                            (s32)((draw->x + width * draw->scaleX) * 4.0f),
                            (s32)((draw->y + height * draw->scaleY) * 4.0f), G_TX_RENDERTILE, (u32)draw->s << 5,
                            ((u32)texture.height - (u32)draw->t - 1U) << 5,
                            (s32)((ds = 1024.0f / draw->scaleX) >= 0.0f ? ds + 0.5f : ds - 0.5f),
                            (s32)((dt = -1024.0f / draw->scaleY) >= 0.0f ? dt + 0.5f : dt - 0.5f));
        draw++;
    }
    gSPTexture((*gdl)++, 0x7FFF, 0, 0, 0, G_OFF);
    gMoveWd((*gdl)++, G_MW_FOG, 0x10, 0x7FFF);
}

/// Cartridge ROM start address of the texture image and palette archive.
/// \note Original identifier: \c D_D69490 (0xD69490).
extern u8 gTextureArchiveRomStart[];

/// Request a cached texture image and its required palette.
/// \param texture Texture descriptor identifying the image, format, and palette.
/// \param image Receives the image address; pending reads must complete before use.
/// \param palette Receives the palette address, or zero for a texture without a palette;
/// pending reads must complete before use.
/// \returns For a color-indexed texture, one when ready or zero while reads are
/// pending; otherwise three when ready or four while the image read is pending.
///
/// \note NONPORTABLE: the unused `pad` local only sizes the frame.
/// \note Original decompilation identifier: \c func_8000C704 (0x8000C704).
s32 textureCacheResolve(TextureDescriptor texture, s32 *image, s32 *palette) {
    s32 pad; /* unused, sizes the frame */
    s32 rp;
    s32 r;
    s32 size;

    size = (((texture.width >= 17) ? texture.width : 16) * texture.height << texture.size) >> 1;
    r = romCacheRead(&gTextureRomCache, texture.image + gTextureArchiveRomStart, size, image);
    if (texture.format != G_IM_FMT_CI) {
        *palette = 0;
        return r;
    }
    rp = romCacheRead(&gTextureRomCache, texture.palette + gTextureArchiveRomStart, 0x20, palette);
    if (r == 4 || rp == 4) {
        return 0;
    }
    return 1;
}
