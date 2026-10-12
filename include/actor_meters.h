/// \file
/// Declares adjustments to actor gameplay resource meters.

#ifndef ACTOR_METERS_H
#define ACTOR_METERS_H

#include "ultra64.h"

/// Adjust the actor secondary gameplay meter within its allowed range.
/// \param object Object whose owning actor has the meter to adjust.
/// \param delta Signed change to apply to the meter.
/// \returns The updated meter value, from 0 through 100.
/// \note Original decompilation identifier: \c func_800149D0 (0x800149D0).
f32 actorAdjustSecondaryMeter(u8 *object, f32 delta);

/// Adjust the actor action resource and request its HUD display when appropriate.
/// \param object Object whose owning actor has the action resource to adjust.
/// \param delta Signed change to apply to the action resource.
/// \returns The updated action resource, from 0 through 100.
/// \note Original decompilation identifier: \c func_80014A3C (0x80014A3C).
f32 actorAdjustActionMeter(u8 *object, f32 delta);

#endif
