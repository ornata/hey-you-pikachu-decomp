/// \file
/// Declares borrowed display names for area identifiers.

#ifndef AREA_NAMES_H
#define AREA_NAMES_H

#include <ultra64.h>

/// Look up the short region name associated with an area.
/// \param area Area identifier from zero through 22.
/// \returns The persistent region-name string, or null when the area has no short region name.
/// \note Original decompilation identifier: \c func_8010E96C (0x8010E96C).
char *areaGetRegionName(s32 area);

#endif
