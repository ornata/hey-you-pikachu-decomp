/// \file
/// Declares item interaction-response lookup and handle-preserving transformations.

#ifndef ITEM_RESPONSE_H
#define ITEM_RESPONSE_H

#include <ultra64.h>

/// Look up an item kind's configured response to an interaction category.
/// \param responseType Interaction response category from one through five.
/// \param kind Item kind identifier from zero through 165.
/// \returns The configured response code, or zero for an unsupported category.
/// \note Original decompilation identifier: \c func_801146A4 (0x801146A4).
u8 itemGetKindResponse(s32 responseType, s32 kind);

/// Look up the interaction response of an item that permits responses.
/// \param responseType Interaction response category from one through five.
/// \param objectId Item object handle.
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
