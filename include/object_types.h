/// \file
/// Defines the shared transform and ownership prefix of runtime scene objects.

#ifndef OBJECT_TYPES_H
#define OBJECT_TYPES_H

#include <ultra64.h>

struct RuntimeObjectPrefix;
struct ActorScriptState;
struct ItemState;

/// Object link with its original 32-bit address representation.
/// The address view preserves callers that carry object handles as integers.
typedef union ObjectReference {
    struct RuntimeObjectPrefix *object;
    u8 *bytes;
    void *pointer;
    s32 address;
} ObjectReference;

/// Kind-specific state with the access views used by existing object callbacks.
/// The object's kind determines the state layout and lifetime.
typedef union ObjectPrivateState {
    u8 *bytes;
    void *pointer;
    s32 address;
    s32 *words;
    f32 *floats;
    Gfx **displayLists;
    struct ActorScriptState *actor;
    struct ItemState *item; ///< Complete item-owned state for runtime kind 0x86.
} ObjectPrivateState;

typedef char ObjectPrivateStateSizeCheck[(sizeof(ObjectPrivateState) == 4) ? 1 : -1];

/// Recovered object prefix shared by hierarchy, actor, and item code.
/// This view ends at the private-state pointer. It is not the full allocation
/// and must not be used to allocate or copy a runtime object.
typedef struct RuntimeObjectPrefix {
    f32 scale[3];             ///< Local scale along each axis.
    f32 rotation[3];          ///< Local Euler rotation in degrees.
    f32 position[3];          ///< Local position relative to the hierarchy.
    f32 radius;               ///< Visibility or collision radius; texture-motion kinds initially reuse this value for encoded motion settings.
    ObjectReference sibling;  ///< Next sibling in the hierarchy.
    ObjectReference child;    ///< First child in the hierarchy.
    Gfx *displayList;         ///< Borrowed model display list.
    u16 kind;                 ///< Runtime object kind selecting lifecycle callbacks.
    u16 flags;                ///< Object and hierarchy flags.
    s32 handle;               ///< Object-manager lookup handle.
    u8 *matrixBuffers[2];     ///< Root slot zero owns both buffers; other slots borrow interior transform storage.
    f32 worldPosition[3];     ///< Position produced by hierarchy traversal.
    ObjectReference root;     ///< Root owning this object's hierarchy.
    ObjectPrivateState state; ///< Kind-specific state managed by lifecycle callbacks.
} RuntimeObjectPrefix;

typedef char RuntimeObjectPrefixSizeCheck[(sizeof(RuntimeObjectPrefix) == 0x58) ? 1 : -1];
#define OBJECT_OFFSET(field) ((u32) & (((RuntimeObjectPrefix *)0)->field))
typedef char ObjectSiblingOffsetCheck[(OBJECT_OFFSET(sibling) == 0x28) ? 1 : -1];
typedef char ObjectKindOffsetCheck[(OBJECT_OFFSET(kind) == 0x34) ? 1 : -1];
typedef char ObjectWorldPositionOffsetCheck[(OBJECT_OFFSET(worldPosition) == 0x44) ? 1 : -1];
typedef char ObjectPrivateStateOffsetCheck[(OBJECT_OFFSET(state) == 0x54) ? 1 : -1];
#undef OBJECT_OFFSET

#endif
