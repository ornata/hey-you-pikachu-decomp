/// \file
/// Defines motion playback mode values shared by actors and animated model parts.

#ifndef MOTION_PLAYBACK_TYPES_H
#define MOTION_PLAYBACK_TYPES_H

#include "animation_tracks.h"
#include "matrix_types.h"

/// Motion evaluation policy stored in a target 16-bit playback field.
/// The enum supplies names; it does not change the width of stored mode fields.
typedef enum MotionPlaybackMode {
    MOTION_PLAYBACK_DISABLED = 0,  ///< Motion evaluation is inactive.
    MOTION_PLAYBACK_TRACKS = 1,    ///< Evaluate motion archive tracks.
    MOTION_PLAYBACK_TIMED = 2,     ///< Advance a timed motion without track evaluation.
    MOTION_PLAYBACK_POSE_BLEND = 3 ///< Blend captured source and destination poses.
} MotionPlaybackMode;

/// Mode-dependent child-part orientation storage.
typedef union ChildMotionWorkspace {
    AnimationTrack3 rotationTrack; ///< Rotation channels borrowing motion archive keys.
    Matrix34 posePair[2];          ///< Captured source and destination orientations for pose blending.
} ChildMotionWorkspace;

struct TargetFacingState;
struct CameraFacingState;

/// One optional controller allocation, interpreted by the installed child callback.
/// The target and camera records have distinct layouts and must not be interchanged.
/// The child owns the selected allocation until its motion state is released.
typedef union ChildFacingController {
    void *storage;                    ///< Owned allocation, or null when no facing controller is installed.
    struct TargetFacingState *target; ///< Target-facing history and blend state for kind 0x2B.
    struct CameraFacingState *camera; ///< Camera-facing orientation and blend state for kind 0x43.
} ChildFacingController;

typedef char ChildFacingControllerSizeCheck[(sizeof(ChildFacingController) == 4) ? 1 : -1];

/// Allocated child-part motion state; the mode selects the active workspace view.
/// The tail also supports optional part controllers whose layouts remain separate.
typedef struct ChildMotionState {
    s16 playbackMode;              ///< MotionPlaybackMode value; disabled after track completion.
    u8 unknown002[2];              ///< Unresolved bytes preceding the aligned workspace.
    ChildMotionWorkspace playback; ///< Track or pose data selected by playbackMode.
    u8 unknown064[0x18];           ///< Unresolved storage before the optional controller.
    ChildFacingController
        facingController; ///< Owned optional target- or camera-facing state, selected by the child update callback.
} ChildMotionState;

typedef char ChildMotionWorkspaceSizeCheck[(sizeof(ChildMotionWorkspace) == 0x60) ? 1 : -1];
typedef char ChildMotionStateSizeCheck[(sizeof(ChildMotionState) == 0x80) ? 1 : -1];
typedef char ChildMotionWorkspaceOffsetCheck[((u32) & ((ChildMotionState *)0)->playback == 4) ? 1 : -1];

typedef char
    ChildMotionFacingControllerOffsetCheck[((u32) & ((ChildMotionState *)0)->facingController == 0x7C) ? 1 : -1];

#endif
