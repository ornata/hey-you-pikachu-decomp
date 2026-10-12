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
    u8 unknown07;     ///< Uninitialized reserved byte; no interpretation established.
    union {
        ObjectImpulse impulse; ///< Inline payload for response kind zero.
        void *owned;           ///< Owned payload when flags bit zero is set.
    } payload;
} ObjectResponse;

typedef char ObjectImpulseSizeCheck[(sizeof(ObjectImpulse) == 4) ? 1 : -1];
typedef char ObjectResponseSizeCheck[(sizeof(ObjectResponse) == 0xC) ? 1 : -1];
typedef char ObjectResponsePayloadOffsetCheck[(((u32) & ((ObjectResponse *)0)->payload) == 8) ? 1 : -1];

/// Get unused response registry capacity.
/// \returns Available entries; payload heap capacity is a separate constraint.
s32 objectResponsesGetFreeCapacity(void);

/// Queue a packed impulse when the target and storage permit it.
/// \param source Source object, or null when absent.
/// \param target Initialized target item with a valid hierarchy root.
/// \param impulse Packed direction and signed strength, copied by value.
/// \returns Nothing.
void objectResponseQueueImpulse(u8 *source, u8 *target, ObjectImpulse impulse);
#endif
