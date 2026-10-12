/// \file
/// Declares borrowed actor-kind inspection names and descriptions.

#ifndef ACTOR_TEXT_H
#define ACTOR_TEXT_H

#include <ultra64.h>

/// Get an actor kind's display name for inspection text.
/// \param kind Valid actor kind whose text is requested.
/// \param useDefault Nonzero selects the default name; zero prefers the alternate name when available.
/// \returns Borrowed selected name string, or null when no name is configured.
/// \note Original decompilation identifier: \c func_8001E680 (0x8001E680).
char *actorConfigGetDisplayName(s32 kind, s32 useDefault);

/// Get an actor kind's description for inspection text.
/// \param kind Valid actor kind whose text is requested.
/// \param useDefault Nonzero selects the default description; zero prefers the alternate description when available.
/// \returns Borrowed selected description string, or null when no description is configured.
/// \note Original decompilation identifier: \c func_8001E6D8 (0x8001E6D8).
char *actorConfigGetDescription(s32 kind, s32 useDefault);

#endif
