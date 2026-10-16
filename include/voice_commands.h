/// \file
/// Defines gameplay voice-command flags, state, and script-message interfaces.

#ifndef VOICE_COMMANDS_H
#define VOICE_COMMANDS_H

#include "script_message.h"
#include "voice_recognition.h"
#include "voice_phrases.h"
#include "voice_item_selector.h"

/// Gameplay quality flags combined with device warnings in message argument 1.
/// \ingroup VoiceInteraction
/// These flags describe a prepared voice command; they are not VRU error codes.
typedef enum VoiceCommandQualityFlag {
    VOICE_COMMAND_QUIET = 0x400,         ///< Input was silent or below the configured voice-level threshold.
    VOICE_COMMAND_DISTANT_MATCH = 0x4000 ///< Match distance exceeded the configured threshold.
} VoiceCommandQualityFlag;

/// Consume recognition and prepare a gameplay command when a usable word is available.
/// \param out Destination for the prepared message, unchanged when no message is produced.
/// \returns One when a message is produced, otherwise zero.
s32 voiceCommandTakeMessage(ScriptMessage *out);

/// Launch a visual messenger for a prepared voice command when gameplay permits.
/// \param message Command and quality information to attempt to deliver.
/// \returns Nothing.
/// \note Launching a messenger does not guarantee acceptance by the recipient.
void voiceCommandLaunchEffect(ScriptMessage message);

/// Process a pending recognition result and attempt gameplay delivery.
/// \returns Nothing.
void voiceCommandUpdate(void);

#endif
