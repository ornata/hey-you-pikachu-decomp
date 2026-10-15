/// \file
/// Defines recognition vocabulary, word records, and voice recognition interfaces.

#ifndef VOICE_RECOGNITION_H
#define VOICE_RECOGNITION_H

#include <ultra64.h>

/// Recorded availability of the recognition device, distinct from command/read progress.
typedef enum VoiceRecognitionDeviceState {
    VOICE_RECOGNITION_UNAVAILABLE = 0, ///< Disconnected or not yet confirmed.
    VOICE_RECOGNITION_READY = 1,       ///< Initialized device available for operations.
    VOICE_RECOGNITION_ERROR = 2        ///< Device operation failed and recovery is required.
} VoiceRecognitionDeviceState;

/// Dictionary-selection sentinel; ordinary dictionary identifiers are nonnegative.
/// This describes retained selection fields, not whether hardware vocabulary was cleared.
enum { VOICE_DICTIONARY_NONE = -1 };

/// Result-index sentinel returned when no completed recognition result is pending.
enum { VOICE_RECOGNITION_NO_RESULT = -1 };

/// Amplifier settings accepted by the game-facing recognition interface.
/// These are device-operation arguments, distinct from the retained Boolean amplifier state.
typedef enum VoiceAmplifierSetting {
    VOICE_AMPLIFIER_OFF = 0, ///< Disable microphone amplification.
    VOICE_AMPLIFIER_ON = 8   ///< Enable microphone amplification.
} VoiceAmplifierSetting;

/// Session-policy request for recognition reading, distinct from an active read.
/// The microphone session writes this flag; recognition polling services it.
/// \note Original identifier: \c D_800F8AC8 (0x800F8AC8).
extern volatile s32 gVoiceRecognitionReadRequested;

/// Vocabulary selected for recognition, expressed as global word identifiers.
/// \ingroup VoiceInteraction
typedef struct VoiceDictionary {
    s32 count;    ///< Number of entries in wordIds.
    s32 *wordIds; ///< Borrowed ordered list of global word identifiers.
} VoiceDictionary;

/// Global word definition linking device vocabulary to a gameplay command.
/// \ingroup VoiceInteraction
typedef struct VoiceWord {
    char *name;       ///< Borrowed zero-terminated display name.
    u8 *text;         ///< Borrowed zero-terminated device vocabulary text.
    s32 commandType;  ///< Gameplay message category associated with this word.
    s32 commandValue; ///< Gameplay command argument associated with this word.
} VoiceWord;

/// Resolve an entry in the selected vocabulary to a global word identifier.
/// \param index Valid entry index in the selected dictionary.
/// \returns Global word identifier, or zero when no dictionary is selected.
/// \note Original decompilation identifier: \c func_80040440 (0x80040440).
s32 voiceDictionaryGetWordId(u16 index);

/// Get the display name of an entry in the selected vocabulary.
/// \param index Dictionary-local word index.
/// \returns Borrowed name from resident vocabulary storage, or an empty string for an invalid or unavailable entry.
/// \note Original decompilation identifier: \c func_80040C8C (0x80040C8C).
char *voiceDictionaryGetWordName(u16 index);

/// Get the gameplay message category associated with a vocabulary entry.
/// \param index Dictionary-local word index.
/// \returns Command category, or -1 for an invalid or unavailable entry.
/// \note Original decompilation identifier: \c func_80040CF8 (0x80040CF8).
s32 voiceDictionaryGetCommandType(u16 index);

/// Get the gameplay command argument associated with a vocabulary entry.
/// \param index Dictionary-local word index.
/// \returns Command value, or -1 for an invalid or unavailable entry.
/// \note Original decompilation identifier: \c func_80040D60 (0x80040D60).
s32 voiceDictionaryGetCommandValue(u16 index);

/// Initialize recognition and load the resident vocabulary for a connected device.
/// \param queue Initialized serial-interface queue, retained for the device lifetime.
/// \param channel Controller port connected to the device, from zero through three.
/// \returns Nothing; device state reports initialization success or failure.
/// \note Original decompilation identifier: \c func_800404E8 (0x800404E8).
void voiceRecognitionInitialize(OSMesgQueue *queue, s32 channel);

/// Advance registration of a selected vocabulary on the ready device.
/// \param dictionary Valid dictionary index to select and upload.
/// \param batch Positive word limit for this call, or zero for all remaining words.
/// \returns One when registration is complete, or zero while incomplete or after a device error.
/// \note Completion does not start recognition reading.
/// \note Original decompilation identifier: \c func_800406B4 (0x800406B4).
s32 voiceDictionaryUpload(s32 dictionary, s32 batch);

/// Advance recognition and retain a newly completed result.
/// \returns Nothing; a later completed read replaces the retained result.
/// \note Original decompilation identifier: \c func_800409CC (0x800409CC).
void voiceRecognitionPoll(void);

/// Check whether a recognition read is active.
/// \returns Nonzero while a read is active, or zero otherwise.
/// \note Original decompilation identifier: \c func_80040C80 (0x80040C80).
s32 voiceRecognitionIsReadActive(void);

/// Get the recorded recognition-device availability state.
/// \returns The recorded VoiceRecognitionDeviceState value.
/// \note Original decompilation identifier: \c func_80040DC8 (0x80040DC8).
s32 voiceRecognitionGetDeviceState(void);

/// Set the microphone amplifier on a ready recognition device.
/// \param flag VOICE_AMPLIFIER_ON to enable the amplifier, or VOICE_AMPLIFIER_OFF to disable it.
/// \returns Zero on success, minus one when unavailable, or a device error code.
/// \note Original decompilation identifier: \c func_80040E64 (0x80040E64).
s32 voiceRecognitionSetAmplifier(u8 flag);

/// Get the recorded microphone amplifier state.
/// \returns Nonzero when enabled, or zero when disabled.
/// \note Original decompilation identifier: \c func_80040F10 (0x80040F10).
s32 voiceRecognitionIsAmplifierEnabled(void);

/// Consume the pending recognition-result notification.
/// \param out Destination for the retained result, unchanged when none is pending; null discards the notification.
/// \returns First dictionary-local answer index, or -1 when no result is pending; check the answer count before lookup.
/// \note Original decompilation identifier: \c func_80040BA0 (0x80040BA0).
s32 voiceRecognitionTakeResult(OSVoiceData *out);

/// Get the most recently recorded device recognition command status.
/// \returns Device command status.
/// \note Original decompilation identifier: \c func_80040C70 (0x80040C70).
s32 voiceRecognitionGetCommandStatus(void);

typedef char VoiceDictionarySizeCheck[(sizeof(VoiceDictionary) == 8) ? 1 : -1];
typedef char VoiceWordSizeCheck[(sizeof(VoiceWord) == 0x10) ? 1 : -1];

#define VOICE_RECOGNITION_OFFSET_CHECK(name, type, member, offset)                                                     \
    typedef char name[(((u32) & ((type *)0)->member) == (offset)) ? 1 : -1]
VOICE_RECOGNITION_OFFSET_CHECK(VoiceDictionaryWordIdsOffsetCheck, VoiceDictionary, wordIds, 0x4);
VOICE_RECOGNITION_OFFSET_CHECK(VoiceWordNameOffsetCheck, VoiceWord, name, 0x0);
VOICE_RECOGNITION_OFFSET_CHECK(VoiceWordTextOffsetCheck, VoiceWord, text, 0x4);
VOICE_RECOGNITION_OFFSET_CHECK(VoiceWordCommandTypeOffsetCheck, VoiceWord, commandType, 0x8);
VOICE_RECOGNITION_OFFSET_CHECK(VoiceWordCommandValueOffsetCheck, VoiceWord, commandValue, 0xC);
#undef VOICE_RECOGNITION_OFFSET_CHECK

#endif
