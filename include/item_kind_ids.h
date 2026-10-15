/// \file
/// Names established item-kind identifiers used by gameplay command routing.

#ifndef ITEM_KIND_IDS_H
#define ITEM_KIND_IDS_H

/// Selected identities established by the matching item catalog's display names.
/// Other catalog identifiers remain numeric until their consumers are reviewed.
typedef enum ItemKindId {
    ITEM_KIND_MEGAPHONE = 0x51,    ///< Catalog names both variants "megaphone".
    ITEM_KIND_CARDBOARD_BOX = 0x8A ///< Catalog names the variants "box" and "cardboard box".
} ItemKindId;

#endif
