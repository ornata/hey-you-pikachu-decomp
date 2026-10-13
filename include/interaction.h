/// \file
/// Declares shared interaction-candidate selection and marker queries.

#ifndef INTERACTION_H
#define INTERACTION_H

#include <ultra64.h>

/// Independent classification bits returned by interaction candidate queries.
typedef enum InteractionClassificationFlag {
    INTERACTION_SECONDARY_REGION = 1, ///< Candidate qualifies for the secondary region.
    INTERACTION_PRIMARY_REGION = 2,   ///< Candidate qualifies for the primary interaction region.
    INTERACTION_CAMERA_VISIBLE = 4    ///< Candidate passes the camera visibility query.
} InteractionClassificationFlag;

/// Classify a live object and consider it for preferred interaction selection.
/// \param object Borrowed live object with initialized transform and identity.
/// \returns Classification bits: one for the secondary region, two for the primary region, and four for camera visibility; zero means ineligible.
/// \note Updates shared candidate state; candidate scans must not overlap.
/// \note Original decompilation identifier: \c func_8001E830 (0x8001E830).
s32 interactionConsiderObject(u8 *object);

/// Create and register the interaction marker hierarchy.
/// \returns Registered marker object whose lifetime is managed by the object system.
/// \note Original decompilation identifier: \c func_8001EA78 (0x8001EA78).
u8 *interactionMarkerCreate(void);

/// Retrieve the preferred object from the current interaction scan.
/// \returns Candidate object handle, or zero when none is selected.
/// \note Original decompilation identifier: \c func_8001F060 (0x8001F060).
s32 interactionGetCandidateId(void);

/// Retrieve the active interaction marker presentation mode.
/// \returns Current mode, or zero when inactive.
/// \note Original decompilation identifier: \c func_8001F06C (0x8001F06C).
s32 interactionGetMarkerMode(void);

/// Select a player's interaction target/action and its marker presentation.
/// \param player Initialized player record receiving selection state.
/// \param object Live candidate with valid kind-specific state, or null to clear selection.
/// \param flags InteractionClassificationFlag mask from the candidate query.
/// \returns Marker style identifier, or zero to hide the marker.
/// \note Original identifier: \c func_80111784 (0x80111784).
s32 playerClassifyInteractionTarget(u8 *player, u8 *object, s32 flags);

/// Set the primary interaction region's horizontal and vertical scale.
/// \param scale Both axis scales in tenths.
/// \returns Nothing.
/// \note Original identifier: \c func_8001F078 (0x8001F078).
void interactionSetPrimaryRegionScale(s32 scale);

/// Set the primary interaction region's margin.
/// \param margin Margin in tenths of a coordinate unit.
/// \returns Nothing.
/// \note Original identifier: \c func_8001F0A0 (0x8001F0A0).
void interactionSetPrimaryRegionMargin(s32 margin);

/// Set the secondary interaction region's horizontal and vertical scale.
/// \param scale Both axis scales in tenths.
/// \returns Nothing.
/// \note Original identifier: \c func_8001F0C0 (0x8001F0C0).
void interactionSetSecondaryRegionScale(s32 scale);

/// Set the secondary interaction region's margin.
/// \param margin Margin in tenths of a coordinate unit.
/// \returns Nothing.
/// \note Original identifier: \c func_8001F0E8 (0x8001F0E8).
void interactionSetSecondaryRegionMargin(s32 margin);

#endif
