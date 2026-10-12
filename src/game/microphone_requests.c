/// \file
/// Asynchronous microphone configuration requests.
/// \ingroup VoiceInteraction
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"

/* message handed to the pad thread (queued through D_80063450) */
typedef struct PadMsg {
    s16 type;
    s32 cmd;
    s32 pending;
    s32 arg0;
    s32 arg1;
} PadMsg;

/// Persistent ring of microphone configuration requests submitted to the controller thread.
/// \note Original identifier: \c D_80063450 (0x80063450).
extern PadMsg gMicrophoneRequestRing[8];
/// Next microphone configuration request slot, wrapping after eight submissions.
/// \note Original identifier: \c D_800634F0 (0x800634F0).
extern s32 gMicrophoneNextRequestIndex;
OSMesgQueue *controllerGetCommandQueue();
void micQueueConfigurationRequest(PadMsg *);

/// Submit a microphone configuration request for asynchronous processing.
/// \param msg Writable request specifying the operation and its configuration values;
/// marked pending by this call.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004670 (0x80004670).
/// \note Submission does not guarantee that the requested configuration has
/// finished taking effect when this call returns.
/// \see micSessionDispatchRequest
void micQueueConfigurationRequest(PadMsg *msg) {
    msg->pending = 1;
    msg->type = 6;
    gMicrophoneRequestRing[gMicrophoneNextRequestIndex] = *msg;
    osSendMesg(controllerGetCommandQueue(), &gMicrophoneRequestRing[gMicrophoneNextRequestIndex], 1);
    gMicrophoneNextRequestIndex += 1;
    if (gMicrophoneNextRequestIndex >= 8) {
        gMicrophoneNextRequestIndex = 0;
    }
}

/// Request that the microphone amplifier be turned off.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004738 (0x80004738).
void micRequestAmplifierOff(void) {
    PadMsg msg;

    msg.cmd = 0;
    micQueueConfigurationRequest(&msg);
}

/// Configure the microphone trigger mode and its initial toggle state.
/// \param toggleMode Nonzero for a toggled trigger; zero for a trigger held by a button.
/// \param initialState Initial trigger state when toggle mode is enabled.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000475C (0x8000475C).
void micRequestTriggerMode(s32 toggleMode, s32 initialState) {
    PadMsg msg;

    msg.cmd = 1;
    msg.arg0 = toggleMode;
    msg.arg1 = initialState;
    micQueueConfigurationRequest(&msg);
}

/// Request a voice dictionary for subsequent recognition.
/// \param dictionaryId Identifier of the voice dictionary to select.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000478C (0x8000478C).
/// \note Dictionary registration completes asynchronously through the session.
/// \see micSessionUpdateDictionary
void micRequestDictionary(s32 dictionaryId) {
    PadMsg msg;

    msg.cmd = 2;
    msg.arg0 = dictionaryId;
    micQueueConfigurationRequest(&msg);
}

/// Request that microphone session processing be enabled or disabled.
/// \param enabled Nonzero to enable session processing; zero to disable it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800047B8 (0x800047B8).
void micRequestSessionEnabled(s32 enabled) {
    PadMsg msg;

    msg.cmd = 3;
    msg.arg0 = enabled;
    micQueueConfigurationRequest(&msg);
}

/// Set the recognition request used when trigger-controlled recognition is disabled.
/// \param enabled Nonzero to request recognition; zero to suppress it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800047E4 (0x800047E4).
void micRequestRecognitionOverride(s32 enabled) {
    PadMsg msg;

    msg.cmd = 4;
    msg.arg0 = enabled;
    micQueueConfigurationRequest(&msg);
}

/// Choose whether voice recognition follows the microphone trigger.
/// \param enabled Nonzero to follow the trigger; zero to use the recognition override.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004810 (0x80004810).
void micRequestRecognitionFollowsTrigger(s32 enabled) {
    PadMsg msg;

    msg.cmd = 5;
    msg.arg0 = enabled;
    micQueueConfigurationRequest(&msg);
}

/// Choose the controller buttons used to trigger the microphone.
/// \param portMask Bit mask of controller ports whose buttons may trigger the microphone.
/// \param buttonMask Mask of controller buttons that activate the trigger.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000483C (0x8000483C).
void micRequestTriggerButtons(s32 portMask, s32 buttonMask) {
    PadMsg msg;

    msg.cmd = 6;
    msg.arg0 = portMask;
    msg.arg1 = buttonMask;
    micQueueConfigurationRequest(&msg);
}

/// Submit the unsupported custom-word dictionary request.
/// \param reserved0 Unused configuration value for the unsupported request.
/// \param reserved1 Unused configuration value for the unsupported request.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000486C (0x8000486C).
void micRequestUnsupportedWordDictionary(s32 reserved0, s32 reserved1) {
    PadMsg msg;

    msg.cmd = 7;
    msg.arg0 = reserved0;
    msg.arg1 = reserved1;
    micQueueConfigurationRequest(&msg);
}

/// Provide an inactive microphone operation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000489C (0x8000489C).
void micNoOp0489C(void) {
}

s32 micSessionIsDictionaryUploadActive(); /* extern */

/// Query whether a voice dictionary upload is active without exposing the result.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800048A4 (0x800048A4).
void micQueryDictionaryUploadNoResult(void) {
    micSessionIsDictionaryUploadActive();
}

s32 micSessionIsTriggerReady(void); /* extern */

/// Query microphone trigger readiness without exposing the result.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800048C4 (0x800048C4).
void micQueryTriggerReadinessNoResult(void) {
    micSessionIsTriggerReady();
}

s32 micSessionGetTriggerState(); /* extern */

/// Query the microphone trigger state without exposing the result.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800048E4 (0x800048E4).
void micQueryTriggerStateNoResult(void) {
    micSessionGetTriggerState();
}

s32 micSessionGetRequestedDictionary(); /* extern */

/// Query the requested voice dictionary without exposing the result.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004904 (0x80004904).
void micQueryRequestedDictionaryNoResult(void) {
    micSessionGetRequestedDictionary();
}

s32 micSessionGetDevicePort(); /* extern */

/// Query the selected Voice Recognition Unit port without exposing the result.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004924 (0x80004924).
void micQueryDevicePortNoResult(void) {
    micSessionGetDevicePort();
}

s32 micSessionIsDeviceConnected(s32);

/// Check whether a controller port has a usable Voice Recognition Unit.
/// \param port Controller port index, from 0 through 3.
/// \returns 1 when the port reports a voice device and voice initialization succeeded, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80004944 (0x80004944).
s32 micIsDeviceConnected(s32 port) {
    return micSessionIsDeviceConnected(port);
}
