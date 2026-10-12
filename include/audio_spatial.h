/// \file
/// Declares camera-relative volume and stereo-pan evaluation for sound positions.

#ifndef AUDIO_SPATIAL_H
#define AUDIO_SPATIAL_H

#include "ultra64.h"

/// Evaluate volume and stereo pan for a world-space sound position.
/// \param position Three world-space coordinates whose camera-space length is nonzero.
/// \param camera Camera transformation matrix, passed by value.
/// \param volume Receives distance-dependent volume from 80 through 256.
/// \param pan Receives stereo pan from zero through 255, with 128 centered.
/// \param nearDistance Distance at maximum volume, less than farDistance.
/// \param farDistance Distance at minimum volume, greater than nearDistance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C944 (0x8000C944).
void audioGetPositionVolumeAndPan(f32 *position, Mtx camera, s32 *volume, s32 *pan,
                                  f32 nearDistance, f32 farDistance);

#endif
