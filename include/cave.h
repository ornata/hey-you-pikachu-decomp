/// \file
/// Declares cave interaction creation, landmark association and scene-anchor queries.
#ifndef CAVE_H
#define CAVE_H
#include <ultra64.h>

/// Create and register a cave interaction object.
/// \param position Three world-space position components.
/// \param anchorA First three-component scene anchor, copied into owned state.
/// \param anchorB Second three-component scene anchor, copied into owned state.
/// \param kind Description selector; zero selects the supported description.
/// \returns Registered object managed by the object system.
u8 *caveCreate(f32 *position, f32 *anchorA, f32 *anchorB, u8 kind);

/// Associate a cave with a landmark.
/// \param object Initialized cave object.
/// \param landmark Borrowed live landmark; must remain valid while queried through the cave.
/// \returns Nothing.
void caveSetLinkedLandmark(u8 *object, u8 *landmark);

/// Format the cave description when its description selector is zero.
/// \param object Initialized cave object.
/// \returns Shared borrowed text, unchanged for other selectors and overwritten by later requests.
s8 *caveGetDescription(u8 *object);

/// Query a coordinate of the first scene anchor.
/// \param handle Cave handle; other kinds are unsupported.
/// \param axis Nonnegative axis: zero X, one Y, two Z.
/// \returns Zero for a nonlive handle, the coordinate for axis below three, or -65536 otherwise.
f32 caveGetFirstAnchorCoordinate(s32 handle, s32 axis);

/// Query a coordinate of the second scene anchor.
/// \param handle Cave handle; other kinds are unsupported.
/// \param axis Nonnegative axis: zero X, one Y, two Z.
/// \returns Zero for a nonlive handle, the coordinate for axis below three, or -65536 otherwise.
f32 caveGetSecondAnchorCoordinate(s32 handle, s32 axis);

/// Query a cave's associated landmark handle.
/// \param handle Cave handle with a live associated landmark; other kinds are unsupported.
/// \returns Zero when the cave handle is not live, otherwise its landmark handle.
s32 caveGetLinkedLandmarkHandle(s32 handle);
#endif
