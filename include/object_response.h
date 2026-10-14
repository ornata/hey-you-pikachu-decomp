/// \file
/// Defines queued object responses and the collision impulse submission interface.
#ifndef OBJECT_RESPONSE_H
#define OBJECT_RESPONSE_H
#include <ultra64.h>

/// Packed collision direction followed by a signed impulse strength.
typedef struct ObjectImpulse {
    u8 bytes[4]; ///< Three signed normal components, then signed strength; byte storage preserves the value ABI.
} ObjectImpulse;

/// Prepared response owned by the response system after attachment.
typedef struct ObjectResponse {
    s32 sourceHandle; ///< Source object handle, or zero when absent.
    u16 flags;        ///< Bit zero marks an owned pointer payload requiring release.
    u8 kind;          ///< Response handler index; zero selects a packed impulse.
    u8 unknown07;     ///< Uninitialized byte; no interpretation established.
    union {
        ObjectImpulse impulse; ///< Inline payload for response kind zero.
        void *owned;           ///< Owned payload when flags bit zero is set.
    } payload;
} ObjectResponse;

/// Handle one borrowed response before the response system releases it.
/// \param object Initialized target object.
/// \param response Borrowed response; neither it nor its payload may be retained.
/// \returns Nothing.
typedef void (*ObjectResponseHandler)(u8 *object, ObjectResponse *response);

typedef char ObjectImpulseSizeCheck[(sizeof(ObjectImpulse) == 4) ? 1 : -1];
typedef char ObjectResponseSizeCheck[(sizeof(ObjectResponse) == 0xC) ? 1 : -1];
typedef char ObjectResponseSourceOffsetCheck[(((u32) & ((ObjectResponse *)0)->sourceHandle) == 0) ? 1 : -1];
typedef char ObjectResponseFlagsOffsetCheck[(((u32) & ((ObjectResponse *)0)->flags) == 4) ? 1 : -1];
typedef char ObjectResponseKindOffsetCheck[(((u32) & ((ObjectResponse *)0)->kind) == 6) ? 1 : -1];
typedef char ObjectResponsePayloadOffsetCheck[(((u32) & ((ObjectResponse *)0)->payload) == 8) ? 1 : -1];

/// Begin an empty response-system lifetime before objects submit responses.
/// \param capacity Positive maximum number of registered response records.
/// \param heapBytes Bytes available for response records and owned payloads.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C460 (0x8001C460).
void objectResponsesInitialize(s32 capacity, s32 heapBytes);

/// End the response-system lifetime after all target objects are destroyed.
/// Outstanding response records and owned payloads become invalid.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C4CC (0x8001C4CC).
void objectResponsesDestroy(void);

/// Get unused response registry capacity.
/// \returns Available entries; payload heap capacity is a separate constraint.
s32 objectResponsesGetFreeCapacity(void);

/// Queue a packed impulse when the target and storage permit it.
/// \param source Source object, or null when absent.
/// \param target Initialized target item with a valid hierarchy root.
/// \param impulse Packed direction and signed strength, copied by value.
/// \returns Nothing.
void objectResponseQueueImpulse(u8 *source, u8 *target, ObjectImpulse impulse);
/// Dispatch queued responses through the target's kind hook and release them.
/// \param object Live initialized object; released responses and payloads become invalid.
/// \returns Nothing.
void objectResponsesDispatchAndClear(u8 *object);

/// Release queued responses without dispatching them.
/// \param object Live object whose pending responses and owned payloads are released.
/// \returns Nothing.
void objectResponsesDiscard(u8 *object);

#endif
