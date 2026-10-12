/// \file
/// Defines three-channel animation curve state and its evaluation contracts.
#ifndef ANIMATION_TRACKS_H
#define ANIMATION_TRACKS_H

#include "ultra64.h"

/// Curve descriptor shared by archived data and writable playback state.
typedef struct AnimationCurveDescriptor {
    s32 kind;           ///< Zero selects a constant, one a cubic curve, and two a linear curve.
    s32 valueOrAddress; ///< Signed 16.16 constant, archive key offset, or relocated current-key address.
} AnimationCurveDescriptor;

/// Three independently timed animation channels borrowing their key data.
typedef struct AnimationTrack3 {
    AnimationCurveDescriptor curves[3]; ///< Current curve descriptors.
    f32 times[3];                       ///< Elapsed seconds within each current key.
    s32 keyIndices[3];                  ///< Number of keys passed in each channel.
} AnimationTrack3;

/// Three-channel state retaining values for incremental output.
typedef struct AnimationDeltaTrack3 {
    AnimationCurveDescriptor curves[3]; ///< Current curve descriptors.
    f32 times[3];                       ///< Elapsed seconds within each current key.
    s32 keyIndices[3];                  ///< Number of keys passed in each channel.
    f32 previousValues[3];              ///< Last evaluated values used to measure channel changes.
} AnimationDeltaTrack3;

typedef char AnimationCurveDescriptorSizeCheck[(sizeof(AnimationCurveDescriptor) == 8) ? 1 : -1];
typedef char AnimationTrack3SizeCheck[(sizeof(AnimationTrack3) == 0x30) ? 1 : -1];
typedef char AnimationDeltaTrack3SizeCheck[(sizeof(AnimationDeltaTrack3) == 0x3C) ? 1 : -1];
typedef char AnimationTrack3TimeOffsetCheck[((u32) & ((AnimationTrack3 *)0)->times == 0x18) ? 1 : -1];
typedef char AnimationTrack3IndexOffsetCheck[((u32) & ((AnimationTrack3 *)0)->keyIndices == 0x24) ? 1 : -1];
typedef char
    AnimationDeltaTrack3ValueOffsetCheck[((u32) & ((AnimationDeltaTrack3 *)0)->previousValues == 0x30) ? 1 : -1];

/// Initialize three animation channels.
/// \param dst Track state to initialize.
/// \param src Three source curve descriptors.
/// \param off Address offset added to each keyed curve reference.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000A34C (0x8000A34C).
void animationTrack3Initialize(AnimationTrack3 *dst, AnimationCurveDescriptor *src, s32 off);

/// Initialize three animation channels for incremental output.
/// \param dst Track state to initialize.
/// \param src Three source curve descriptors.
/// \param off Address offset added to each keyed curve reference.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000A3AC (0x8000A3AC).
void animationDeltaTrack3Initialize(AnimationDeltaTrack3 *dst, AnimationCurveDescriptor *src, s32 off);

/// Advance three animation channels and write their available values.
/// \param track Three-channel track state to advance.
/// \param output Receives up to three channel values; absent channels leave their values unchanged.
/// \param velocities Optional three-element slope output; constant channels write
/// zero, while held or absent channels leave their slopes unchanged.
/// \param step Time step in seconds.
/// \returns 1 when every channel is constant, absent, or unsupported; 0 otherwise.
/// \note Original decompilation identifier: \c func_8000A410 (0x8000A410).
s32 animationTrack3Evaluate(AnimationTrack3 *track, f32 *output, f32 *velocities, f32 step);

/// Advance three animation channels and add their changes to existing output values.
/// \param track Three-channel track state to advance.
/// \param output Three output values receiving each active or held channel's
/// change; constant, absent, or unsupported channels leave output unchanged.
/// \param velocities Optional three-element slope output; constant channels write
/// zero, while held or absent channels leave their slopes unchanged.
/// \param step Time step in seconds.
/// \returns 1 when every channel is constant, absent, or unsupported; 0 otherwise.
/// \note Original decompilation identifier: \c func_8000A4D4 (0x8000A4D4).
s32 animationDeltaTrack3Evaluate(AnimationDeltaTrack3 *track, f32 *output, f32 *velocities, f32 step);

#endif
