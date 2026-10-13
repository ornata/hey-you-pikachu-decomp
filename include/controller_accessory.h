/// \file
/// Defines persistent controller accessory contexts and cached initialization state.

#ifndef CONTROLLER_ACCESSORY_H
#define CONTROLLER_ACCESSORY_H

#include <ultra64.h>

/// Established accessory lifecycle states maintained by the controller thread.
typedef enum ControllerAccessoryState {
    CONTROLLER_ACCESSORY_UNAVAILABLE = 0, ///< No available accessory was observed.
    CONTROLLER_ACCESSORY_READY = 2,       ///< Rumble Pak initialization succeeded.
    CONTROLLER_ACCESSORY_PENDING = 3,     ///< Accessory initialization is pending or being retried.
    CONTROLLER_ACCESSORY_FAILED = 4       ///< Rumble Pak initialization attempts were exhausted.
} ControllerAccessoryState;

/// Persistent SDK accessory contexts indexed by controller port.
/// Borrowed contexts may be reinitialized after a device change.
/// \note Original identifier: \c D_800ACE98 (0x800ACE98).
extern OSPfs gControllerAccessoryStorage[4];
typedef char ControllerAccessoryContextSizeCheck[(sizeof(OSPfs) == 0x68) ? 1 : -1];

/// Read cached accessory initialization state without probing the device.
/// \param port Controller port index from zero through three.
/// \returns Recorded ControllerAccessoryState value.
/// \note Original decompilation identifier: \c func_80002824 (0x80002824).
s32 controllerGetAccessoryState(s32 port);

/// Record accessory initialization state for controller-thread processing.
/// \param port Controller port index from zero through three.
/// \param state ControllerAccessoryState value; no validation is performed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80002838 (0x80002838).
void controllerSetAccessoryState(s32 port, s32 state);

/// Borrow a persistent controller accessory context.
/// \param port Controller port index from zero through three.
/// \returns SDK context owned by the controller subsystem; initialization may replace its contents.
/// \note Original decompilation identifier: \c func_8000284C (0x8000284C).
OSPfs *controllerGetAccessoryStorage(s32 port);

#endif
