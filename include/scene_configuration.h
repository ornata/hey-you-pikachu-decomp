/// \file
/// Declares the current loaded area's borrowed configuration storage.

#ifndef SCENE_CONFIGURATION_H
#define SCENE_CONFIGURATION_H

struct SceneConfigurationPrefix;

/// Configuration selected by area activation; require a loaded area before dereferencing it.
/// SceneConfigurationPrefix describes its established common fields, not the complete allocation.
/// Resources and callbacks obtained from the configuration remain borrowed from their owning area.
/// Do not retain them after that area's overlay or associated resources are replaced.
/// \note Consumers of area-specific extensions and matching views use explicit byte addresses.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern struct SceneConfigurationPrefix *gCurrentAreaConfiguration;

#endif
