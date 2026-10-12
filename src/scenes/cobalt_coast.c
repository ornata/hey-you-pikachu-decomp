#include "common.h"

u8 *actorCreateForKind(s32, f32, f32, f32, f32, f32, f32, u8 *, s32);
void togepiCreate(f32, f32, f32, f32, f32, u8 *, s32);
u8 *ambientAudioCreate(f32 *, s32, f32, f32, s32, s32, f32, s32, s32);
void runtimeCheckResetCheckpoint(s32, s32);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Provide the coast-scene callback that requires no action.
/// \returns Nothing.
void cobaltCoastSceneNoOp(void) {
}

extern u8 D_80140BF8_scene_cobalt_coast[];

/// Spawn the fixed type-2 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType2(void) {
    actorCreateForKind(2, 320.0f, 10.0f, 225.0f, 0.0f, 0.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_80140BF8_scene_cobalt_coast, 0);
}

extern u8 D_80144CAC_scene_cobalt_coast[];

/// Spawn the fixed type-18 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType18(void) {
    actorCreateForKind(0x12, 254.0f, 15.0f, 370.0f, 0.0f, 180.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_80144CAC_scene_cobalt_coast, 0);
}

extern u8 D_801418D4_scene_cobalt_coast[];

/// Spawn the fixed type-3 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType3(void) {
    actorCreateForKind(3, 354.98f, 6.41f, 285.68f, 0.0f, 270.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_801418D4_scene_cobalt_coast, 0);
}

extern u8 D_8014263C_scene_cobalt_coast[];

/// Spawn the fixed type-4 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType4(void) {
    actorCreateForKind(4, 310.3f, 6.53f, 264.0f, 0.0f, 350.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_8014263C_scene_cobalt_coast, 0);
}

extern u8 D_80143334_scene_cobalt_coast[];

/// Spawn the fixed type-1 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType1(void) {
    actorCreateForKind(1, 284.64f, 6.19f, 294.3f, 0.0f, 50.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_80143334_scene_cobalt_coast, 0);
}

extern u8 D_80144010_scene_cobalt_coast[];

/// Spawn the fixed type-8 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType8(void) {
    actorCreateForKind(8, 364.38f, 6.01f, 310.02f, 0.0f, 312.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_80144010_scene_cobalt_coast, 0);
}

extern f32 D_801476B8_scene_cobalt_coast;
extern f32 D_801476BC_scene_cobalt_coast;

/// Spawn the coast resident enabled by saved progress.
/// \returns Nothing.
void cobaltCoastSpawnProgressResident(void) {
    togepiCreate(D_801476B8_scene_cobalt_coast, D_801476BC_scene_cobalt_coast, 6.01f, 310.02f, 90.0f,
                  D_80144010_scene_cobalt_coast, 3);
}

extern u8 D_801455D0_scene_cobalt_coast[];

/// Spawn the fixed type-11 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType11(void) {
    actorCreateForKind(0xB, 320.0f, 0.23f, 413.0f, 0.0f, 160.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_801455D0_scene_cobalt_coast, 0);
}

extern u8 D_801460FC_scene_cobalt_coast[];

/// Spawn the first fixed type-14 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnFirstResidentType14(void) {
    actorCreateForKind(0xE, 310.0f, 0.0f, 350.0f, 0.0f, 320.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_801460FC_scene_cobalt_coast, 0);
}

extern u8 D_80146A8C_scene_cobalt_coast[];

/// Spawn the second fixed type-14 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnSecondResidentType14(void) {
    actorCreateForKind(0xE, 320.0f, 0.0f, 353.0f, 0.0f, 320.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_80146A8C_scene_cobalt_coast, 0);
}

extern f32 D_80146AA0_scene_cobalt_coast[];

/// Create the coast scene's ambient audio emitter.
/// \returns Nothing.
void cobaltCoastCreateAmbientAudio(void) {
    ambientAudioCreate(D_80146AA0_scene_cobalt_coast, 1, 4.0f, 4.0f, 0x2B, 0x2B, 1.0f, 4,
                  (s32)D_80146AA0_scene_cobalt_coast);
}

extern u8 D_80147674_scene_cobalt_coast[];

/// Populate the coast scene with residents, ambient audio, and scene events.
/// \returns Nothing.
void cobaltCoastInitializeObjects(void) {
    if (progressGetValue(0x16U) != 0) {
        cobaltCoastSpawnProgressResident();
    } else {
        cobaltCoastSpawnResidentType8();
    }
    cobaltCoastSpawnResidentType2();
    cobaltCoastSpawnResidentType18();
    cobaltCoastSpawnResidentType3();
    cobaltCoastSpawnResidentType4();
    runtimeCheckResetCheckpoint((s32)D_80147674_scene_cobalt_coast, 1);
    cobaltCoastSpawnResidentType1();
    cobaltCoastSpawnResidentType11();
    cobaltCoastSpawnFirstResidentType14();
    cobaltCoastSpawnSecondResidentType14();
    cobaltCoastCreateAmbientAudio();
}

extern s32 *D_8014F520_scene_cobalt_coast[64];
/// Look up the coast spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 cobaltCoastGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 5.0);
    if (iy < 0 || iy >= 64 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 64 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 64)
        return 0;
    layer = D_8014F520_scene_cobalt_coast[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 64 + ix];
}
