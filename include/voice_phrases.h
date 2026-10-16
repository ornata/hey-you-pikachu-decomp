/// \file
/// Names recognized general-command and Pikachu-like phrase selectors carried by voice messages.
/// These names describe vocabulary labels, not translations or guaranteed reactions.

#ifndef VOICE_PHRASES_H
#define VOICE_PHRASES_H

/// Message category whose argument 0 can carry a general command or Pikachu-like phrase.
/// The same payload may be posted by scripts; this category does not prove voice origin.
enum { VOICE_GENERAL_COMMAND_MESSAGE_TYPE = 2 };

/// Authored command-label families shared by the retained category-two vocabulary.
/// Values are message arguments, not dictionary indices or global word IDs.
/// Names describe vocabulary families, not guaranteed actions or reactions.
/// Values one and two remain distinct despite overlapping recognition labels.
/// Mixed-purpose values zero, four and eight retain their numeric representation.
typedef enum VoiceGeneralCommandValue {
    VOICE_GENERAL_COMMAND_HEY_OR_COME_HERE = 1,
    VOICE_GENERAL_COMMAND_THIS_WAY_OR_COME_HERE = 2,
    VOICE_GENERAL_COMMAND_NEVER_MIND = 3,
    VOICE_GENERAL_COMMAND_TAG_YOURE_IT = 5,
    VOICE_GENERAL_COMMAND_HELLO = 6,
    VOICE_GENERAL_COMMAND_FAREWELL = 7,
    VOICE_GENERAL_COMMAND_GO_AWAY = 9,
    VOICE_GENERAL_COMMAND_GOOD_MORNING = 11,
    VOICE_GENERAL_COMMAND_GOOD_NIGHT = 12,
    VOICE_GENERAL_COMMAND_THANK_YOU = 13,
    VOICE_GENERAL_COMMAND_SORRY = 14,
    VOICE_GENERAL_COMMAND_I_CHOOSE_YOU = 15,
    VOICE_GENERAL_COMMAND_WAKE_UP = 16,
    VOICE_GENERAL_COMMAND_DANCE = 17,
    VOICE_GENERAL_COMMAND_I_LOVE_YOU = 18,
    VOICE_GENERAL_COMMAND_HANG_ON = 19,
    VOICE_GENERAL_COMMAND_STATUS_QUESTION = 20,
    VOICE_GENERAL_COMMAND_WHAT_IS_IT = 21,
    VOICE_GENERAL_COMMAND_LOOK_OUT = 33,
    VOICE_GENERAL_COMMAND_DO_SOMETHING_OR_LISTEN = 34,
    VOICE_GENERAL_COMMAND_ROCK = 35,
    VOICE_GENERAL_COMMAND_SCISSORS = 36,
    VOICE_GENERAL_COMMAND_PAPER = 37
} VoiceGeneralCommandValue;

/// Message category whose argument 0 can carry a VoicePikachuPhraseValue.
/// This category also carries other dictionary-defined command values.
enum { VOICE_PIKACHU_PHRASE_MESSAGE_TYPE = 2 };

/// Recognized phrase selectors shared by the retained dictionaries containing them.
/// Values identify command arguments, not dictionary entry indices or global word IDs.
/// Interpret them only with VOICE_PIKACHU_PHRASE_MESSAGE_TYPE; other categories
/// have their own value domains. Alternate word records may select the same phrase.
typedef enum VoicePikachuPhraseValue {
    VOICE_PHRASE_PIKA_PIKACHU = 23, ///< Recognized "pika-pikachu" phrase.
    VOICE_PHRASE_PIKA_PIKA_PI = 24, ///< Recognized "pika-pika-pi" phrase.
    VOICE_PHRASE_PI_PIKACHU = 25,   ///< Recognized "pi-pikachu" phrase.
    VOICE_PHRASE_PI_PIKA_PI = 26,   ///< Recognized "pi-pika-pi" phrase.
    VOICE_PHRASE_PIKKA = 27,        ///< Recognized "pikka" phrase.
    VOICE_PHRASE_PIKA = 28,         ///< Recognized "pika" phrase.
    VOICE_PHRASE_PIKAPIKA = 29,     ///< Recognized "pikapika" phrase.
    VOICE_PHRASE_PI_KA = 30,        ///< Recognized "pi-ka" phrase.
    VOICE_PHRASE_PI_KA_KA = 31,     ///< Recognized "pi-ka-ka-" phrase.
    VOICE_PHRASE_PI_KAKAPI = 32     ///< Recognized "pi-kakapi-" phrase.
} VoicePikachuPhraseValue;

#endif
