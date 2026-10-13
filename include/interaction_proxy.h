/// \file
/// Creates descriptive and consumable targets for player interaction.

#ifndef INTERACTION_PROXY_H
#define INTERACTION_PROXY_H

#include <ultra64.h>

/// Create and register a descriptive interaction target at a world position.
/// \param position Three world coordinates in X, Y, Z order, read during this call.
/// \param context Caller context; consumable proxies use this as remaining uses.
/// \param interactionResult Classification result supplied when the target is selected.
/// \param metadata Caller metadata included in consumption messages.
/// \param description Borrowed null-terminated help text that must outlive the proxy
/// and fit the interaction-description buffer.
/// \returns Registered proxy whose lifetime is managed by the object system.
/// \note Original identifier: \c func_80112940 (0x80112940).
u8 *interactionProxyCreate(f32 *position, s32 context, s32 interactionResult, s32 metadata, char *description);

#endif
