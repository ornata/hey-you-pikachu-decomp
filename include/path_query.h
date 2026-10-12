/// \file
/// Declares world-coordinate queries over live path objects and their waypoints.
#ifndef PATH_QUERY_H
#define PATH_QUERY_H
#include <ultra64.h>

/// Query a coordinate of a path's first point.
/// \param handle Live path-object handle with at least one initialized point.
/// \param axis Nonnegative axis: zero X, one Y, two Z.
/// \returns Zero for a missing handle, the coordinate for a path and axis below three, or -65536 otherwise.
f32 pathGetFirstPointCoordinateByHandle(s32 handle, s32 axis);

/// Query the destination after a waypoint near an actor.
/// \param object Live actor whose world position selects a nearby waypoint.
/// \param handle Path handle with at least one waypoint and a complete three-coordinate end marker with negative X.
/// \param axis Nonnegative axis: zero X, one Y, two Z.
/// \returns Coordinate after the first waypoint within three ground-distance units, or an end-marker coordinate
/// when none qualifies; zero for a missing handle, or -65536 for a non-path object or an axis above two.
f32 actorGetFollowingPathPointCoordinate(u8 *object, s32 handle, s32 axis);
#endif
