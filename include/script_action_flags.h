/// \file
/// Defines recovered meanings in the compact creature action status exposed to actor scripts.

#ifndef SCRIPT_ACTION_FLAGS_H
#define SCRIPT_ACTION_FLAGS_H

/// Known projections of the active action's attributes into script status.
/// Actor behavior inhibition flags are not included. Other status bits remain
/// numeric until their behavioral meanings are established.
typedef enum ScriptActionFlag {
    SCRIPT_ACTION_HIGH_UPWARD_MOTION_LIMIT = 0x10,  ///< The action selects the higher upward-motion limit.
    SCRIPT_ACTION_BYPASS_TERRAIN_AND_GRAVITY = 0x20 ///< The action requests terrain/gravity bypass.
} ScriptActionFlag;

#endif
