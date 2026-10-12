/// \file
/// Defines scene item placements consumed by the maze population interface.

#ifndef ITEM_SPAWN_H
#define ITEM_SPAWN_H

#include <ultra64.h>

/// Borrowed scene placement; an entry with type zero terminates a placement list.
typedef struct ItemSpawn {
    s32 type;       ///< Item catalog kind; zero terminates the list.
    f32 x, y, z;    ///< Initial world position.
    f32 yaw, pitch; ///< Initial orientation in degrees.
} ItemSpawn;

typedef char ItemSpawnSizeCheck[(sizeof(ItemSpawn) == 0x18) ? 1 : -1];
typedef char ItemSpawnYawOffsetCheck[((u32) & ((ItemSpawn *)0)->yaw == 0x10) ? 1 : -1];

#endif
