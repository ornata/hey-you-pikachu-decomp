/// \file
/// Defines recognition vocabulary, word records, and voice recognition interfaces.

#ifndef VOICE_RECOGNITION_H
#define VOICE_RECOGNITION_H

#include <ultra64.h>

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
s32 voiceDictionaryGetWordId(u16 index);

/// Get the display name of an entry in the selected vocabulary.
/// \param index Dictionary-local word index.
/// \returns Borrowed name, or an empty string for an invalid or unavailable entry.
/// \note Original decompilation identifier: \c func_80040C8C (0x80040C8C).
char *voiceDictionaryGetWordName(u16 index);

/// Get the gameplay message category associated with a vocabulary entry.
/// \param index Dictionary-local word index.
/// \returns Command category, or -1 for an invalid or unavailable entry.
s32 voiceDictionaryGetCommandType(u16 index);

/// Get the gameplay command argument associated with a vocabulary entry.
/// \param index Dictionary-local word index.
/// \returns Command value, or -1 for an invalid or unavailable entry.
s32 voiceDictionaryGetCommandValue(u16 index);

/// Consume the pending recognition-result notification.
/// \param out Destination for the retained result, or null to discard the notification.
/// \returns First answer index, or -1 when no result is pending; callers must also check the answer count.
s32 voiceRecognitionTakeResult(OSVoiceData *out);

/// Get the most recently recorded device recognition command status.
/// \returns Device command status.
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
