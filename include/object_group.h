/// \file
/// Defines the independent draw and update group bits stored in runtime objects.

#ifndef OBJECT_GROUP_H
#define OBJECT_GROUP_H

/// Independent group-membership bits in the retained object flags.
/// Draw and update selectors are zero for the default group or the corresponding
/// raw bit value for the alternate group; they are not ordinal group indices.
typedef enum ObjectGroupFlag {
    OBJECT_DRAW_GROUP_ALTERNATE = 0x2000,  ///< Alternate drawing group.
    OBJECT_UPDATE_GROUP_ALTERNATE = 0x4000 ///< Alternate update group.
} ObjectGroupFlag;

#endif
