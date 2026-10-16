/// \file
/// Declares handle-based object categories, kinds and creature action status exposed to actor scripts.

#ifndef OBJECT_SCRIPT_QUERY_H
#define OBJECT_SCRIPT_QUERY_H

#include "script_action_flags.h"
#include <ultra64.h>
#include "actor_kind.h"

/// Stable category values returned to actor scripts.
/// Numeric object groups retain their type numbers until their semantic roles are established.
typedef enum ObjectScriptCategory {
    SCRIPT_CATEGORY_UNAVAILABLE = 0,
    SCRIPT_CATEGORY_ITEM = 1,
    SCRIPT_CATEGORY_PLAYER_CONTROL = 2,
    SCRIPT_CATEGORY_TYPES_141_TO_148 = 3,
    SCRIPT_CATEGORY_ACTOR = 4,                 ///< Actor configurations, including actor props.
    SCRIPT_CATEGORY_VOICE_REACTIVE_SPINNER = 5 ///< Voice-reactive rotation objects.
} ObjectScriptCategory;

/// Classify an available object for actor scripts.
/// \param actor Unused actor context accepted by the script interface.
/// \param handle Handle of the object to classify.
/// \returns One for items, two for player-control roots, three for types 141 through 148, four for ActorKind configurations including props, five for voice-reactive spinners, or zero for an unavailable or unsupported object.
/// \note Original decompilation identifier: \c func_80117840 (0x80117840).
s32 objectGetScriptCategory(u8 *actor, s32 handle);

/// Get the kind identifier exposed to actor scripts for an available object.
/// \param handle Handle of the object to query.
/// \returns The item or actor kind, the runtime kind for voice-reactive spinners and types 141 through 148, or zero for an unavailable or unsupported object.
/// \note Interpret the result with the object's script category; the returned domains overlap.
/// \note Zero alone does not establish availability or identify the result's domain.
/// \note Original decompilation identifier: \c func_801178EC (0x801178EC).
s32 objectGetScriptKind(s32 handle);

/// Get the item kind associated with an available handle.
/// \param handle Handle of the item to query.
/// \returns The item kind, or zero if the handle is unavailable or does not identify an item.
/// \note Original decompilation identifier: \c func_80117998 (0x80117998).
s16 itemGetKindByHandle(s32 handle);

/// Get the actor configuration kind associated with an available handle.
/// \param handle Handle of the actor to query.
/// \returns The ActorKind, including actor props, or zero for an unavailable handle or another object domain.
/// \note Original decompilation identifier: \c func_801179E8 (0x801179E8).
u16 actorGetKindByHandle(s32 handle);

/// Get the runtime lifecycle kind of an available object.
/// \param handle Handle of the object to query.
/// \returns The unsigned runtime kind, or zero when the handle is unavailable.
/// \note Actor configuration IDs share this domain; item catalog IDs and script categories do not.
/// \note Zero does not distinguish an unavailable handle from a runtime kind of zero.
/// \note Original decompilation identifier: \c func_80117A30 (0x80117A30).
u16 objectGetTypeByHandle(s32 handle);

/// Get the compact action status exposed to actor scripts for a creature.
/// \param handle Handle of the creature whose current action is queried.
/// \returns The script action status flags, or zero when the handle is not live or does not identify a creature.
/// \note Returned action status excludes actor behavior inhibition flags.
/// \note Original decompilation identifier: \c func_80117EE8 (0x80117EE8).
u16 actorGetScriptActionFlagsByHandle(s32 handle);

#endif
