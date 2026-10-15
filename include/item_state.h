/// \file
/// Defines the complete item-owned state allocation and its visual direction playback.

#ifndef ITEM_STATE_H
#define ITEM_STATE_H

#include "item_types.h"
#include "direction_animation_types.h"

/// State owned by one item, released with its separately owned lighting storage.
/// Unresolved bytes retain their offsets and are not assumed to be padding.
typedef struct ItemState {
    ItemStatePrefix common;       ///< Shared motion, interaction, material and registry state.
    u8 unknown06C[4];             ///< Unresolved bytes preceding visual direction playback.
    DirectionAnimation direction; ///< Owned paired lighting poses and playback state.
    u8 unknown0EC[4];             ///< Unresolved allocation tail.
} ItemState;

typedef char ItemStateSizeCheck[(sizeof(ItemState) == 0xF0) ? 1 : -1];
typedef char ItemStateCommonOffsetCheck[(((u32) & ((ItemState *)0)->common) == 0) ? 1 : -1];
typedef char ItemStateDirectionOffsetCheck[(((u32) & ((ItemState *)0)->direction) == 0x70) ? 1 : -1];
typedef char ItemStatePoseIndexOffsetCheck[(((u32) & ((ItemState *)0)->direction.poseIndex) == 0xE2) ? 1 : -1];
typedef char ItemStateTailOffsetCheck[(((u32) & ((ItemState *)0)->unknown0EC) == 0xEC) ? 1 : -1];

#endif
