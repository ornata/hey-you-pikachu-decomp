/// \file
/// Declares shared interaction-candidate selection and marker queries.

#ifndef INTERACTION_H
#define INTERACTION_H

#include <ultra64.h>

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

#endif
