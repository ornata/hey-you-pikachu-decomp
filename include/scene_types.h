/// \file
/// Defines scene geometry, entrance poses, paths and small interaction records.

#ifndef SCENE_TYPES_H
#define SCENE_TYPES_H

#include <ultra64.h>
#include "maze_types.h"
#include "asset_loader.h"
#include "passage_types.h"

/// Classify a world position for the configured maze.
/// \param x World-space X coordinate.
/// \param y World-space Y coordinate.
/// \param z World-space Z coordinate.
/// \returns The scene's occupancy classification; zero denotes an empty cell.
typedef s32 (*SceneCellSampler)(f32 x, f32 y, f32 z);

/// Borrowed coordinate table for one scene path.
typedef struct ScenePathEntry {
    s32 count;        ///< Number of three-coordinate points.
    f32 (*points)[3]; ///< Borrowed world-space path points.
} ScenePathEntry;

/// Rectangular region state owned by a scene-region object.
typedef struct SceneRegionState {
    f32 width;      ///< Region half-extent along X.
    f32 depth;      ///< Region half-extent along Z.
    u8 highlighted; ///< Nonzero while the region is highlighted.
    u8 unknown009[3];
    s32 sceneData; ///< Opaque associated scene-data value.
} SceneRegionState;

/// Interaction anchors and landmark retained by a cave object.
typedef struct CaveState {
    f32 firstAnchor[3];  ///< First scene anchor in world coordinates.
    f32 secondAnchor[3]; ///< Second scene anchor copied into the cave state.
    u8 *landmark;        ///< Borrowed associated landmark object, or null before association.
    u8 descriptionKind;  ///< Description selector; zero selects the supported description.
    u8 unknown01D[3];
} CaveState;

/// Player and companion spawn poses selected together by an area entrance.
typedef struct SceneSpawnPoint {
    f32 playerPosition[3]; ///< Player world-space spawn position.
    f32 playerYaw;         ///< Initial player view yaw in degrees.
    f32 actorPosition[3];  ///< Companion world-space spawn position.
    f32 actorYaw;          ///< Initial companion facing in degrees.
} SceneSpawnPoint;

/// Recovered scene configuration prefix used by maze and scene initialization.
/// This view is smaller than the complete configuration and must not be used
/// to allocate or copy it.
typedef struct SceneConfigurationPrefix {
    f32 cellDimensions[3];         ///< Maze cell dimensions along the X, Y and Z axes.
    f32 objectScale;               ///< Default scale for scene objects and regions.
    s32 subdivisionPower;          ///< Power of two used for maze subdivision.
    SceneCellType *cellTypes;      ///< Borrowed resource table indexed by occupancy classification.
    SceneEventLink *eventLinks;    ///< Borrowed event-link list, terminated by a null handler; null for none.
    u8 unknown01C[4];              ///< Offset 0x1C; role is unproven, so preserve these bytes unchanged.
    SceneSpawnPoint *spawns;       ///< Borrowed player and companion entrance poses.
    f32 farClip;                   ///< Far clipping distance for area visibility.
    f32 nearClip;                  ///< Near clipping distance for area visibility.
    f32 projectionExtent;          ///< Scene extent used to scale camera projection and maze visibility.
    s32 fogStart;                  ///< Scene fog start value.
    s32 fogEnd;                    ///< Scene fog end value.
    u8 fogColor[4];                ///< Fog red, green, blue and alpha components.
    SceneCellSampler sampleCell;   ///< Classify a world position for maze construction.
    void (*initializeScene)(void); ///< Optional area-specific initializer after common gameplay setup.
    SceneItemPlacement *items;     ///< Borrowed, negative-kind-terminated item placements.
    AssetDescriptor
        renderAsset;            ///< Encoded area graphics and correction metadata; decoded storage is owned separately.
    s32 backdrop;               ///< Backdrop selected for the primary view.
    f32 backdropVerticalOffset; ///< Scene-specific vertical backdrop offset in screen pixels.
    u8 **actorScripts;          ///< Borrowed companion programs selected by scenario.
    ScenePathEntry *paths;      ///< Borrowed scene path table.
} SceneConfigurationPrefix;

typedef char SceneConfigurationPrefixSizeCheck[(sizeof(SceneConfigurationPrefix) == 0x74) ? 1 : -1];
typedef char
    SceneConfigurationScaleOffsetCheck[(((u32) & (((SceneConfigurationPrefix *)0)->objectScale)) == 0xC) ? 1 : -1];

typedef char SceneSpawnPointSizeCheck[(sizeof(SceneSpawnPoint) == 0x20) ? 1 : -1];
typedef char ScenePathEntrySizeCheck[(sizeof(ScenePathEntry) == 8) ? 1 : -1];
typedef char SceneRegionStateSizeCheck[(sizeof(SceneRegionState) == 0x10) ? 1 : -1];
typedef char CaveStateSizeCheck[(sizeof(CaveState) == 0x20) ? 1 : -1];
typedef char SceneSpawnsOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->spawns) == 0x20) ? 1 : -1];
typedef char SceneActorScriptsOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->actorScripts) == 0x6C) ? 1 : -1];
typedef char ScenePathsOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->paths) == 0x70) ? 1 : -1];
typedef char CaveLandmarkOffsetCheck[(((u32) & ((CaveState *)0)->landmark) == 0x18) ? 1 : -1];

typedef char SceneCellTypesOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->cellTypes) == 0x14) ? 1 : -1];
typedef char SceneSampleCellOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->sampleCell) == 0x3C) ? 1 : -1];
typedef char SceneInitializerOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->initializeScene) == 0x40) ? 1 : -1];
typedef char SceneItemsOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->items) == 0x44) ? 1 : -1];

typedef char
    SceneProjectionExtentOffsetCheck[((u32) & ((SceneConfigurationPrefix *)0)->projectionExtent == 0x2C) ? 1 : -1];
typedef char SceneFogColorOffsetCheck[((u32) & ((SceneConfigurationPrefix *)0)->fogColor == 0x38) ? 1 : -1];
typedef char SceneRenderAssetOffsetCheck[((u32) & ((SceneConfigurationPrefix *)0)->renderAsset == 0x48) ? 1 : -1];
typedef char
    SceneBackdropOffsetCheck[((u32) & ((SceneConfigurationPrefix *)0)->backdropVerticalOffset == 0x68) ? 1 : -1];
typedef char ScenePlayerYawOffsetCheck[((u32) & ((SceneSpawnPoint *)0)->playerYaw == 0xC) ? 1 : -1];
typedef char SceneActorPositionOffsetCheck[((u32) & ((SceneSpawnPoint *)0)->actorPosition == 0x10) ? 1 : -1];
typedef char SceneActorYawOffsetCheck[((u32) & ((SceneSpawnPoint *)0)->actorYaw == 0x1C) ? 1 : -1];

#endif
