/// \file
/// Defines the owned rotation-speed state shared by voice-reactive spinner callbacks and script queries.

#ifndef VOICE_REACTIVE_SPINNER_TYPES_H
#define VOICE_REACTIVE_SPINNER_TYPES_H

#include <ultra64.h>

/// Complete four-byte private state allocated for a voice-reactive spinner.
/// The runtime object owns this allocation and releases it with its state destructor.
typedef struct VoiceReactiveSpinnerState {
    f32 rotationSpeed; ///< Retained degrees per second; decay can leave a nonpositive value.
} VoiceReactiveSpinnerState;

typedef char VoiceReactiveSpinnerStateSizeCheck[(sizeof(VoiceReactiveSpinnerState) == 4) ? 1 : -1];

#endif
