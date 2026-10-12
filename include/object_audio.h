/// \file
/// Declares sound playback positioned at live runtime objects.

#ifndef OBJECT_AUDIO_H
#define OBJECT_AUDIO_H

#include <ultra64.h>

/// Play a sound effect at an object position with spatial volume and panning.
/// \param obj Live object emitting the sound; with an active camera matrix, its camera-space position must have nonzero length.
/// \param kind Valid sound effect identifier in the loaded bank.
/// \param near Distance at which volume attenuation begins; must be less than far.
/// \param far Distance at which the sound reaches minimum volume; must be greater than near.
/// \returns Playback handle returned by the sound-effect player.
/// \note Original decompilation identifier: \c func_80009BB8 (0x80009BB8).
s32 objectPlaySpatialSound(u8 *obj, s32 kind, f32 near, f32 far);

#endif
