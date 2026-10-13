/// \file
/// Declares camera-relative launch and delivery of script-message projectiles.

#ifndef MESSENGER_H
#define MESSENGER_H

#include "script_message.h"

/// Launch a message projectile and dissipate the previously tracked messenger.
/// \param id Recipient object-part handle.
/// \param position Three world-space launch coordinates, copied during the call.
/// \param velocity Three initial velocity components, copied during the call.
/// \param scale Uniform projectile scale.
/// \param message Message copied for delivery to the recipient.
/// \param mode Zero for homing delivery, or nonzero for collision-based flight.
/// \param flags Nonzero to permit drawing in map and demonstration modes.
/// \returns Nothing; the object system manages the projectile lifetime.
/// \note Original identifier: \c func_80026AB8 (0x80026AB8).
void messengerCreate(s32 id, f32 *position, f32 *velocity, f32 scale, ScriptMessage message, s32 mode, s32 flags);

/// Choose a camera-relative launch position and velocity.
/// \param position Receives three world-space launch coordinates.
/// \param velocity Receives three world-space velocity components.
/// \param mode Zero for a curved homing launch, or nonzero for a faster direct launch.
/// \returns Nothing.
/// \pre The selected camera and its orientation are initialized.
/// \note Original identifier: \c func_80026C58 (0x80026C58).
void messengerGetCameraLaunch(f32 *position, f32 *velocity, s32 mode);

#endif
