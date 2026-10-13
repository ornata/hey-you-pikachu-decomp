/// \file
/// Defines game audio output settings and category-wide playback controls.

#ifndef AUDIO_CONTROL_H
#define AUDIO_CONTROL_H

#include <ultra64.h>

/// Playback categories accepted by category-wide audio operations.
typedef enum AudioCategory {
    AUDIO_CATEGORY_SOUND_EFFECTS = 1, ///< Sound-effect playback slots.
    AUDIO_CATEGORY_SEQUENCES = 2,     ///< Music and other sequence playback slots.
    AUDIO_CATEGORY_ALL = 3            ///< Both playback categories.
} AudioCategory;

/// Apply the output setting to initialized sequence and sound-effect services.
/// \param mode One selects mono; two selects stereo.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001FA28 (0x8001FA28).
void audioSetOutputMode(s32 mode);

/// Read the stored audio output setting.
/// \returns The last selected output mode.
/// \note Original decompilation identifier: \c func_8001FA6C (0x8001FA6C).
s32 audioGetOutputMode(void);

/// Stop sound effects immediately and request sequence fades in selected categories.
/// \param categories AudioCategory bit mask; zero selects no playback.
/// \param fadeSeconds Nonnegative sequence fade duration in seconds.
/// \returns Nothing; sequence fades may remain active after return.
/// \note Original decompilation identifier: \c func_8001FC10 (0x8001FC10).
void audioStopCategories(s32 categories, f64 fadeSeconds);

/// Count active playback slots in selected categories of initialized audio services.
/// \param categories AudioCategory bit mask; zero selects no playback.
/// \returns Combined active slot count, including sequences still fading out.
/// \note Original decompilation identifier: \c func_8001FCA4 (0x8001FCA4).
s32 audioCountActive(s32 categories);

/// Set master levels for selected categories of initialized audio services.
/// \param categories AudioCategory bit mask; zero selects no playback.
/// \param volume Category-specific level: sequences use volume divided by 65535;
/// sound effects receive the integer level directly.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002006C (0x8002006C).
void audioSetCategoryVolume(s32 categories, s32 volume);

#endif
