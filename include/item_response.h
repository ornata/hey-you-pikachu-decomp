/// \file
/// Declares item interaction-response lookup and handle-preserving transformations.

#ifndef ITEM_RESPONSE_H
#define ITEM_RESPONSE_H

#include <ultra64.h>

/// Recovered voice-request categories selecting an item interaction response.
/// Categories one and two are supported numeric selectors with unresolved meanings.
/// A category selects a response code; it does not itself perform the interaction.
/// These selectors are distinct from transformation causes and collection groups.
typedef enum ItemInteractionResponseCategory {
    ITEM_RESPONSE_TASTE = 3,      ///< Taste or eat request.
    ITEM_RESPONSE_SMELL = 4,      ///< Smell request.
    ITEM_RESPONSE_PLAY_OR_USE = 5 ///< Play, use, or sound request.
} ItemInteractionResponseCategory;

/// Look up an item kind's configured response to an interaction category.
/// \param responseType Category from one through five; see ItemInteractionResponseCategory.
/// \param kind Item kind identifier from zero through 165 for supported categories; ignored otherwise.
/// \returns The configured unsigned reaction code, or zero for an unsupported category.
/// \note Original decompilation identifier: \c func_801146A4 (0x801146A4).
u8 itemGetKindResponse(s32 responseType, s32 kind);

/// Look up the interaction response of an item that permits responses.
/// \param responseType Category from one through five; see ItemInteractionResponseCategory.
/// \param objectId Item handle, or a missing handle.
/// \pre An available handle must refer to initialized item state.
/// \returns The configured response code, or zero if the item is missing, cannot respond, or the category is
/// unsupported.
/// \note Original decompilation identifier: \c func_80114788 (0x80114788).
s32 itemGetResponse(s32 responseType, s32 objectId);

/// Apply a transformation cause to an item, retaining, replacing, or removing it as appropriate.
/// \param id Initialized item handle, or a missing handle to return zero.
/// \param mode Supported transformation-cause selector from zero through five.
/// \returns The surviving item handle, which may identify a replacement; zero if the item is missing or
/// removed.
///
/// \note Requires initialized item, effect, and area resources. A replacement invalidates the original item
/// handle; use the returned handle. Held-item replacement requires the recorded actor holder to remain live.
/// \note Original decompilation identifier: \c func_801147E4 (0x801147E4).
s32 itemTransform(s32 id, s32 mode);

#endif
