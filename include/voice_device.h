/// \file
/// Declares voice-device connection and dictionary upload contracts.

#ifndef VOICE_DEVICE_H
#define VOICE_DEVICE_H

#include <ultra64.h>

/// Connect a voice handle to a controller port.
/// \param queue Serial-interface queue retained by the handle and valid for its lifetime.
/// \param handle Writable handle receiving connection state.
/// \param channel Controller port from zero through three.
/// \returns Zero on success, or a controller or voice-device error code.
/// \note Original decompilation identifier: \c func_8004C8A0 (0x8004C8A0).
s32 voiceDeviceInitialize(OSMesgQueue *queue, OSVoiceHandle *handle, s32 channel);

/// Register an encoded word in the device dictionary.
/// \param handle Initialized voice handle.
/// \param word Borrowed zero-terminated target-encoded 16-bit text, with at most 37 nonzero code units.
/// \returns Zero on success, or a controller or word-registration error code.
/// \note Original decompilation identifier: \c func_8004CAAC (0x8004CAAC).
s32 voiceDeviceRegisterWord(OSVoiceHandle *handle, u8 *word);

/// Select the device dictionary word mask.
/// \param handle Initialized voice handle.
/// \param mask Borrowed mask with readable storage rounded up to an even byte length.
/// \param bytes Mask length from zero through 38 bytes.
/// \returns Final device operation status: zero on success, or an error code.
/// \note Original decompilation identifier: \c func_8004CC5C (0x8004CC5C).
s32 voiceDeviceSetDictionaryMask(OSVoiceHandle *handle, u8 *mask, s32 bytes);

/// Clear the device dictionary and set its replacement capacity.
/// \param handle Initialized voice handle.
/// \param wordCount Number of words in the replacement dictionary.
/// \returns Zero on success, or a controller or voice-device error code.
/// \note Original decompilation identifier: \c func_8004CDA4 (0x8004CDA4).
s32 voiceDeviceClearDictionary(OSVoiceHandle *handle, u16 wordCount);

#endif
