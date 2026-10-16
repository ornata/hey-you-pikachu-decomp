/// \file
/// Defines the shared transform and ownership prefix of runtime scene objects.

#ifndef OBJECT_TYPES_H
#define OBJECT_TYPES_H

#include <ultra64.h>

struct RuntimeObjectPrefix;
struct ActorScriptState;
struct ItemState;

/// Named runtime object tags established by their constructors and consumers.
/// Other encodings remain numeric. These tags are distinct from item-catalog IDs;
/// the runtime object's kind field retains its original unsigned 16-bit storage.
typedef enum RuntimeObjectKind {
    RUNTIME_OBJECT_KIND_PLAYER_CONTROL_ROOT = 0x7F,    ///< Root of the player-control collision hierarchy.
    RUNTIME_OBJECT_KIND_LANDMARK_CANNON = 0x8F,        ///< Interactable cannon landmark.
    RUNTIME_OBJECT_KIND_VOICE_REACTIVE_SPINNER = 0x82, ///< Voice-responsive rotation with owned speed state.
    RUNTIME_OBJECT_KIND_ITEM = 0x86,                   ///< Registered item with item-owned private state.
    RUNTIME_OBJECT_KIND_PASSAGE = 0x87,                ///< Door, sign, or gate with owned passage state.
    RUNTIME_OBJECT_KIND_INTERACTION_PROXY = 0x8A,      ///< Descriptive interaction target with caller-defined context.
    RUNTIME_OBJECT_KIND_SCENE_REGION = 0xAF,           ///< Region with owned occupancy state and borrowed waypoints.
    RUNTIME_OBJECT_KIND_CAVE = 0xB0                    ///< Cave with owned anchors and a borrowed linked landmark.
} RuntimeObjectKind;

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
    struct ItemState *item; ///< Complete item-owned state for RUNTIME_OBJECT_KIND_ITEM.
} ObjectPrivateState;

typedef char ObjectPrivateStateSizeCheck[(sizeof(ObjectPrivateState) == 4) ? 1 : -1];

/// Recovered object prefix shared by hierarchy, actor, and item code.
/// This view ends at the private-state pointer. It is not the full allocation
/// and must not be used to allocate or copy a runtime object.
typedef struct RuntimeObjectPrefix {
    f32 scale[3];    ///< Local scale along each axis.
    f32 rotation[3]; ///< Local Euler rotation in degrees.
    f32 position[3]; ///< Local position relative to the hierarchy.
    f32 radius; ///< Visibility or collision radius; texture-motion kinds initially reuse this value for encoded motion settings.
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
typedef char ObjectRotationOffsetCheck[(OBJECT_OFFSET(rotation) == 0xC) ? 1 : -1];
typedef char ObjectPositionOffsetCheck[(OBJECT_OFFSET(position) == 0x18) ? 1 : -1];
typedef char ObjectRadiusOffsetCheck[(OBJECT_OFFSET(radius) == 0x24) ? 1 : -1];
#undef OBJECT_OFFSET

#endif
