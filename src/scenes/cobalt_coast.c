/// Geometry of the cobalt coast spatial lookup grid.
enum { COBALT_COAST_GRID_LAYERS = 64, COBALT_COAST_GRID_SIDE = 64 };
/// Height of one lookup layer in world-coordinate units.
#define COBALT_COAST_GRID_LAYER_HEIGHT 5.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define COBALT_COAST_GRID_CELL_WIDTH 10.0

#include "runtime_control.h"
#include "ambient_audio.h"
#include "common.h"
#include "actor_create.h"

#include "togepi.h"
#include "scene_configuration.h"

/// Provide the coast-scene callback that requires no action.
/// \returns Nothing.
void cobaltCoastSceneNoOp(void) {
}

extern u8 D_80140BF8_scene_cobalt_coast[];

/// Spawn the fixed type-2 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType2(void) {
    actorCreateForKind(2, 320.0F, 10.0F, 225.0F, 0.0F, 0.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_80140BF8_scene_cobalt_coast, 0);
}

extern u8 D_80144CAC_scene_cobalt_coast[];

/// Spawn the fixed type-18 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType18(void) {
    actorCreateForKind(0x12, 254.0F, 15.0F, 370.0F, 0.0F, 180.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_80144CAC_scene_cobalt_coast, 0);
}

extern u8 D_801418D4_scene_cobalt_coast[];

/// Spawn the fixed type-3 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType3(void) {
    actorCreateForKind(3, 354.98F, 6.41F, 285.68F, 0.0F, 270.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_801418D4_scene_cobalt_coast, 0);
}

extern u8 D_8014263C_scene_cobalt_coast[];

/// Spawn the fixed type-4 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType4(void) {
    actorCreateForKind(4, 310.3F, 6.53F, 264.0F, 0.0F, 350.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_8014263C_scene_cobalt_coast, 0);
}

extern u8 D_80143334_scene_cobalt_coast[];

/// Spawn the fixed type-1 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType1(void) {
    actorCreateForKind(1, 284.64F, 6.19F, 294.3F, 0.0F, 50.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_80143334_scene_cobalt_coast, 0);
}

extern u8 D_80144010_scene_cobalt_coast[];

/// Spawn the fixed type-8 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType8(void) {
    actorCreateForKind(8, 364.38F, 6.01F, 310.02F, 0.0F, 312.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_80144010_scene_cobalt_coast, 0);
}

extern f32 D_801476B8_scene_cobalt_coast;
extern f32 D_801476BC_scene_cobalt_coast;

/// Spawn the coast resident enabled by saved progress.
/// \returns Nothing.
void cobaltCoastSpawnProgressResident(void) {
    togepiCreate(D_801476B8_scene_cobalt_coast, D_801476BC_scene_cobalt_coast, 6.01F, 310.02F, 90.0F,
                 D_80144010_scene_cobalt_coast, 3);
}

extern u8 D_801455D0_scene_cobalt_coast[];

/// Spawn the fixed type-11 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnResidentType11(void) {
    actorCreateForKind(0xB, 320.0F, 0.23F, 413.0F, 0.0F, 160.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_801455D0_scene_cobalt_coast, 0);
}

extern u8 D_801460FC_scene_cobalt_coast[];

/// Spawn the first fixed type-14 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnFirstResidentType14(void) {
    actorCreateForKind(0xE, 310.0F, 0.0F, 350.0F, 0.0F, 320.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_801460FC_scene_cobalt_coast, 0);
}

extern u8 D_80146A8C_scene_cobalt_coast[];

/// Spawn the second fixed type-14 resident for the coast scene.
/// \returns Nothing.
void cobaltCoastSpawnSecondResidentType14(void) {
    actorCreateForKind(0xE, 320.0F, 0.0F, 353.0F, 0.0F, 320.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                       D_80146A8C_scene_cobalt_coast, 0);
}

extern f32 D_80146AA0_scene_cobalt_coast[];

/// Create the coast scene's ambient audio emitter.
/// \returns Nothing.
void cobaltCoastCreateAmbientAudio(void) {
    ambientAudioCreate(D_80146AA0_scene_cobalt_coast, 1, 4.0F, 4.0F, 0x2B, 0x2B, 1.0F, 4,
                       (Vec3f *)D_80146AA0_scene_cobalt_coast);
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
    runtimeCheckResetCheckpoint((const char *)D_80147674_scene_cobalt_coast, 1);
    cobaltCoastSpawnResidentType1();
    cobaltCoastSpawnResidentType11();
    cobaltCoastSpawnFirstResidentType14();
    cobaltCoastSpawnSecondResidentType14();
    cobaltCoastCreateAmbientAudio();
}

extern s32 *D_8014F520_scene_cobalt_coast[COBALT_COAST_GRID_LAYERS];
/// Look up the coast spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x8013475C in the cobalt coast overlay.
s32 cobaltCoastGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / COBALT_COAST_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= COBALT_COAST_GRID_LAYERS || (ix = (s32)((f64)x / COBALT_COAST_GRID_CELL_WIDTH), ix < 0) ||
        ix >= COBALT_COAST_GRID_SIDE || (iz = (s32)((f64)z / COBALT_COAST_GRID_CELL_WIDTH), iz < 0) ||
        iz >= COBALT_COAST_GRID_SIDE) {
        return 0;
    }
    layer = D_8014F520_scene_cobalt_coast[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * COBALT_COAST_GRID_SIDE) + ix];
}
