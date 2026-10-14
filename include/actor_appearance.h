/// \file
/// Declares model variant and scale controls for two actor appearance slots.

#ifndef ACTOR_APPEARANCE_H
#define ACTOR_APPEARANCE_H

#include "ultra64.h"

/// Bounds of the retained appearance slots and their model-variant tables.
enum {
    ACTOR_APPEARANCE_SLOT_COUNT = 2, ///< Two independently selected model parts.
    ACTOR_APPEARANCE_VARIANT_COUNT = 6 ///< Default model plus five table-selected variants.
};

/// Find the actor model part controlled by an appearance slot.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \returns A borrowed scale-component view of the actor-owned model part, or null when absent.
/// \note Original decompilation identifier: \c func_80014760 (0x80014760).
f32 *actorAppearanceGetPart(u8 *actor, s32 slot);

/// Set the uniform scale of an actor appearance part.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \param scale Uniform scale applied to the selected part, which must exist.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001478C (0x8001478C).
void actorAppearanceSetScale(u8 *actor, s32 slot, f32 scale);

/// Query the scale of an actor appearance part.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \returns The selected part scale along its first axis; the part must exist.
/// \note Original decompilation identifier: \c func_800147C0 (0x800147C0).
f32 actorAppearanceGetScale(u8 *actor, s32 slot);

/// Select the model variant for an actor appearance slot.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \param variant Model-variant identifier from zero through five; zero uses the part default model.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800147E4 (0x800147E4).
void actorAppearanceSetVariant(u8 *actor, s32 slot, s32 variant);

/// Query the model variant selected for an actor appearance slot.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \returns The selected model-variant identifier, or zero for the default model.
/// \note Original decompilation identifier: \c func_800147F8 (0x800147F8).
s32 actorAppearanceGetVariant(u8 *actor, s32 slot);

/// Restore default model variants and unit scale for both actor appearance slots.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001480C (0x8001480C).
void actorAppearanceReset(u8 *actor);

#endif
