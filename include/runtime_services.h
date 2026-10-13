/// \file
/// Declares runtime thread startup, prompt drawing and UI pulse state.

#ifndef RUNTIME_SERVICES_H
#define RUNTIME_SERVICES_H

#include <ultra64.h>

/// Get the current opacity of the shared repeating UI pulse.
/// \pre Pulse age in milliseconds must be representable as a signed 32-bit integer.
/// \returns Opacity from zero through 255.
/// \note Original decompilation identifier: \c func_8000B134 (0x8000B134).
s32 runtimeGetUiPulseAlpha(void);

/// Restart the shared UI pulse timer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B124 (0x8000B124).
void runtimeResetUiPulse(void);

/// Create and start the main runtime thread.
/// \param scheduler Scheduler context that must remain valid for the lifetime of the started thread.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C040 (0x8000C040).
void runtimeStartMainThread(void *scheduler);

/// Draw the active runtime prompt panel with a pulsing final line.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C094 (0x8000C094).
void runtimeDrawPromptPanel(void);

#endif
