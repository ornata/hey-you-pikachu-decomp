/// \file
/// Names established item-kind identifiers used by command routing and cooking results.

#ifndef ITEM_KIND_IDS_H
#define ITEM_KIND_IDS_H

/// Selected identities established by the matching catalog's names, help and consumers.
/// Quality adjectives retain catalog labels without guaranteeing recipe success.
/// Other catalog identifiers remain numeric until their consumers are reviewed.
typedef enum ItemKindId {
    ITEM_KIND_MEGAPHONE = 0x51, ///< Catalog names both variants "megaphone".
    ITEM_KIND_TASTY_STEW = 106,
    ITEM_KIND_TYPICAL_STEW = 107,
    ITEM_KIND_WEIRD_STEW = 108,
    ITEM_KIND_TASTY_STIR_FRY = 109,
    ITEM_KIND_TYPICAL_STIR_FRY = 110,
    ITEM_KIND_WEIRD_STIR_FRY = 111,
    ITEM_KIND_TASTY_SOUP = 112,
    ITEM_KIND_TYPICAL_SOUP = 113,
    ITEM_KIND_WEIRD_SOUP = 114,
    ITEM_KIND_TASTY_GOULASH = 115,
    ITEM_KIND_AVERAGE_GOULASH = 116,
    ITEM_KIND_WEIRD_GOULASH = 117,
    ITEM_KIND_TASTY_COOKIE = 118,
    ITEM_KIND_AVERAGE_COOKIE = 119,
    ITEM_KIND_WEIRD_COOKIE = 120,
    ITEM_KIND_MYSTERY_SOUP = 121,
    ITEM_KIND_CARDBOARD_BOX = 0x8A ///< Catalog names the variants "box" and "cardboard box".
} ItemKindId;

#endif
