/// \file
/// Names item-noun group requests carried by recognized command messages.

#ifndef VOICE_ITEM_SELECTOR_H
#define VOICE_ITEM_SELECTOR_H

/// Message category carrying dictionary-defined item nouns in argument zero.
enum { VOICE_ITEM_NOUN_MESSAGE_TYPE = 6 };

/// Authored item-noun families whose selectors do not denote individual item kinds.
/// These values are distinct from ItemCategoryId and cannot be used for item creation.
/// Names describe vocabulary requests, not successful search or membership results.
/// Other noun values and dictionary-specific aliases retain their numeric encoding.
typedef enum VoiceItemNounGroupValue {
    VOICE_ITEM_NOUN_GROUP_FRUIT = 166,
    VOICE_ITEM_NOUN_GROUP_APPLE = 167,
    VOICE_ITEM_NOUN_GROUP_NUT = 169,
    VOICE_ITEM_NOUN_GROUP_FLOWER = 170,
    VOICE_ITEM_NOUN_GROUP_ACORN = 171,
    VOICE_ITEM_NOUN_GROUP_MUSHROOM = 172,
    VOICE_ITEM_NOUN_GROUP_BALLOON = 173,
    VOICE_ITEM_NOUN_GROUP_COIN = 174,
    VOICE_ITEM_NOUN_GROUP_JEWEL = 175,
    VOICE_ITEM_NOUN_GROUP_MARBLE = 176,
    VOICE_ITEM_NOUN_GROUP_CRATE_OR_CARDBOARD_BOX = 179,
} VoiceItemNounGroupValue;

#endif
