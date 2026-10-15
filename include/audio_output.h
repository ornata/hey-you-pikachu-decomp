/// \file
/// Defines the game's mono and stereo output configuration values.

#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

/// Supported output settings forwarded to the sequence and sound-effect drivers.
/// Other raw values retain each driver's existing behavior; storage widths are unchanged.
typedef enum AudioOutputMode {
    AUDIO_OUTPUT_MONO = 1,  ///< Centered mono output.
    AUDIO_OUTPUT_STEREO = 2 ///< Individually positioned stereo output.
} AudioOutputMode;

#endif
