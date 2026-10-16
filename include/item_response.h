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

/// Recovered plan classes returned by item interaction-response lookup.
/// These codes are distinct from response categories, item kinds and action IDs.
/// Names describe the reviewed shared script plan, not guaranteed execution or
/// outcomes. Deferred actions, conditional profile feedback, meter clamping and
/// special item routes still apply. Catalog storage remains unsigned bytes and
/// public interfaces retain their existing integer representations.
typedef enum ItemReactionCode {
    ITEM_REACTION_EAT_CUE_2 = 2,      ///< Eating cue 2 plan with a requested +10 secondary-meter change.
    ITEM_REACTION_EAT_CUE_0 = 3,      ///< Eating cue 0 plan with profile feedback.
    ITEM_REACTION_EAT_AND_SHIVER = 4, ///< Eating and shivering plan with a requested -5 secondary-meter change.
    ITEM_REACTION_EAT_CUE_2_ALTERNATE_CODE = 5, ///< Alternate code for the same ordinary plan as EAT_CUE_2.
    ITEM_REACTION_EAT_AND_COUGH = 6, ///< Eating and coughing plan with a requested -5 secondary-meter change.
    ITEM_REACTION_SMELL_THROW_WITH_OPTIONAL_THUNDERBOLT =
        8,                              ///< Smell and throw, with a conditional Thunderbolt request.
    ITEM_REACTION_SMELL_AND_COUGH = 10, ///< Smelling and coughing plan.
    ITEM_REACTION_SMELL_SNEEZE_WITH_OPTIONAL_THUNDERBOLT =
        11,                                     ///< Smell and sneeze, with a conditional Thunderbolt request.
    ITEM_REACTION_SMELL_RELEASE_AND_SLEEP = 12, ///< Smelling, held-item release and sleep plan.
    ITEM_REACTION_THROW_ITEM = 15,              ///< Throwing plan with default cue 2.
    ITEM_REACTION_TILT_GESTURE = 16,            ///< Tilting gesture with profile feedback.
    ITEM_REACTION_THROW_THEN_THUNDERBOLT = 17,  ///< Throw then request Thunderbolt.
    ITEM_REACTION_THROW_THEN_THUNDERBOLT_WITH_TISSUE_OVERRIDE =
        18,                             ///< Ordinary throw/Thunderbolt plan with a tissue-specific wrapper override.
    ITEM_REACTION_PLAY_INSTRUMENT = 19, ///< Select a harmonica, trumpet or grass-whistle gesture by held item kind.
    ITEM_REACTION_THROW_WITH_OPTIONAL_THUNDER = 20 ///< Throwing plan with a conditional Thunder request.
} ItemReactionCode;

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
