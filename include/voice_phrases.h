/// \file
/// Names recognized Pikachu-like phrase values carried by voice messages.
/// These names describe vocabulary labels, not translations or guaranteed reactions.

#ifndef VOICE_PHRASES_H
#define VOICE_PHRASES_H

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
