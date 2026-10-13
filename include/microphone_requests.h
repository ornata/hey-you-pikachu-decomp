/// \file
/// Declares asynchronous microphone configuration requests issued by gameplay.
/// Requests require the initialized controller thread; return does not imply device completion.

#ifndef MICROPHONE_REQUESTS_H
#define MICROPHONE_REQUESTS_H

#include "ultra64.h"

/// Request that the microphone amplifier be turned off.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004738 (0x80004738).
void micRequestAmplifierOff(void);

/// Configure the microphone trigger mode and its initial toggle state.
/// \param toggleMode Nonzero for a toggled trigger; zero for a trigger held by a button.
/// \param initialState Initial trigger state when toggle mode is enabled.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000475C (0x8000475C).
void micRequestTriggerMode(s32 toggleMode, s32 initialState);

/// Request a voice dictionary for subsequent recognition.
/// \param dictionaryId Identifier of the voice dictionary to select.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000478C (0x8000478C).
/// \note Dictionary registration completes asynchronously through the session.
/// \see micSessionUpdateDictionary
void micRequestDictionary(s32 dictionaryId);

/// Request that microphone session processing be enabled or disabled.
/// \param enabled Nonzero to enable session processing; zero to disable it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800047B8 (0x800047B8).
void micRequestSessionEnabled(s32 enabled);

/// Set the recognition request used when trigger-controlled recognition is disabled.
/// \param enabled Nonzero to request recognition; zero to suppress it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800047E4 (0x800047E4).
void micRequestRecognitionOverride(s32 enabled);

/// Choose whether voice recognition follows the microphone trigger.
/// \param enabled Nonzero to follow the trigger; zero to use the recognition override.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004810 (0x80004810).
void micRequestRecognitionFollowsTrigger(s32 enabled);

/// Choose the controller buttons used to trigger the microphone.
/// \param portMask Bit mask of controller ports whose buttons may trigger the microphone.
/// \param buttonMask Mask of controller buttons that activate the trigger.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000483C (0x8000483C).
void micRequestTriggerButtons(s32 portMask, s32 buttonMask);

/// Submit the unsupported custom-word dictionary request.
/// \param reserved0 Unused configuration value for the unsupported request.
/// \param reserved1 Unused configuration value for the unsupported request.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000486C (0x8000486C).
void micRequestUnsupportedWordDictionary(s32 reserved0, s32 reserved1);

#endif
