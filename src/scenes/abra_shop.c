#include "common.h"
#include "actor_create.h"

void *itemCreate(f32, f32, f32, f32, f32, s32);
typedef struct {
    u8 unknown[12];
    f32 scale;
} SceneScale;
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern SceneScale *gCurrentAreaConfiguration;
extern u8 D_801431F8_scene_abra_shop[], D_8014363C_scene_abra_shop[];

/// Set up shop residents and any special encounter enabled by saved game state.
/// \returns Nothing.
void abraShopInitializeObjects(void) {
    s32 level;
    f32 scale;
    u8 *companion;
    u8 *state;

    actorCreateForKind(0xF, 100.0f, 1.2f, 95.0f, 0, 0, gCurrentAreaConfiguration->scale * 1.5f,
                       D_801431F8_scene_abra_shop, 0);
    if (progressGetValue(0x15) != 0 && progressGetValue(0x16) == 0) {
        level = progressGetValue(0x17);
        scale = (f32)level;
        actorCreateForKind(0x17, 100.0f, 0.0f, 0.0f, 0, 0,
                           ((scale / 5.0f + 1.0f) * (gCurrentAreaConfiguration->scale * 0.5f)),
                           D_8014363C_scene_abra_shop, 0);
    }
    if (progressGetCurrentChapter() == 8) {
        companion = itemCreate(145.0f, 1.5f, 150.0f, 0.0f, 0.0f, 0x8F);
        state =
            *(u8 **)(actorCreateForKind(0x15, 0.0f, 0.0f, 0.0f, 0.0f, 45.0f, gCurrentAreaConfiguration->scale, 0, 0) +
                     0x54);
        *(f32 *)(state + 0x1EC) = *(f32 *)(companion + 0x44);
        *(f32 *)(state + 0x1F0) = *(f32 *)(companion + 0x48);
        *(f32 *)(state + 0x1F4) = *(f32 *)(companion + 0x4C);
        *(s32 *)(state + 0x198) = *(s32 *)(companion + 0x38);
        *(s32 *)(state + 0x19C) = 1;
    }
}

/// Provide the shop scene callback that requires no action.
/// \returns Nothing.
void abraShopSceneNoOp(void) {
}

extern s32 *D_8014743C_scene_abra_shop[8];
/// Look up the shop spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 abraShopGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;

    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 8 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 8 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 8) {
        return 0;
    }
    layer = D_8014743C_scene_abra_shop[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[iz * 8 + ix];
}
