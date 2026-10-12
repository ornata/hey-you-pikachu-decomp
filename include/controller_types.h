/// \file
/// Defines the processed controller sample shared by input producers and game consumers.
#ifndef CONTROLLER_TYPES_H
#define CONTROLLER_TYPES_H
#include <ultra64.h>

/// One controller port's processed sample for the current input update.
/// Button masks occupy the low sixteen bits; stick values retain signed input.
typedef struct ControllerState {
    u32 held;     ///< Buttons held in the current sample.
    u32 pressed;  ///< Buttons newly pressed since the previous sample.
    u32 released; ///< Buttons released since the previous sample.
    u32 repeated; ///< Buttons generating an auto-repeat event.
    s32 stickX;   ///< Signed horizontal stick input.
    s32 stickY;   ///< Signed vertical stick input.
} ControllerState;
typedef char ControllerStateSizeCheck[(sizeof(ControllerState) == 0x18) ? 1 : -1];
typedef char ControllerStickOffsetCheck[(((u32) & ((ControllerState *)0)->stickX) == 0x10) ? 1 : -1];
#endif
