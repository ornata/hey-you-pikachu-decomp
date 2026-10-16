/// \file
/// Names established membership categories in the item-kind range catalog.

#ifndef ITEM_CATEGORY_H
#define ITEM_CATEGORY_H

/// Selected category indices accepted by itemKindIsInCategory.
/// These selectors are distinct from individual item kinds and voice noun groups.
/// Unnamed categories retain their numeric values; the complete valid range is zero through eighteen.
typedef enum ItemCategoryId {
    ITEM_CATEGORY_ACORN = 5,            ///< Acorn family, kinds thirty through thirty-six.
    ITEM_CATEGORY_GENERIC_MUSHROOM = 7, ///< Generic mushrooms, kinds forty-five through forty-seven.
    ITEM_CATEGORY_BALLOON = 10,         ///< Balloons, kinds sixty-one through sixty-five.
    ITEM_CATEGORY_CRATE = 17            ///< Crates, kinds 124 through 137.
} ItemCategoryId;

#endif
