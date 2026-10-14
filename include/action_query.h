/// \file
/// Declares action attributes, gravity scaling, motion identifiers and borrowed action names.
#ifndef ACTION_QUERY_H
#define ACTION_QUERY_H
#include <ultra64.h>

/// Recovered action permissions; other attribute bits retain numeric forms.
typedef enum ActionAttribute {
    ACTION_ATTRIBUTE_ALLOW_TURNING = 0x100000 ///< Allows action-controlled orientation updates.
} ActionAttribute;

/// Retrieve the current action's behavior attributes.
/// \param object Live actor with initialized action state.
/// \returns Current attributes, or the default mask 0x20100400 when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F594 (0x8000F594).
s32 actionGetAttributes(u8 *object);

/// Retrieve the current action's gravity multiplier.
/// \param object Live actor with initialized action state.
/// \returns Current action gravity multiplier, or one when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F5BC (0x8000F5BC).
f32 actionGetGravityScale(u8 *object);

/// Get the motion identifier of the currently playing action.
/// \param object Actor whose action state is accessed.
/// \returns The current motion identifier, or minus one when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F5E8 (0x8000F5E8).
s16 actionGetCurrentMotionId(u8 *object);

/// Get a display name for an action or action sentinel.
/// \param actionId Action identifier or negative action sentinel.
/// \returns The stored action name, a sentinel name, or an invalid-action marker; ownership remains with the action system.
/// \note Original decompilation identifier: \c func_8000F60C (0x8000F60C).
char *actionGetName(s32 actionId);

#endif
