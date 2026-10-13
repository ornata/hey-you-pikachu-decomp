/// \file
/// Declares the borrowed Pikachu actor reference retained for the active area.

#ifndef PIKACHU_ACTOR_H
#define PIKACHU_ACTOR_H

#include "object_types.h"

/// Active area's Pikachu runtime object, or null before creation.
/// The object manager owns its storage. Retained references must not outlive the active area's actor hierarchy.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern RuntimeObjectPrefix *gPikachuActor;

#endif
