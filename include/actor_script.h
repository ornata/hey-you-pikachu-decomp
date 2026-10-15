/// \file
/// Declares actor script state views and the script-message delivery interface.

#ifndef ACTOR_SCRIPT_H
#define ACTOR_SCRIPT_H

#include "script_message.h"
#include "action_script_types.h"
#include "actor_action_cue.h"
#include "actor_action_callbacks.h"
#include "action_definition_types.h"
#include "direction_animation_types.h"
#include "matrix_types.h"
#include "actor_orientation_cache.h"
#include "motion_playback_types.h"
#include "animation_tracks.h"

/// Root playback workspace whose active view follows the motion playback mode.
/// Track mode borrows curve keys; pose-blend mode stores two consecutive transforms.
/// The full cleared extent is known, but bytes beyond these views remain unresolved.
typedef union ActorRootPlaybackWorkspace {
    AnimationTrack3 orientationTrack; ///< Root orientation channels in track mode.
    Matrix34 posePair[2];             ///< Source and destination transforms in pose-blend mode.
    u8 storage[0x78];                 ///< Preserves the full workspace extent cleared at initialization.
} ActorRootPlaybackWorkspace;

typedef char ActorRootPlaybackWorkspaceSizeCheck[(sizeof(ActorRootPlaybackWorkspace) == 0x78) ? 1 : -1];

/// Action-specific movement scratch storage; the active action selects the valid view.
/// Values do not persist as a common physical quantity across action changes.
typedef union ActorMovementWorkspace {
    f32 approachScale;   ///< Multiplier applied to pending movement while finishing an approach.
    f32 groundClearance; ///< Nonnegative height above sampled terrain recorded by the ground-check action.
    f32 pulseAngle;      ///< Vertical scale-pulse phase in degrees.
} ActorMovementWorkspace;

typedef char ActorMovementWorkspaceSizeCheck[(sizeof(ActorMovementWorkspace) == 4) ? 1 : -1];

/// Recovered prefix of actor state used by script-message consumers.
/// \ingroup ActorScriptVm
/// Unresolved storage and scalar roles retain explicit offset-based names. This view does not describe the
/// ownership of its resources and must not be copied to clone an actor or transfer ownership.
/// Presentation snapshots borrow the original live actor's resources and restore selected fields only.
/// The 0x408-byte extent is established by actorInitializeModelState allocation and clearing.
typedef struct ActorScriptState {
    u8 *groundReference;          ///< Borrowed hierarchy part used for ground-reference pose.
    f32 verticalPoseOffset;       ///< Accumulated vertical offset of the animated pose.
    f32 defaultPoseOffset;        ///< Default vertical offset supplied by motion data.
    f32 orientation[3];           ///< Actor orientation in degrees, separate from child poses.
    Matrix34 rootMotionTransform; ///< Root-motion orientation with heading separated into actor yaw.
    f32 rootMotionHeading;        ///< Last extracted root-motion heading in degrees.
    u8 **partTable;               ///< Owned array of partCount borrowed object pointers; released with model state.
    s32 partCount;                ///< Number of slots in the owned part-lookup table.
    Gfx *modelDrawLists[2];       ///< Two drawing lists sharing one owned allocation.
    u8 unknown05C[4];
    DirectionAnimation directionAnimation; ///< Owned paired lighting poses and their angular playback state.
    u8 unknown0DC[4];
    u8 environmentColor[4];            ///< Model environment red, green, blue and alpha components.
    u8 targetEnvironmentColor[4];      ///< Requested model environment RGBA components.
    f32 environmentColorTimeRemaining; ///< Seconds remaining in the environment-color transition.
    s32 actionId;                      ///< Selected action identifier; negative values identify synthetic transitions.
    char *actionName;                  ///< Borrowed display/debug name of the selected action.
    f32 actionGravityScale;            ///< Gravity multiplier supplied by the selected action definition.
    s32 actionAttributes;              ///< Behavior flags supplied by the selected action.
    s16 motionId;                      ///< Selected motion-data identifier.
    u8 previousPoseId;                 ///< Source pose selector used when composing an action transition.
    u8 currentPoseId;       ///< Destination pose selector used for action compatibility and transition lookup.
    s16 continuationAction; ///< Next action identifier; values outside the action table end continuation.
    u8 unknown102[2];
    f32 actionMovementScale; ///< Movement scale supplied by an action, or duration supplied by a pose transition.
    ActorActionInitializer actionInitializer;   ///< Optional initializer borrowed from the selected action definition.
    ActorActionUpdateCallback actionCallback;   ///< Per-frame action callback; one ends playback.
    ActorActionCueCommand *primaryCueProgram;   ///< Borrowed primary cue program or action-kind selection collection.
    ActorActionCueCommand *secondaryCueProgram; ///< Borrowed secondary cue program or action-kind selection collection.
    s16 actionStartPending;                     ///< Nonzero when action playback must start or restart.
    s16 playbackMode;                           ///< MotionPlaybackMode value stored as a signed 16-bit word.
    s16 loopCount;                              ///< Completed loop count for current playback.
    s16 loopExitRequested;                      ///< Nonzero requests transition out of the current loop.
    s16 pendingAction;                          ///< Requested next action identifier, or -1 when absent.
    u8 unknown122[2];
    s32 nextRequestId;      ///< Identifier to assign to the next action request.
    s32 activeRequestId;    ///< Most recently selected action request identifier.
    s32 pendingRequestId;   ///< Identifier assigned to the pending action request.
    f32 movementScale;      ///< Scale applied to motion displacement and gravity.
    f32 motionDuration;     ///< Total duration of current motion or pose blending.
    f32 motionRemaining;    ///< Remaining motion time, scaled by playback speed.
    s16 defaultActionMode;  ///< Default action mode selected by scripts.
    s16 selectedActionKind; ///< Kind of the selected action used to select its cue program.
    s16 pendingActionKind;  ///< Kind supplied by the pending action request.
    u8 unknown142[2];
    f32 primaryCueClock;                          ///< Elapsed time of the retained primary action-cue program.
    f32 secondaryCueClock;                        ///< Elapsed time of the current secondary action-cue program.
    ActorActionCueCommand *primaryCueCommand;     ///< Borrowed pending command in the primary cue program.
    ActorActionCueCommand *lastPrimaryCueProgram; ///< Borrowed primary program retained to detect repeated selection.
    ActorActionCueCommand *secondaryCueCommand;   ///< Borrowed pending command in the secondary cue program.
    ActionScriptState actionScript;               ///< Owned timed sequencer state borrowing its command resource.
    s32 soundHandles[4];                      ///< Four tracked spatial playback handles; zero denotes an empty slot.
    s32 nextSoundSlot;                        ///< Next slot to consider for tracked spatial playback.
    s32 trackedVoice;                         ///< Separately tracked character voice playback handle.
    s32 heldItemHandle;                       ///< Object handle of the actor's held item, or zero.
    s32 attachmentMode;                       ///< Attachment selection used for held items and actor effects.
    s32 targetHandle;                         ///< Object handle used by stored-target resolution.
    s32 targetMode;                           ///< Mode selecting how the stored target is interpreted.
    f32 unknown1A0;                           ///< Randomized approach sample; its later purpose remains unresolved.
    f32 targetDistanceAllowance;              ///< Approach distance allowance for the stored target.
    u64 lastInteractionMilliseconds;          ///< Time origin for the item interaction cooldown.
    s32 searchItemKind;                       ///< Item kind used as both bounds of an exact-kind search.
    f32 searchItemRange;                      ///< Search range for the actor's preferred item kind.
    s32 vmAddress;                            ///< Address of the actor's retained script context in the N64 ABI.
    s32 airborne;                             ///< Nonzero when movement leaves clearance above the ground.
    f32 poseDisplacement[3];                  ///< Local pose displacement accumulated by ground-reference updates.
    f32 localMotion[3];                       ///< Motion vector before transformation into world space.
    f32 worldMotion[3];                       ///< Transformed and scaled motion vector.
    f32 gravityVelocity;                      ///< Additional vertical velocity accumulated under gravity.
    u16 behaviorFlags;                        ///< Flags controlling inhibition, cooperation and appearance persistence.
    s16 movementCollisionFlags;               ///< Result of the latest constrained movement.
    f32 targetPosition[3];                    ///< Stored target position in world coordinates.
    ActorMovementWorkspace movementWorkspace; ///< Scratch value selected by the current movement action.
    f32 collisionDepth;                       ///< Vertical span used by horizontal clearance queries.
    f32 clearanceHeight;                      ///< Permitted clearance height above the starting position.
    f32 collisionWidth;                       ///< Horizontal width used by terrain constraints.
    f32 unknown208;                           ///< Scaled kind-configuration value; its role remains unproven.
    f32 groundOffset;                         ///< Vertical offset subtracted from the ground-reference pose.
    s32 collisionRegistration;                ///< Collision-grid registration key, or zero when unregistered.

    s32 messengerContactMode;                ///< Policy used when a messenger contacts the actor.
    s32 messagePending;                      ///< Nonzero while a posted message awaits acknowledgement.
    ScriptMessage message;                   ///< Last accepted message, retained after acknowledgement.
    s32 fallbackObjectHandle;                ///< Object whose position supplies the fallback target.
    s32 creationScriptArgument;              ///< Script argument retained from actor creation.
    ActorRootPlaybackWorkspace rootPlayback; ///< Mode-dependent root orientation workspace.
    AnimationDeltaTrack3 rootMovementTrack;  ///< Incremental movement channels borrowing motion key data.
    s32 appearanceVariants[2];               ///< Model variant per appearance slot; zero selects its default model.
    ActorOrientationCache secondaryPoseReference; ///< Reference for the secondary signed part sequence.
    Matrix34 secondaryPoseTransform;              ///< Relative transform of the secondary signed part sequence.
    f32 secondaryPoseAngles[3];                   ///< Folded secondary pose pitch, yaw and roll in degrees.
    ActorOrientationCache primaryPoseReference;   ///< Reference for the primary signed part sequence.
    Matrix34 primaryPoseTransform;                ///< Relative transform of the primary signed part sequence.
    f32 primaryPoseAngles[3];                     ///< Folded primary pose pitch, yaw and roll in degrees.
    u8 *shakingPart;                              ///< Borrowed selected model part, or null when no shake is active.
    f32 partShakeTimeRemaining;                   ///< Seconds remaining in the temporary part shake.
    f32 partShakeReferenceAngles[3];              ///< Part rotation captured when selected for shaking.
    s32 targetedBurstAllowance; ///< Remaining paired target bursts allowed by the current costed action.
    s32 costedActionResult; ///< Script result: zero for insufficient resource, one for a response, three after starting.
    f32 actionResource;     ///< Action resource clamped to zero through 100.
    f32 secondaryMeter;     ///< Secondary gameplay meter clamped to zero through 100.
    f32 actionResourceHudLevel; ///< Smoothed action-resource HUD level on a zero-to-32 scale.
    f32 secondaryMeterHudLevel; ///< Smoothed secondary-meter HUD level on a zero-to-32 scale.
    s32 attachedEffectHandle;   ///< Object-manager handle of the latest attached effect, or zero.
} ActorScriptState;

/// Post a message to an actor's single script mailbox.
/// \param actorAddress Receiving actor's address in the original 32-bit ABI.
/// \param message Message to deliver; its storage need not outlive this call.
/// \param force Nonzero to replace an unread message.
/// \returns One when accepted, or zero when an unread message prevents delivery.
s32 actorPostScriptMessage(s32 actorAddress, ScriptMessage message, s32 force);

/// Notify an actor script of an item's impact without replacing an unread message.
/// \param actor Borrowed actor with initialized script/mailbox state.
/// \param item Borrowed initialized item whose impact speed is finite and representable as s32.
/// \returns One when the message is accepted, or zero when another message is pending.
/// No ownership of the item is transferred.
/// \note Original decompilation identifier: \c func_80118EE8 (0x80118EE8).
s32 actorPostItemImpactMessage(u8 *actor, u8 *item);

/// Acknowledge an actor's pending script message without erasing its contents.
/// \param actor Actor whose notification is cleared.
/// \returns The previous pending-message flag.
s32 actorAcknowledgeScriptMessage(u8 *actor);

typedef char ActorScriptStatePrefixSizeCheck[(sizeof(ActorScriptState) == 0x408) ? 1 : -1];

#define ACTOR_SCRIPT_OFFSET_CHECK(name, type, member, offset)                                                          \
    typedef char name[(((u32) & ((type *)0)->member) == (offset)) ? 1 : -1]
ACTOR_SCRIPT_OFFSET_CHECK(ActorPreviousPoseIdOffsetCheck, ActorScriptState, previousPoseId, 0xFE);
ACTOR_SCRIPT_OFFSET_CHECK(ActorCurrentPoseIdOffsetCheck, ActorScriptState, currentPoseId, 0xFF);
ACTOR_SCRIPT_OFFSET_CHECK(ActorContinuationActionOffsetCheck, ActorScriptState, continuationAction, 0x100);
ACTOR_SCRIPT_OFFSET_CHECK(ActorRootMotionTransformOffsetCheck, ActorScriptState, rootMotionTransform, 0x18);
ACTOR_SCRIPT_OFFSET_CHECK(ActorRootMotionHeadingOffsetCheck, ActorScriptState, rootMotionHeading, 0x48);
ACTOR_SCRIPT_OFFSET_CHECK(ActorScriptStateVmAddressOffsetCheck, ActorScriptState, vmAddress, 0x1B8);
ACTOR_SCRIPT_OFFSET_CHECK(ActorScriptStateMessengerContactModeOffsetCheck, ActorScriptState, messengerContactMode,
                          0x214);
ACTOR_SCRIPT_OFFSET_CHECK(ActorScriptStateMessagePendingOffsetCheck, ActorScriptState, messagePending, 0x218);
ACTOR_SCRIPT_OFFSET_CHECK(ActorScriptStateMessageOffsetCheck, ActorScriptState, message, 0x21C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateGroundReferenceOffsetCheck, ActorScriptState, groundReference, 0x0);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStatePartTableOffsetCheck, ActorScriptState, partTable, 0x4C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateDrawListsOffsetCheck, ActorScriptState, modelDrawLists, 0x54);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateDirectionAnimationOffsetCheck, ActorScriptState, directionAnimation, 0x60);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateEnvironmentColorOffsetCheck, ActorScriptState, environmentColor, 0xE0);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateTargetEnvironmentColorOffsetCheck, ActorScriptState, targetEnvironmentColor, 0xE4);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateEnvironmentColorTimeOffsetCheck, ActorScriptState, environmentColorTimeRemaining,
                          0xE8);
ACTOR_SCRIPT_OFFSET_CHECK(ActorMotionLoopCountOffsetCheck, ActorScriptState, loopCount, 0x11C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorMotionDurationOffsetCheck, ActorScriptState, motionDuration, 0x134);
ACTOR_SCRIPT_OFFSET_CHECK(ActorMotionRemainingOffsetCheck, ActorScriptState, motionRemaining, 0x138);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStatePlaybackModeOffsetCheck, ActorScriptState, playbackMode, 0x11A);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStatePendingActionOffsetCheck, ActorScriptState, pendingAction, 0x120);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateMovementScaleOffsetCheck, ActorScriptState, movementScale, 0x130);
ACTOR_SCRIPT_OFFSET_CHECK(ActorSelectedActionIdOffsetCheck, ActorScriptState, actionId, 0xEC);
ACTOR_SCRIPT_OFFSET_CHECK(ActorSelectedActionNameOffsetCheck, ActorScriptState, actionName, 0xF0);
ACTOR_SCRIPT_OFFSET_CHECK(ActorActionInitializerOffsetCheck, ActorScriptState, actionInitializer, 0x108);
ACTOR_SCRIPT_OFFSET_CHECK(ActorActionMovementScaleOffsetCheck, ActorScriptState, actionMovementScale, 0x104);
ACTOR_SCRIPT_OFFSET_CHECK(ActorPrimaryCueProgramOffsetCheck, ActorScriptState, primaryCueProgram, 0x110);
ACTOR_SCRIPT_OFFSET_CHECK(ActorSecondaryCueProgramOffsetCheck, ActorScriptState, secondaryCueProgram, 0x114);
ACTOR_SCRIPT_OFFSET_CHECK(ActorSelectedActionKindOffsetCheck, ActorScriptState, selectedActionKind, 0x13E);
ACTOR_SCRIPT_OFFSET_CHECK(ActorPrimaryCueClockOffsetCheck, ActorScriptState, primaryCueClock, 0x144);
ACTOR_SCRIPT_OFFSET_CHECK(ActorSecondaryCueClockOffsetCheck, ActorScriptState, secondaryCueClock, 0x148);
ACTOR_SCRIPT_OFFSET_CHECK(ActorPrimaryCueCommandOffsetCheck, ActorScriptState, primaryCueCommand, 0x14C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorLastPrimaryCueProgramOffsetCheck, ActorScriptState, lastPrimaryCueProgram, 0x150);
ACTOR_SCRIPT_OFFSET_CHECK(ActorSecondaryCueCommandOffsetCheck, ActorScriptState, secondaryCueCommand, 0x154);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateSequencerOffsetCheck, ActorScriptState, actionScript, 0x158);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionScriptIdOffsetCheck, ActorScriptState, actionScript.selectedProgramId, 0x158);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionElapsedOffsetCheck, ActorScriptState, actionScript.elapsed, 0x15C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionDurationOffsetCheck, ActorScriptState, actionScript.duration, 0x160);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionCommandTimeOffsetCheck, ActorScriptState, actionScript.commandTime, 0x164);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionCommandOffsetCheck, ActorScriptState, actionScript.command, 0x168);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionSoundOffsetCheck, ActorScriptState, actionScript.soundHandle, 0x16C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionLastRequestOffsetCheck, ActorScriptState, actionScript.lastRequestId, 0x170);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionPendingRequestOffsetCheck, ActorScriptState, actionScript.pendingRequestId,
                          0x174);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateSoundHandlesOffsetCheck, ActorScriptState, soundHandles, 0x178);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateNextSoundSlotOffsetCheck, ActorScriptState, nextSoundSlot, 0x188);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateTrackedVoiceOffsetCheck, ActorScriptState, trackedVoice, 0x18C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateHeldItemOffsetCheck, ActorScriptState, heldItemHandle, 0x190);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateAttachmentModeOffsetCheck, ActorScriptState, attachmentMode, 0x194);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateTargetHandleOffsetCheck, ActorScriptState, targetHandle, 0x198);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateApproachSampleOffsetCheck, ActorScriptState, unknown1A0, 0x1A0);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateAirborneOffsetCheck, ActorScriptState, airborne, 0x1BC);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateWorldMotionOffsetCheck, ActorScriptState, worldMotion, 0x1D8);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateBehaviorFlagsOffsetCheck, ActorScriptState, behaviorFlags, 0x1E8);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateTargetPositionOffsetCheck, ActorScriptState, targetPosition, 0x1EC);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateCollisionDepthOffsetCheck, ActorScriptState, collisionDepth, 0x1FC);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateCollisionRegistrationOffsetCheck, ActorScriptState, collisionRegistration, 0x210);
ACTOR_SCRIPT_OFFSET_CHECK(ActorRootPlaybackOffsetCheck, ActorScriptState, rootPlayback, 0x23C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorRootMovementTrackOffsetCheck, ActorScriptState, rootMovementTrack, 0x2B4);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateAppearanceVariantsOffsetCheck, ActorScriptState, appearanceVariants, 0x2F0);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateSecondaryPoseTransformOffsetCheck, ActorScriptState, secondaryPoseTransform, 0x32C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateSecondaryPoseAnglesOffsetCheck, ActorScriptState, secondaryPoseAngles, 0x35C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStatePrimaryPoseTransformOffsetCheck, ActorScriptState, primaryPoseTransform, 0x39C);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStatePrimaryPoseAnglesOffsetCheck, ActorScriptState, primaryPoseAngles, 0x3CC);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateShakingPartOffsetCheck, ActorScriptState, shakingPart, 0x3D8);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStatePartShakeTimeOffsetCheck, ActorScriptState, partShakeTimeRemaining, 0x3DC);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStatePartShakeAnglesOffsetCheck, ActorScriptState, partShakeReferenceAngles, 0x3E0);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateCreationArgumentOffsetCheck, ActorScriptState, creationScriptArgument, 0x238);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateSecondaryPoseReferenceOffsetCheck, ActorScriptState, secondaryPoseReference, 0x2F8);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStatePrimaryPoseReferenceOffsetCheck, ActorScriptState, primaryPoseReference, 0x368);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateBurstAllowanceOffsetCheck, ActorScriptState, targetedBurstAllowance, 0x3EC);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateResponseFlagOffsetCheck, ActorScriptState, costedActionResult, 0x3F0);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateActionResourceOffsetCheck, ActorScriptState, actionResource, 0x3F4);
ACTOR_SCRIPT_OFFSET_CHECK(ActorActionResourceHudLevelOffsetCheck, ActorScriptState, actionResourceHudLevel, 0x3FC);
ACTOR_SCRIPT_OFFSET_CHECK(ActorSecondaryMeterHudLevelOffsetCheck, ActorScriptState, secondaryMeterHudLevel, 0x400);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateSecondaryMeterOffsetCheck, ActorScriptState, secondaryMeter, 0x3F8);
ACTOR_SCRIPT_OFFSET_CHECK(ActorStateAttachedEffectOffsetCheck, ActorScriptState, attachedEffectHandle, 0x404);
ACTOR_SCRIPT_OFFSET_CHECK(ActorMovementWorkspaceOffsetCheck, ActorScriptState, movementWorkspace, 0x1F8);
#undef ACTOR_SCRIPT_OFFSET_CHECK

/// The selected resource occupies actor bytes 0xEC through 0x117.
/// Resource pose selectors become the actor's source and destination pose state.
#define ACTOR_ACTION_RESOURCE_MEMBER_CHECK(name, actorField, resourceField)                                            \
    typedef char name[(((u32) & ((ActorScriptState *)0)->actorField - (u32) &                                          \
                        ((ActorScriptState *)0)->actionId == (u32) & ((ActionDefinition *)0)->resourceField) &&        \
                       sizeof(((ActorScriptState *)0)->actorField) == sizeof(((ActionDefinition *)0)->resourceField))  \
                          ? 1                                                                                          \
                          : -1]
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceNameLayoutCheck, actionName, name);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceGravityLayoutCheck, actionGravityScale, gravityScale);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceAttributesLayoutCheck, actionAttributes, attributes);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceMotionLayoutCheck, motionId, motionId);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceSourcePoseLayoutCheck, previousPoseId, initialPoseId);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceDestinationPoseLayoutCheck, currentPoseId, finalPoseId);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceContinuationLayoutCheck, continuationAction, continuationAction);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceReservedLayoutCheck, unknown102, unknown016);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceMovementLayoutCheck, actionMovementScale, movementScale);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceInitializerLayoutCheck, actionInitializer, initializer);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceUpdateLayoutCheck, actionCallback, update);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourcePrimaryCueLayoutCheck, primaryCueProgram, primaryCueProgram);
ACTOR_ACTION_RESOURCE_MEMBER_CHECK(ActorActionResourceSecondaryCueLayoutCheck, secondaryCueProgram,
                                   secondaryCueProgram);
#undef ACTOR_ACTION_RESOURCE_MEMBER_CHECK
typedef char ActorActionResourceExtentCheck[((u32) & ((ActorScriptState *)0)->actionStartPending - (u32) &
                                             ((ActorScriptState *)0)->actionId == sizeof(ActionDefinition))
                                                ? 1
                                                : -1];

#endif
