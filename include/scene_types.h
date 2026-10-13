/// \file
/// Defines scene geometry, entrance poses, paths and small interaction records.

#ifndef SCENE_TYPES_H
#define SCENE_TYPES_H

#include <ultra64.h>
#include "maze_types.h"

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

/// Destination and access state owned by a door or sign object.
typedef struct PassageState {
    s32 kind;        ///< Description and destination category, zero through three.
    s32 destination; ///< Destination area or chase-stage identifier, depending on kind.
    s32 entrance;    ///< Entrance selected in the destination area.
    s32 accessState; ///< Nonzero when passage is blocked.
} PassageState;

/// Recovered scene configuration prefix used by maze and scene initialization.
/// This view is smaller than the complete configuration and must not be used
/// to allocate or copy it.
typedef struct SceneConfigurationPrefix {
    f32 cellDimensions[3]; ///< Maze cell dimensions along the X, Y and Z axes.
    f32 objectScale;       ///< Default scale for scene objects and regions.
    s32 subdivisionPower;  ///< Power of two used for maze subdivision.
    SceneCellType *cellTypes; ///< Borrowed resource table indexed by occupancy classification.
    u8 unknown018[8];
    SceneSpawnPoint *spawns; ///< Borrowed player and companion entrance poses.
    f32 farClip;             ///< Far clipping distance for area visibility.
    f32 nearClip;            ///< Near clipping distance for area visibility.
    u8 unknown02C[4];
    s32 fogStart; ///< Scene fog start value.
    s32 fogEnd;   ///< Scene fog end value.
    u8 unknown038[4];
    s32 (*sampleCell)(f32, f32, f32); ///< Classify a world position for maze construction.
    void (*updateMaze)(void);         ///< Optional area update callback.
    SceneItemPlacement *items;        ///< Borrowed, negative-kind-terminated item placements.
    u8 unknown048[0x1C];
    s32 backdrop; ///< Backdrop selected for the primary view.
    u8 unknown068[4];
    u8 **actorScripts;     ///< Borrowed companion programs selected by scenario.
    ScenePathEntry *paths; ///< Borrowed scene path table.
} SceneConfigurationPrefix;

typedef char SceneConfigurationPrefixSizeCheck[(sizeof(SceneConfigurationPrefix) == 0x74) ? 1 : -1];
typedef char
    SceneConfigurationScaleOffsetCheck[(((u32) & (((SceneConfigurationPrefix *)0)->objectScale)) == 0xC) ? 1 : -1];

typedef char SceneSpawnPointSizeCheck[(sizeof(SceneSpawnPoint) == 0x20) ? 1 : -1];
typedef char PassageStateSizeCheck[(sizeof(PassageState) == 0x10) ? 1 : -1];
typedef char PassageKindOffsetCheck[(((u32) & ((PassageState *)0)->kind) == 0) ? 1 : -1];
typedef char PassageDestinationOffsetCheck[(((u32) & ((PassageState *)0)->destination) == 4) ? 1 : -1];
typedef char PassageEntranceOffsetCheck[(((u32) & ((PassageState *)0)->entrance) == 8) ? 1 : -1];
typedef char PassageAccessStateOffsetCheck[(((u32) & ((PassageState *)0)->accessState) == 0xC) ? 1 : -1];
typedef char ScenePathEntrySizeCheck[(sizeof(ScenePathEntry) == 8) ? 1 : -1];
typedef char SceneRegionStateSizeCheck[(sizeof(SceneRegionState) == 0x10) ? 1 : -1];
typedef char CaveStateSizeCheck[(sizeof(CaveState) == 0x20) ? 1 : -1];
typedef char SceneSpawnsOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->spawns) == 0x20) ? 1 : -1];
typedef char SceneActorScriptsOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->actorScripts) == 0x6C) ? 1 : -1];
typedef char ScenePathsOffsetCheck[(((u32) & ((SceneConfigurationPrefix *)0)->paths) == 0x70) ? 1 : -1];
typedef char CaveLandmarkOffsetCheck[(((u32) & ((CaveState *)0)->landmark) == 0x18) ? 1 : -1];

typedef char SceneCellTypesOffsetCheck[(((u32)&((SceneConfigurationPrefix *)0)->cellTypes) == 0x14) ? 1 : -1];
typedef char SceneSampleCellOffsetCheck[(((u32)&((SceneConfigurationPrefix *)0)->sampleCell) == 0x3C) ? 1 : -1];
typedef char SceneUpdateMazeOffsetCheck[(((u32)&((SceneConfigurationPrefix *)0)->updateMaze) == 0x40) ? 1 : -1];
typedef char SceneItemsOffsetCheck[(((u32)&((SceneConfigurationPrefix *)0)->items) == 0x44) ? 1 : -1];
#endif
