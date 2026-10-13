/// \file
/// Declares player interaction, view control and gameplay music-ducking controls.

#ifndef PLAYER_INTERACTION_H
#define PLAYER_INTERACTION_H

#include <ultra64.h>

/// Reset the primary player view orientation and immediately refresh the primary camera.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_80112520 (0x80112520).
void playerResetViewOrientation(void);

/// Check whether story progress and the primary player input permissions allow companion-control toggling.
/// \returns One if toggling is available, or zero otherwise.
/// \note Original decompilation identifier: \c func_80112830 (0x80112830).
s32 playerCanToggleCompanionControl(void);

/// Check whether story progress and the primary player input permissions allow stationary look control.
/// \returns One if stationary look control is available, or zero otherwise.
/// \note Original decompilation identifier: \c func_8011288C (0x8011288C).
s32 playerCanEnterStationaryLook(void);

/// Update automatic music attenuation for the current gameplay and voice-interaction state, respecting
/// inhibited transitions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80112B48 (0x80112B48).
void audioUpdateGameplayMusicDucking(void);

/// Assign the controller used by the primary player.
/// \param port Controller port from zero through three; other values supply neutral input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801119F4 (0x801119F4).
void playerSetControllerPort(s32 port);

/// Control which automatic music-volume transitions are inhibited.
/// \param flags Inhibit mask: value one prevents reducing music volume and value two prevents restoring it.
/// Combine them to prevent both; zero allows both transitions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801129DC (0x801129DC).
void audioSetMusicDuckingInhibitFlags(s32 flags);

/// Take an eligible item into the primary player hand, releasing the previously held item.
/// \param id Available item handle to pick up.
/// \returns One if the item was picked up, or zero if it is unavailable or cannot be picked up.
/// \note Original decompilation identifier: \c func_8011271C (0x8011271C).
s32 playerPickUpItem(s32 id);

/// Set the primary player view yaw when view-angle changes are enabled.
/// \param yaw Requested yaw in degrees, wrapped to a full turn.
/// \returns Nothing.
///
/// \note Angles must be finite and small enough for full-turn adjustments to change their floating-point
/// value.
/// \note Original decompilation identifier: \c func_801124C8 (0x801124C8).
void playerSetViewYaw(f32 yaw);

/// Set the primary player view pitch when view-angle changes are enabled.
/// \param pitch Requested pitch in degrees, interpreted as a signed angle and limited to minus sixty through
/// sixty degrees.
/// \returns Nothing.
///
/// \note Angles must be finite and small enough for full-turn adjustments to change their floating-point
/// value.
/// \note Original decompilation identifier: \c func_801124F4 (0x801124F4).
void playerSetViewPitch(f32 pitch);

/// Constrain a requested actor heading according to the active action facing restrictions.
/// \param object Live actor with initialized action state and facing restrictions.
/// \param angle Requested heading in degrees.
/// \returns The permitted heading in degrees, or the original angle when the active action has no facing
/// restriction.
///
/// \note Angles must be finite and small enough for full-turn adjustments to change their floating-point
/// value.
/// \note Original decompilation identifier: \c func_80112D24 (0x80112D24).
f32 actorConstrainFacingHeading(u8 *object, f32 angle);

#endif
