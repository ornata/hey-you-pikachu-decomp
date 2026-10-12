/// \file
/// Declares borrowed item-kind names, class labels and interaction descriptions.

#ifndef ITEM_TEXT_H
#define ITEM_TEXT_H

#include <ultra64.h>

/// Look up an item kind display name with the requested naming preference.
/// \param kind Item kind identifier from zero through 165.
/// \param nameType Zero prefers the alternate name and falls back to the default; one requests the default
/// name.
/// \returns A borrowed persistent name string, or null for an unsupported preference or missing name.
/// \note Original decompilation identifier: \c func_80114E74 (0x80114E74).
char *itemGetDisplayName(s32 kind, s32 nameType);

/// Look up the label suffix used to identify an item kind class.
/// \param kind Item kind identifier from zero through 165.
/// \returns A borrowed persistent item-label suffix, an empty string for classes without a suffix, or null for an
/// unspecified or invalid class.
/// \note Original decompilation identifier: \c func_80114EE8 (0x80114EE8).
u8 *itemGetClassLabelSuffix(s32 kind);

/// Look up an item kind help text with the requested description preference.
/// \param kind Item kind identifier from zero through 165.
/// \param helpType Zero prefers alternate help text and falls back to the default; one requests the default
/// help text.
/// \returns A borrowed persistent help string, or null for an unsupported preference or missing text.
/// \note Original decompilation identifier: \c func_80114F4C (0x80114F4C).
char *itemGetHelpText(s32 kind, s32 helpType);

#endif
