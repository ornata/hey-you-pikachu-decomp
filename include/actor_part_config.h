/// \file
/// Declares actor-kind part selections for attachments and ground clearance.

#ifndef ACTOR_PART_CONFIG_H
#define ACTOR_PART_CONFIG_H

#include <ultra64.h>

/// Get the configured parts used to measure an actor's ground clearance.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Borrowed part-ID list; positive IDs end at a nonpositive value.
/// \note Original decompilation identifier: \c func_8001E5F0 (0x8001E5F0).
const s32 *actorConfigGetGroundReferencePartIds(s32 kind);

/// Get the first configured part used for actor attachment positions.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns First attachment part ID.
/// \note Original decompilation identifier: \c func_8001E614 (0x8001E614).
s32 actorConfigGetFirstAttachmentPartId(s32 kind);

/// Get the second configured part used for actor attachment positions.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Second attachment part ID.
/// \note Original decompilation identifier: \c func_8001E638 (0x8001E638).
s32 actorConfigGetSecondAttachmentPartId(s32 kind);

/// Get the configured default part for an actor kind.
/// \param kind Valid actor kind whose configuration is queried.
/// \returns Default part ID.
/// \note Original decompilation identifier: \c func_8001E65C (0x8001E65C).
s32 actorConfigGetDefaultPartId(s32 kind);

#endif
