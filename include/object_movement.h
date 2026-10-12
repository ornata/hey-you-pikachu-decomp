/// \file
/// Declares pending local movement and camera-facing object orientation.

#ifndef OBJECT_MOVEMENT_H
#define OBJECT_MOVEMENT_H

#include <ultra64.h>

/// Retrieve the movement pending for the current object update.
/// \param movement Receives three local-coordinate movement components.
/// \returns Nothing.
/// \note Uses shared traversal state; valid within the current object update.
/// \note Original decompilation identifier: \c func_80008060 (0x80008060).
void objectGetPendingLocalMovement(f32 *movement);

/// Replace the movement pending for the current object update.
/// \param movement Borrowed three-component local-coordinate movement.
/// \returns Nothing.
/// \note Uses shared traversal state; valid within the current object update.
/// \note Original decompilation identifier: \c func_80008084 (0x80008084).
void objectSetPendingLocalMovement(f32 *movement);

/// Add movement to the current object update.
/// \param movement Borrowed three-component local-coordinate movement to add.
/// \returns Nothing.
/// \note Uses shared traversal state; valid within the current object update.
/// \note Original decompilation identifier: \c func_800080A8 (0x800080A8).
void objectAddPendingLocalMovement(f32 *movement);

/// Align an object with the active camera viewing direction.
/// \param object Live object whose orientation is updated.
/// \returns Nothing.
/// \pre The active camera is initialized.
/// \note Original decompilation identifier: \c func_80009AB8 (0x80009AB8).
void objectOrientAlongCameraView(u8 *object);

#endif
