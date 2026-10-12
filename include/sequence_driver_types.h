/// \file
/// Defines sequence-driver registration and the caller-supplied voice pool.

#ifndef SEQUENCE_DRIVER_TYPES_H
#define SEQUENCE_DRIVER_TYPES_H

#include <ultra64.h>

/// Driver-owned voice storage linked through active and free voice lists.
typedef struct Voice50 {
    struct Voice50 *next;     ///< Next active or free voice.
    struct Voice50 *previous; ///< Previous active voice, or NULL at the head.
    u8 unknown8[0x48];        ///< Retained voice playback storage; detailed view remains private.
} Voice50;

/// Synthesizer registration and shared sequence playback controls.
typedef struct VoicePlayer {
    ALPlayer node;   ///< Synthesizer callback registration; valid while registered.
    s32 synth;       ///< Borrowed synthesizer context address in the target ABI.
    s32 active;      ///< First registered sequence address, or zero.
    u8 bank;         ///< Output mode; one selects mono panning.
    u8 maxVoices;    ///< Number of records supplied in the voice pool.
    Voice50 *voices; ///< First available voice in caller-supplied storage.
    u8 vol;          ///< Master sequence volume on a zero-to-255 scale.
} VoicePlayer;

typedef char Voice50SizeCheck[(sizeof(Voice50) == 0x50) ? 1 : -1];
typedef char VoicePlayerSizeCheck[(sizeof(VoicePlayer) == 0x28) ? 1 : -1];
typedef char VoicePlayerPoolOffsetCheck[((u32) & ((VoicePlayer *)0)->voices == 0x20) ? 1 : -1];

#endif
