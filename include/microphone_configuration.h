/// \file
/// Defines controller-thread microphone configuration requests and acknowledgement.

#ifndef MICROPHONE_CONFIGURATION_H
#define MICROPHONE_CONFIGURATION_H

#include "ultra64.h"

/// Operations accepted by the microphone configuration dispatcher.
typedef enum MicConfigurationCommand {
    MIC_CONFIG_AMPLIFIER_OFF = 0,
    MIC_CONFIG_TRIGGER_MODE = 1,
    MIC_CONFIG_DICTIONARY = 2,
    MIC_CONFIG_SESSION_ENABLED = 3,
    MIC_CONFIG_RECOGNITION_OVERRIDE = 4,
    MIC_CONFIG_RECOGNITION_FOLLOWS_TRIGGER = 5,
    MIC_CONFIG_TRIGGER_BUTTONS = 6,
    MIC_CONFIG_UNSUPPORTED_WORD_DICTIONARY = 7
} MicConfigurationCommand;

/// Retained controller-thread request; only arguments required by its command are meaningful.
/// Storage remains valid until pending becomes zero. Configuration may take effect later.
typedef struct MicConfigurationRequest {
    s16 type;      ///< Controller message kind; six identifies microphone configuration.
    s32 command;   ///< Operation from MicConfigurationCommand.
    s32 pending;   ///< Nonzero until the controller thread acknowledges dispatch.
    s32 argument0; ///< Toggle mode, dictionary ID, enable flag or trigger port mask, as selected by command.
    s32 argument1; ///< Initial toggle state or trigger button mask; unused for other supported commands.
} MicConfigurationRequest;

typedef char MicConfigurationRequestSizeCheck[(sizeof(MicConfigurationRequest) == 0x14) ? 1 : -1];
typedef char MicConfigurationPendingOffsetCheck[((u32) & ((MicConfigurationRequest *)0)->pending == 8) ? 1 : -1];
typedef char MicConfigurationArgumentOffsetCheck[((u32) & ((MicConfigurationRequest *)0)->argument1 == 0x10) ? 1 : -1];

/// Apply a queued microphone configuration request and acknowledge dispatch.
/// \param request Retained request with the arguments required by its command initialized.
/// \returns Nothing; clears pending after dispatch, which may precede asynchronous device changes.
/// \note Original decompilation identifier: \c func_8000432C (0x8000432C).
void micSessionDispatchRequest(MicConfigurationRequest *request);

#endif
