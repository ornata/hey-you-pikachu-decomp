/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "keyed_pool_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

void runtimeRequestGameModeChange(s32);
void playerSetControllerPort(s32);
/// Double-buffered text images for the shared window page rows.
/// \note Original identifier: \c D_800F1080 (0x800F1080).
extern s32 gTextWindowPageImages[6][2];

u64 runtimeGetElapsedMilliseconds(); /* extern */
/// Current shared text-window transition state; four is inactive.
/// \note Original identifier: \c D_800F1AC0 (0x800F1AC0).
extern s32 gTextWindowState;
/// Elapsed-time checkpoint recorded when the text-window state changes.
/// \note Original identifier: \c D_800F1AC4 (0x800F1AC4).
extern u32 gTextWindowStateStartMilliseconds;

void textImageCreateFromText(s32 *, s32, s32);
/// Owned text image used for the shared window input prompt.
/// \note Original identifier: \c D_800F10A8 (0x800F10A8).
extern s32 gTextWindowPromptImage;

s32 textWindowPushPage(s32);            /* extern */
/// Text-image style used when preparing window pages.
/// \note Original identifier: \c D_800F1A78 (0x800F1A78).
extern s32 gTextWindowTextStyle;
/// Selected buffer of window page text images.
/// \note Original identifier: \c D_800F1ABC (0x800F1ABC).
extern s32 gTextWindowPageImageBuffer;

void textWindowResetText(); /* extern */
/// Scene-view mode controlling overlays and scene updates while the window is active.
/// \note Original identifier: \c D_800F1A74 (0x800F1A74).
extern s32 gTextWindowSceneViewMode;
/// Animated window indicator identifier; zero hides the indicator.
/// \note Original identifier: \c D_800F1A7C (0x800F1A7C).
extern s32 gTextWindowIndicator;

/// Shared accumulated message text from which window pages are prepared.
/// \note Original identifier: \c D_800F10B0 (0x800F10B0).
extern u8 gTextWindowPendingText[];
/// Address of text remaining after the displayed page, or zero at the message end.
/// \note Original identifier: \c D_800F1A80 (0x800F1A80).
extern s32 gTextWindowContinuation;

typedef struct {
    u8 b[4];
} Bytes4;
s32 objectResponsesGetFreeCapacity(void);
void objectResponseQueueImpulse(u8 *, u8 *, Bytes4);
void vector3PackNormalBytes(f32 *, u8 *);
u32 randomNext(void);

/// Request a collision impulse for a target object.
/// \param source Object responsible for the collision.
/// \param target Object receiving the impulse request.
/// \param direction Displacement from source to target; may be modified, and must remain usable as a direction when distance is near zero.
/// \param distance Nonnegative distance between the objects.
/// \returns 0 when impulse-record capacity is insufficient, or 1 otherwise, including when no impulse is queued.
/// \note Original decompilation identifier: \c func_8002C8B0 (0x8002C8B0).
s32 collisionRequestImpulse(u8 *source, u8 *target, f32 *direction, f32 distance) {
    Bytes4 impulse;
    f32 randomDirection[3];
    f32 strength;
    f32 maximum = 10.0f;

    if (objectResponsesGetFreeCapacity() < 4) {
        return 0;
    }
    /* Original behavior: the fallback is stored locally, but direction remains the input pointer. */
    if (distance < 0.001f) {
        randomDirection[0] = (randomNext() * 2.3283064365386963e-10 - 0.5) * 1.2f;
        randomDirection[1] = 0.5f;
        randomDirection[2] = (randomNext() * 2.3283064365386963e-10 - 0.5) * 1.2f;
        strength = maximum;
    } else {
        strength = 1.0f / distance;
        direction[0] = direction[0] * strength + (randomNext() * 2.3283064365386963e-10 - 0.5) * 0.001f;
        direction[1] = direction[1] * strength + (randomNext() * 2.3283064365386963e-10 - 0.5) * 0.001f;
        direction[2] = direction[2] * strength + (randomNext() * 2.3283064365386963e-10 - 0.5) * 0.001f;
        strength *= maximum;
    }
    if (strength < 0.001f) {
        return 1;
    } else {
        vector3PackNormalBytes(direction, impulse.b);
        if (strength < maximum) {
            impulse.b[3] = (s8)(s32)strength;
        } else {
            impulse.b[3] = (s8)(s32)maximum;
        }
        objectResponseQueueImpulse(source, target, impulse);
    }
    return 1;
}

/// Provide the inactive collision hook for this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CB98 (0x8002CB98).
void collisionInactiveHook(void) {
}

/// Provide the inactive collision-grid diagnostic entry point for this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CBA0 (0x8002CBA0).
void collisionGridDebugReportNoOp(void) {
    TRACE("-------- collisionTbl Info --------\n");
}

/// Owned keyed pool grouping registered objects for collision candidate queries.
/// \note Original identifier: \c D_800F1B20 (0x800F1B20).
extern KeyedPool gCollisionCandidateRegistry;
/// X-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B40 (0x800F1B40).
extern f32 gCollisionGridAlignmentX;
/// Z-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B44 (0x800F1B44).
extern f32 gCollisionGridAlignmentZ;
s32 keyedPoolInsert(KeyedPool *, s32, u32);

/// Register an eligible object for collision candidate queries.
/// \param object Object with collision size, flags, and position to register.
/// \returns A nonzero registration key, or zero when ineligible or registration fails.
/// \note Original decompilation identifier: \c func_8002CBA8 (0x8002CBA8).
s32 collisionGridRegisterObject(u8 *object) {
    s32 key;
    s32 h;

    if ((*(u16 *)(object + 0x36) & 0xF0) || *(f32 *)(object + 0x24) < 0.001f) {
        return 0;
    }
    h = (s32)((*(f32 *)(object + 0x44) + gCollisionGridAlignmentX) * 0.125f) * 0xAD +
        (s32)((*(f32 *)(object + 0x4C) + gCollisionGridAlignmentZ) * 0.125f) * 0x8B + 0x1E240;
    if (h == 0) {
        key = 1;
    } else {
        key = h;
    }
    if (keyedPoolInsert(&gCollisionCandidateRegistry, (s32)object, key) != 0) {
        return key;
    }
    return 0;
}

/// Compatibility declaration retaining the unused registration-key pointer argument.
s32 keyedPoolRemoveKeyAndValue(KeyedPool *, s32, s32, s32 *); /* extern */

/// Remove an object collision registration and clear its registration key.
/// \param object Address of the registered object.
/// \param registrationKey Stored registration key, cleared by this operation; zero means no registration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CCB4 (0x8002CCB4).
void collisionGridUnregisterObject(s32 object, s32 *registrationKey) {
    s32 registeredKey;

    registeredKey = *registrationKey;
    if (registeredKey != 0) {
        keyedPoolRemoveKeyAndValue(&gCollisionCandidateRegistry, object, registeredKey, registrationKey);
        *registrationKey = 0;
    }
}

/// X-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B40 (0x800F1B40).
/// Z-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B44 (0x800F1B44).
extern f32 gCollisionGridAlignmentX, gCollisionGridAlignmentZ;
s32 collisionRequestImpulse(u8 *, u8 *, f32 *, f32);
f32 vector3Length(f32 *);
s32 keyedPoolIterateFirstForKey(void *, s32);
s32 keyedPoolIterateNextForKey(void *, s32);

/* Advance a spatial-hash bucket iterator to the next entry with the same key. */
#define HASH_NEXT(var, table, key)                                                                                     \
    do {                                                                                                               \
        var = (u8 *)keyedPoolIterateNextForKey(table, key);                                                            \
    } while (0)

/// Find an overlapping collision candidate and request applicable object responses.
/// \param object Object whose collision size and position are queried.
/// \returns The last overlapping candidate visited in the current collision
/// group, or NULL when that group contains no eligible overlap.
/// \note Original decompilation identifier: \c func_8002CCF8 (0x8002CCF8).
u8 *collisionGridFindOverlap(u8 *object) {
    f32 distance;
    s32 hash;
    s32 key;
    f32 delta[3];
    u8 *candidate;
    u8 *hit;

    if (*(f32 *)(object + 0x24) < 0.001f) {
        return NULL;
    }
    hash = (s32)((*(f32 *)(object + 0x44) + gCollisionGridAlignmentX) * 0.125f) * 0xAD +
           (s32)((*(f32 *)(object + 0x4C) + gCollisionGridAlignmentZ) * 0.125f) * 0x8B + 0x1E240;
    if (hash == 0) {
        key = 1;
    } else {
        key = hash;
    }
    hit = NULL;
    candidate = (u8 *)keyedPoolIterateFirstForKey(&gCollisionCandidateRegistry, key);
    if (candidate != NULL) {
        do {
            if (candidate != object && !(*(u16 *)(candidate + 0x36) & 0xC0)) {
                delta[0] = *(f32 *)(candidate + 0x44) - *(f32 *)(object + 0x44);
                delta[1] = *(f32 *)(candidate + 0x48) - *(f32 *)(object + 0x48);
                delta[2] = *(f32 *)(candidate + 0x4C) - *(f32 *)(object + 0x4C);
                distance = vector3Length(delta);
                if (!(*(f32 *)(candidate + 0x24) + *(f32 *)(object + 0x24) < distance)) {
                    hit = candidate;
                    switch (*(u16 *)(candidate + 0x34)) {
                    case 0x86:
                        if (collisionRequestImpulse(object, candidate, delta, distance) == 0)
                            goto done;
                        break;
                    default:
                        break;
                    }
                }
            }
            HASH_NEXT(candidate, &gCollisionCandidateRegistry, key);
        } while (candidate != NULL);
    }
done:
    return hit;
}


/// Randomize the alignment used to group collision candidates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CED0 (0x8002CED0).
void collisionGridRandomizeAlignment(void) {
    gCollisionGridAlignmentX = (0.5 - randomNext() * 2.3283064365386963e-10) * 8.0;
    gCollisionGridAlignmentZ = (0.5 - randomNext() * 2.3283064365386963e-10) * 8.0;
}

void keyedPoolInitialize(KeyedPool *, u32); /* extern */

/// Initialize the collision candidate registry.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CFA8 (0x8002CFA8).
void collisionGridInitialize(void) {
    keyedPoolInitialize(&gCollisionCandidateRegistry, 0x1F3);
    gCollisionGridAlignmentX = 0.0f;
    gCollisionGridAlignmentZ = 0.0f;
}

void keyedPoolDestroy(KeyedPool *); /* extern */

/// Release the collision candidate registry.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CFE0 (0x8002CFE0).
void collisionGridDestroy(void) {
    keyedPoolDestroy(&gCollisionCandidateRegistry);
}
