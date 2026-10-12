/// \file
/// Declares item-object destruction and material selection contracts.

#ifndef ITEM_OBJECT_H
#define ITEM_OBJECT_H

#include <ultra64.h>

/// Remove an item from world participation and request its destruction.
/// \param item Live, registered item to destroy; must not already be pending destruction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80113B78 (0x80113B78).
void itemDestroy(u8 *item);

/// Start an item material animation from its beginning.
/// \param item Initialized item object receiving the animation.
/// \param animation Valid integer animation-track identifier.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801145D0 (0x801145D0).
void itemStartMaterialAnimation(u8 *item, s32 animation);

/// Select a fixed item material frame and stop material animation.
/// \param item Initialized item object receiving the material selection.
/// \param frame Valid material texture frame identifier representable as a signed halfword.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114614 (0x80114614).
void itemSetMaterialFrame(u8 *item, s32 frame);

#endif
