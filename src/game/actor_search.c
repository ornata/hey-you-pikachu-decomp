#include "object_target_search.h"
#include "actor_kind.h"
#include "common.h"

/// Find the nearest actor in a kind range, honoring the script actor-kind aliases.
/// \param self Object excluded from the search, or null to exclude none.
/// \param origin World-space search origin.
/// \param distance Exclusive maximum target distance.
/// \param rotation Three-component facing rotation in degrees; unused for an unrestricted cone.
/// \param cone Maximum angular deviation in degrees, or 360 for all directions.
/// \param minKind Inclusive minimum actor kind; ACTOR_SEARCH_RANGE_ENDPOINT selects Bulbasaur.
/// \param maxKind Inclusive maximum actor kind; ACTOR_SEARCH_RANGE_ENDPOINT selects companion Pikachu.
/// \returns The nearest qualifying actor, or null when none is found.
/// \note Original decompilation identifier: \c func_80027380 (0x80027380).
u8 *actorFindNearestInRange(u8 *self, Vec3f *origin, f32 distance, f32 *rotation, f32 cone, s32 minKind, s32 maxKind) {
    if (minKind == ACTOR_SEARCH_RANGE_ENDPOINT) {
        minKind = ACTOR_KIND_BULBASAUR;
    }
    if (maxKind == ACTOR_SEARCH_RANGE_ENDPOINT) {
        maxKind = ACTOR_KIND_PIKACHU;
    }
    return objectFindNearestTargetInView(self, origin, distance, rotation, cone, minKind, maxKind);
}

/// Search for nearby actors and retain the result in the shared target-search state.
/// \param self Address of the actor excluded from the search.
/// \param origin Address of the three-component world-space search origin.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800273DC (0x800273DC).
void actorSearchNearby(s32 self, s32 origin) {
    f32 sp2C[3];

    sp2C[0] = 0.0F;
    sp2C[1] = 0.0F;
    sp2C[2] = 0.0F;
    objectFindNearestTargetInView((u8 *)self, (Vec3f *)origin, 50.0F, sp2C, 360.0F, 1, 0x25);
}
