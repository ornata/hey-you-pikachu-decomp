/// \file
/// Declares item behavior-mode changes and their world-participation contract.

#ifndef ITEM_MODE_H
#define ITEM_MODE_H

#include "ultra64.h"

/// Change an item's behavior mode and apply the mode's participation rules.
/// \param item Initialized item object to configure.
/// \param mode Supported item behavior mode from zero through six.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011443C (0x8011443C).
void itemSetMode(u8 *item, s32 mode);

#endif
