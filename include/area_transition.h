/// \file
/// Declares destination selection and requests for area/entrance transitions.

#ifndef AREA_TRANSITION_H
#define AREA_TRANSITION_H

#include "ultra64.h"

/// Select the destination area and spawn for the next area load.
/// \param area Destination area identifier, wrapped through the 23 supported areas.
/// \param spawn Valid spawn index in the destination area's configuration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E598 (0x8010E598).
void areaSetNextDestination(s32 area, s32 spawn);

/// Request a transition to the selected area and destination spawn.
/// \param area Destination area identifier, wrapped through the 23 supported areas.
/// \param spawn Valid spawn index in the destination area's configuration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E5E4 (0x8010E5E4).
void areaRequestTransition(s32 area, s32 spawn);

#endif
