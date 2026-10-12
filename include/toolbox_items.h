/// \file
/// Declares replacement of the toolbox bonus and purchase items.

#ifndef TOOLBOX_ITEMS_H
#define TOOLBOX_ITEMS_H

#include <ultra64.h>

/// Replace the stored bonus item, requesting destruction of the previous item if present.
/// \param object Live initialized item distinct from the previous bonus item; the toolbox retains its handle.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002F2F0 (0x8002F2F0).
void toolboxStoreBonusObject(u8 *object);

/// Replace the stored purchase item, requesting destruction of the previous item if present.
/// \param object Live initialized item distinct from the previous purchase item; the toolbox retains its handle.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002F338 (0x8002F338).
void toolboxStorePurchasedObject(u8 *object);

#endif
