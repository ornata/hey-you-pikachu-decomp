/// \file
/// Declares coordinate queries over scene path tables and live path-object waypoints.
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
/// Query the point count of a path in the loaded scene.
/// \param path Valid index in the initialized current scene's path table.
/// \returns Number of three-coordinate points in that path.
s32 areaGetPathPointCount(s32 path);

/// Query one world coordinate in a loaded scene path.
/// \param path Valid index in the initialized current scene's path table.
/// \param point Nonnegative point index.
/// \param axis Nonnegative axis: zero X, one Y, two Z.
/// \returns Coordinate when point and axis are in range, or -65536 otherwise.
f32 areaGetPathPointCoordinate(s32 path, s32 point, s32 axis);

#endif
