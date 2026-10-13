#include "serial_queue.h"
#include "controller_pad.h"
#include "microphone_session_query.h"
#include "microphone_configuration.h"
#include "controller_commands.h"
/// \file
/// Device discovery, listening policy, and dictionary transitions.
/// \ingroup VoiceInteraction
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "eeprom_io.h"
#include "voice_recognition.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/* controller/pad polling state, 0x800AD420.. */
/// Recognition request used when recognition does not follow the trigger.
/// \note Original identifier: \c D_800AD420 (0x800AD420).
s32 gMicSessionRecognitionOverride;
/// Whether recognition requests follow the microphone trigger.
/// \note Original identifier: \c D_800AD424 (0x800AD424).
s32 gMicSessionRecognitionFollowsTrigger;
/// Current microphone trigger state.
/// \note Original identifier: \c D_800AD428 (0x800AD428).
s32 gMicSessionTriggerActive;
/// Microphone trigger state from the preceding update.
/// \note Original identifier: \c D_800AD42C (0x800AD42C).
s32 gMicSessionPreviousTriggerActive;
/// OS clock time of the last session reset and device discovery attempt.
/// \note Original identifier: \c D_800AD430 (0x800AD430).
u64 gMicSessionLastDeviceScanTicks;
/// Whether trigger button presses toggle the microphone trigger.
/// \note Original identifier: \c D_800AD438 (0x800AD438).
s32 gMicSessionTriggerToggleMode;
/// Dictionary being uploaded, or minus one when no upload is active.
/// \note Original identifier: \c D_800AD43C (0x800AD43C).
s32 gMicSessionUploadingDictionary;
/// Last successfully uploaded dictionary identifier, or minus one after reset.
/// \note Original identifier: \c D_800AD440 (0x800AD440).
s32 gMicSessionUploadedDictionary;
/// Dictionary identifier requested by session configuration.
/// \note Original identifier: \c D_800AD444 (0x800AD444).
s32 gMicSessionRequestedDictionary;
/// Controller ports whose buttons contribute to the microphone trigger.
/// \note Original identifier: \c D_800AD448 (0x800AD448).
u32 gMicSessionTriggerPortMask;
/// Controller buttons that activate the microphone trigger.
/// \note Original identifier: \c D_800AD44C (0x800AD44C).
s32 gMicSessionTriggerButtonMask;
/// Whether voice session updates are enabled.
/// \note Original identifier: \c D_800AD450 (0x800AD450).
s32 gMicSessionEnabled;
/// Pending request to disable the microphone amplifier.
/// \note Original identifier: \c D_800AD454 (0x800AD454).
s32 gMicSessionAmplifierOffRequested;
/// OS clock time of the most recent amplifier enable request.
/// \note Original identifier: \c D_800AD458 (0x800AD458).
u64 gMicSessionLastAmplifierOnTicks;
/// OS clock time of the most recent amplifier disable request.
/// \note Original identifier: \c D_800AD460 (0x800AD460).
u64 gMicSessionLastAmplifierOffTicks;
/// OS clock time when the microphone trigger last became inactive.
/// \note Original identifier: \c D_800AD468 (0x800AD468).
u64 gMicSessionTriggerReleaseTicks;
/// Controller ports eligible to host the Voice Recognition Unit.
/// \note Original identifier: \c D_800AD470 (0x800AD470).
s32 gMicSessionEligibleDevicePortMask;
/// Selected Voice Recognition Unit port, or minus one when unavailable.
/// \note Original identifier: \c D_800AD474 (0x800AD474).
s32 gMicSessionDevicePort;
/// Combined held buttons from the preceding trigger update.
/// \note Original identifier: \c D_80063420 (0x80063420).
s32 gMicSessionPreviousTriggerButtons = 0;
/// Most recent recognition-policy poll time with the amplifier disabled.
/// \note Original identifier: \c D_80063428 (0x80063428).
u64 gMicSessionLastAmplifierDisabledPollTicks = 0;
void micSessionUpdateTrigger(void);
void micSessionUpdateAmplifier(void);
void micSessionUpdateRecognitionEnable(s32);
void micSessionUpdateDictionary(void);
void micSessionResetAndFindDevice(void);

/// Persistent EEPROM request shared with the controller thread until acknowledgment.
/// \note Original identifier: \c D_80063430 (0x80063430).
EepromRequest gEepromQueuedRequest = {0};
/// Whether session policy requests an active recognition read.
/// \note Original identifier: \c D_800F8AC8 (0x800F8AC8).
extern volatile s32 gVoiceRecognitionReadRequested;

/// Update the microphone trigger from the configured controller buttons.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80003B50 (0x80003B50).
void micSessionUpdateTrigger(void) {
    s32 port;
    s32 heldButtons = 0;
    OSContPad pad;
    gMicSessionPreviousTriggerActive = gMicSessionTriggerActive;
    for (port = 0; port != 4; port++) {
        if ((1 << port) & gMicSessionTriggerPortMask) {
            controllerReadCachedPad(&pad, port);
            heldButtons |= pad.button;
        }
    }
    if (gMicSessionTriggerToggleMode != 0) {
        if ((heldButtons & gMicSessionTriggerButtonMask) && !(gMicSessionPreviousTriggerButtons & gMicSessionTriggerButtonMask)) {
            gMicSessionTriggerActive = !gMicSessionTriggerActive;
            TRACE("mic toggle %s\n", gMicSessionTriggerActive ? "ON" : "OFF");
        }
    } else if (heldButtons & gMicSessionTriggerButtonMask) {
        gMicSessionTriggerActive = 1;
        TRACE("mic trigger on\n");
    } else {
        gMicSessionTriggerActive = 0;
    }
    if (gMicSessionPreviousTriggerActive != 0 && gMicSessionTriggerActive == 0) {
        gMicSessionTriggerReleaseTicks = osGetTime();
    }
    gMicSessionPreviousTriggerButtons = heldButtons;
}

/// Enable the microphone amplifier for the current voice session.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80003C98 (0x80003C98).
void micSessionEnableAmplifier(void) {
    TRACE("AMP_ON\n");
    voiceRecognitionSetAmplifier(8);
    gMicSessionLastAmplifierOnTicks = osGetTime();
    TRACE("AMP_ON_B\n");
}

/// Disable the microphone amplifier for the current voice session.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80003CC8 (0x80003CC8).
void micSessionDisableAmplifier(void) {
    TRACE("AMP_OFF\n");
    voiceRecognitionSetAmplifier(0);
    gMicSessionLastAmplifierOffTicks = osGetTime();
    TRACE("AMP_OFF_B\n");
}

/// Maintain the microphone amplifier according to trigger state and timing.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80003CF8 (0x80003CF8).
void micSessionUpdateAmplifier(void) {
    u64 now;
    u64 last;
    now = osGetTime();
    if (gMicSessionAmplifierOffRequested != 0) {
        micSessionDisableAmplifier();
        gMicSessionAmplifierOffRequested = 0;
        return;
    }
    if (gMicSessionTriggerActive != 0 && (gMicSessionPreviousTriggerActive == 0 || now - gMicSessionLastAmplifierOnTicks >= 0xDF84758)) {
        micSessionEnableAmplifier();
        return;
    }
    if (voiceRecognitionIsAmplifierEnabled() != 0) {
        if (gMicSessionTriggerActive == 0 && (last = gMicSessionTriggerReleaseTicks, now - last >= 0xABA95) && last > gMicSessionLastAmplifierOffTicks) {
            micSessionDisableAmplifier();
        }
    } else if (now - gMicSessionLastAmplifierOffTicks >= 0x1AD2748) {
        micSessionEnableAmplifier();
    }
}

/// Update whether recognition is permitted after microphone startup.
/// \param enabled Nonzero to request recognition; zero to suppress it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80003E8C (0x80003E8C).
void micSessionUpdateRecognitionEnable(s32 enabled) {
    if (voiceRecognitionIsAmplifierEnabled() == 0) {
        gMicSessionLastAmplifierDisabledPollTicks = osGetTime();
    }
    if ((enabled != 0) && (osGetTime() - gMicSessionLastAmplifierDisabledPollTicks >= 0xE4E1C)) {
        gVoiceRecognitionReadRequested = 1;
        return;
    }
    gVoiceRecognitionReadRequested = 0;
}


/// Advance the pending change to the requested voice dictionary.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80003F30 (0x80003F30).
/// \note Recognition remains suppressed while a dictionary change is pending.
/// The session waits for the active read to stop before registering the vocabulary.
/// \see voiceDictionaryUpload
/// \see micSessionUpdate
void micSessionUpdateDictionary(void) {
    if (micSessionIsDictionaryUploadActive() == 0) {
        if ((voiceRecognitionGetCommandStatus() == 0) && (voiceRecognitionIsReadActive() == 0)) {
            gMicSessionUploadingDictionary = gMicSessionRequestedDictionary;
            TRACE("_changeDic: newDictionary:%d\n", gMicSessionUploadingDictionary);
            goto block_4;
        }
    } else {
    block_4:
        if (voiceDictionaryUpload(gMicSessionUploadingDictionary, 0xA) == 1) {
            gMicSessionUploadedDictionary = gMicSessionUploadingDictionary;
            TRACE("_changeDic: _sendincDicNum:%d\n", gMicSessionUploadedDictionary);
            gMicSessionUploadingDictionary = -1;
        }
    }
}

/* Select the first enabled port whose controller is a Voice Recognition Unit: bind the
 * voice handle to it and record it in D_800AD474 when the unit initializes. */
#define SCAN_VOICE_PORT(port, status)                                                                                  \
    do {                                                                                                               \
        TRACE("_micAvailPort:0x%02x\n", gMicSessionEligibleDevicePortMask);                                                                   \
        for ((port) = 0; (port) != MAXCONTROLLERS; (port)++) {                                                         \
            if ((1 << (port)) & gMicSessionEligibleDevicePortMask) {                                                                          \
                TRACE(" checking port(%d)\n", (port));                                                                 \
                if (((status) = controllerGetPortStatus(port), (status)->errno == 0) &&                                          \
                    (((status)->type & CONT_TYPE_MASK) == CONT_TYPE_VOICE)) {                                          \
                    TRACE("mic found:%d\n", (port));                                                                   \
                    voiceRecognitionInitialize(serialGetMessageQueue(), (port));                                                       \
                    if (voiceRecognitionGetDeviceState() == 1) {                                                                        \
                        gMicSessionDevicePort = (port);                                                                           \
                    } else {                                                                                           \
                        TRACE("failed to initialize mic(port %d)\n", (port));                                          \
                    }                                                                                                  \
                    break;                                                                                             \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/// Reset the voice session and select an available Voice Recognition Unit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80003FB8 (0x80003FB8).
void micSessionResetAndFindDevice(void) {
    s32 port;
    OSContStatus *status;

    gMicSessionTriggerActive = gMicSessionPreviousTriggerActive = 0;
    gMicSessionUploadingDictionary = gMicSessionUploadedDictionary = -1;
    if (gMicSessionRequestedDictionary == -1) {
        gMicSessionRequestedDictionary = 0;
    }
    gMicSessionAmplifierOffRequested = 0;
    gMicSessionLastDeviceScanTicks = gMicSessionLastAmplifierOnTicks = gMicSessionLastAmplifierOffTicks = gMicSessionTriggerReleaseTicks = osGetTime();
    gMicSessionDevicePort = -1;
    SCAN_VOICE_PORT(port, status);
}

/// Check whether a voice dictionary upload is in progress.
/// \returns 1 during an active upload, or 0 otherwise; a requested change that
/// has not started uploading does not make this active.
/// \note Original decompilation identifier: \c func_80004108 (0x80004108).
s32 micSessionIsDictionaryUploadActive(void) {
    return (gMicSessionUploadingDictionary + 1) != 0;
}

/// Check whether the session is ready for its current microphone trigger state.
/// \returns 1 when the dictionary is settled and an active trigger has microphone input enabled, or 0 otherwise.
/// \note Original decompilation identifier: \c func_8000411C (0x8000411C).
int micSessionIsTriggerReady(void) {
    s32 active = gMicSessionTriggerActive;
    return !micSessionIsDictionaryUploadActive() && ((active && voiceRecognitionIsAmplifierEnabled()) || !active);
}

/// Get the current microphone trigger state.
/// \returns Nonzero when the trigger is active, or zero when inactive.
/// \note Original decompilation identifier: \c func_80004180 (0x80004180).
s32 micSessionGetTriggerState(void) {
    return gMicSessionTriggerActive;
}

/// Get the voice dictionary requested for the session.
/// \returns Requested dictionary identifier.
/// \note Original decompilation identifier: \c func_8000418C (0x8000418C).
s32 micSessionGetRequestedDictionary(void) {
    return gMicSessionRequestedDictionary;
}

/// Get the controller port selected for the Voice Recognition Unit.
/// \returns Selected port index, or -1 when no device has initialized.
/// \note Original decompilation identifier: \c func_80004198 (0x80004198).
s32 micSessionGetDevicePort(void) {
    return gMicSessionDevicePort;
}

/// Check whether a port has a usable Voice Recognition Unit.
/// \param port Controller port index, from 0 through 3.
/// \returns 1 when the port reports a voice device and voice initialization succeeded, or 0 otherwise.
/// \note Original decompilation identifier: \c func_800041A4 (0x800041A4).
s32 micSessionIsDeviceConnected(s32 port) {
    OSContStatus *status;

    status = controllerGetPortStatus(port);
    if ((status->errno == 0) && ((status->type & 0x1F07) == 0x100) && (voiceRecognitionGetDeviceState() == 1)) {
        return 1;
    }
    TRACE(" mic-disconnect\n  errno:%d\n  type:0x%08x\n  connect:%d\n", status->errno, status->type, port);
    return 0;
}

/// Advance the enabled voice session and recover from device disconnection.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004204 (0x80004204).
/// \see micQueueConfigurationRequest
/// \see voiceRecognitionPoll
void micSessionUpdate(void) {
    if (gMicSessionEnabled != 0) {
        if ((gMicSessionDevicePort >= 0) && (micSessionIsDeviceConnected(gMicSessionDevicePort) != 0)) {
            micSessionUpdateTrigger();
            micSessionUpdateAmplifier();
            if ((gMicSessionUploadedDictionary != gMicSessionRequestedDictionary) || (micSessionIsDictionaryUploadActive() != 0)) {
                micSessionUpdateRecognitionEnable(0);
                micSessionUpdateDictionary();
            } else if (gMicSessionRecognitionFollowsTrigger != 0) {
                micSessionUpdateRecognitionEnable(gMicSessionTriggerActive);
            } else {
                micSessionUpdateRecognitionEnable(gMicSessionRecognitionOverride);
            }
            if (micSessionIsDictionaryUploadActive() == 0) {
                voiceRecognitionPoll();
            }
        } else {
            if (osGetTime() - gMicSessionLastDeviceScanTicks >= 0x02CB4178) {
                micSessionResetAndFindDevice();
            }
        }
    }
}

/// Apply a microphone configuration request and acknowledge its completion.
/// \param request Configuration request to apply and acknowledge.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000432C (0x8000432C).
void micSessionDispatchRequest(MicConfigurationRequest *request) {
    switch (request->command) {
    case MIC_CONFIG_AMPLIFIER_OFF:
        gMicSessionAmplifierOffRequested = 1;
        break;
    case MIC_CONFIG_TRIGGER_MODE:
        if (request->argument0 != 0) {
            gMicSessionTriggerToggleMode = 1;
            gMicSessionTriggerActive = request->argument1;
        } else {
            gMicSessionTriggerToggleMode = 0;
        }
        break;
    case MIC_CONFIG_DICTIONARY:
        gMicSessionRequestedDictionary = request->argument0;
        TRACE("_change_dictionary(%d)\n", gMicSessionRequestedDictionary);
        break;
    case MIC_CONFIG_SESSION_ENABLED:
        gMicSessionEnabled = request->argument0;
        break;
    case MIC_CONFIG_RECOGNITION_OVERRIDE:
        gMicSessionRecognitionOverride = request->argument0;
        break;
    case MIC_CONFIG_RECOGNITION_FOLLOWS_TRIGGER:
        gMicSessionRecognitionFollowsTrigger = request->argument0;
        break;
    case MIC_CONFIG_TRIGGER_BUTTONS:
        gMicSessionTriggerPortMask = request->argument0;
        gMicSessionTriggerButtonMask = request->argument1;
        break;
    case MIC_CONFIG_UNSUPPORTED_WORD_DICTIONARY:
        TRACE("ERROR: mic64SetWordDic() is not supported.\n");
        break;
    default:
        TRACE("ERROR: Unknown command:%d\n", request->command);
        break;
    }
    request->pending = 0;
}

/// Initialize microphone controls and discover an eligible voice device.
/// \param portMask Bit mask of controller ports eligible to host the voice device.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800043F4 (0x800043F4).
void micSessionInitialize(s32 portMask) {
    gMicSessionTriggerToggleMode = 0;
    gMicSessionTriggerPortMask = 1;
    gMicSessionTriggerButtonMask = 0x2000;
    gMicSessionEnabled = 0;
    gMicSessionRecognitionOverride = 0;
    gMicSessionRecognitionFollowsTrigger = 1;
    gMicSessionDevicePort = -1;
    gMicSessionEligibleDevicePortMask = portMask;
    micSessionResetAndFindDevice();
}


/// Query the EEPROM device type without exposing the result.
/// \returns Nothing.
/// \note Original decompilation identifier: \c n_alSeqpDelete (0x80004460).
void eepromQueryDeviceTypeNoResult(void) {
    eepromGetDeviceType();
}


/// Check whether an EEPROM request or transfer is outstanding.
/// \returns 1 while a request or transfer is busy, or 0 otherwise.
///
/// \note NONPORTABLE: `int` return type chosen for IDO register allocation
/// \note Original decompilation identifier: \c func_80004480 (0x80004480).
int eepromIsRequestOrTransferBusy(void) {
    return gEepromQueuedRequest.pending != 0 || eepromIsTransferBusy() != 0;
}

/// Wait until the outstanding EEPROM request and transfer have finished.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800044BC (0x800044BC).
void eepromWaitForIdle(void) {
    if (eepromIsRequestOrTransferBusy() != 0) {
        do {
            osYieldThread();
        } while (eepromIsRequestOrTransferBusy() != 0);
    }
}


/// Get the state of the EEPROM transfer managed by queued requests.
/// \returns 0 when idle or unavailable, 1/2 for read/write in progress, or 3/4/5 for read/checksum/write failure.
/// \note Original decompilation identifier: \c func_800044FC (0x800044FC).
s32 eepromGetQueuedTransferState(void) {
    return eepromGetTransferState();
}

/// Submit an EEPROM read and wait for the request to be accepted.
/// \param page First eight-byte EEPROM page to read.
/// \param backupPage Fallback starting page, or a negative value to disable
/// fallback; must fit in a signed 16-bit integer.
/// \param destination RAM address of writable payload storage, valid until the transfer finishes.
/// \param byteCount Payload size in bytes, from 0 through 508, excluding its
/// checksum; the rounded-up payload and checksum must fit in the EEPROM page range.
/// \returns 0 if started, 3 if a 4-Kbit EEPROM is unavailable, or a busy transfer state. The transfer may still be in progress.
/// \note Original decompilation identifier: \c func_8000451C (0x8000451C).
s32 eepromRequestRead(s32 page, s32 backupPage, void *destination, s32 byteCount) {
    eepromWaitForIdle();
    gEepromQueuedRequest.pending = 1;
    gEepromQueuedRequest.type = GAME_MESSAGE_EEPROM;
    gEepromQueuedRequest.command = EEPROM_REQUEST_READ;
    gEepromQueuedRequest.page = page;
    gEepromQueuedRequest.backupPage = backupPage;
    gEepromQueuedRequest.buffer = destination;
    gEepromQueuedRequest.byteCount = byteCount;
    osSendMesg(controllerGetCommandQueue(), &gEepromQueuedRequest, 1);
    if (gEepromQueuedRequest.pending != 0) {
        do {
            osYieldThread();
        } while (gEepromQueuedRequest.pending != 0);
    }
    return gEepromQueuedRequest.result;
}

/// Submit an EEPROM write and wait for the request to be accepted.
/// \param page First eight-byte EEPROM page to write.
/// \param backupPage Second copy starting page, or a negative value to disable
/// it; must fit in a signed 16-bit integer.
/// \param source RAM address of the payload to write; may be reused after this
/// call returns.
/// \param byteCount Payload size in bytes, from 0 through 508, excluding its
/// checksum; the rounded-up payload and checksum must fit in the EEPROM page range.
/// \returns 0 if started, 5 if a 4-Kbit EEPROM is unavailable, or a busy transfer state. The transfer may still be in progress.
/// \note Portability: the empty statement break preserves the original IDO register allocation.
/// \note Original decompilation identifier: \c func_800045C0 (0x800045C0).
s32 eepromRequestWrite(s32 page, s32 backupPage, void *source, s32 byteCount) {
    eepromWaitForIdle();
    do {
    } while (0);
    gEepromQueuedRequest.pending = 1;
    gEepromQueuedRequest.type = GAME_MESSAGE_EEPROM;
    gEepromQueuedRequest.command = EEPROM_REQUEST_WRITE;
    gEepromQueuedRequest.page = page;
    gEepromQueuedRequest.backupPage = backupPage;
    gEepromQueuedRequest.buffer = source;
    gEepromQueuedRequest.byteCount = byteCount;
    osSendMesg(controllerGetCommandQueue(), &gEepromQueuedRequest, 1);
    while (gEepromQueuedRequest.pending != 0) {
        osYieldThread();
    }
    return gEepromQueuedRequest.result;
}
