/// \file
/// Defines MIDI status classification and variable-length quantity bit encodings.

#ifndef MIDI_ENCODING_H
#define MIDI_ENCODING_H

/// Message classification shared by the MIDI stream and sequence consumers.
enum {
    MIDI_CHANNEL_COUNT = 16, ///< Number of channel-voice channels.
    MIDI_STATUS_FLAG = 0x80, ///< Distinguishes a status byte from a data byte.
    MIDI_SYSTEM_CLASS = 0xF0 ///< Masked status family, including SysEx and other system messages.
};

/// Lowest controller data value treated as enabled by sustain consumers.
enum { MIDI_SUSTAIN_ON_MIN = 64 };

/// Coarse/fine representation used by fourteen-bit channel controls.
enum {
    MIDI_WIDE_PART_BITS = 7,     ///< Width of either control data part.
    MIDI_WIDE_HIGH_MASK = 0x3F80 ///< Upper data part within the fourteen-bit value.
};

/// Standard controllers consumed by the engine without corresponding SDK names.
/// \see https://midi.org/midi-1-0-control-change-messages
typedef enum MidiController {
    MIDI_CTRL_BANK_MSB = 0,
    MIDI_CTRL_MODULATION = 1,
    MIDI_CTRL_PORTAMENTO_TIME = 5,
    MIDI_CTRL_DATA_ENTRY_MSB = 6,
    MIDI_CTRL_EXPRESSION = 11,
    MIDI_CTRL_BANK_LSB = 32,
    MIDI_CTRL_DATA_ENTRY_LSB = 38,
    MIDI_CTRL_PAN_LSB = 42,
    MIDI_CTRL_PORTAMENTO_ENABLE = 65,
    MIDI_CTRL_PORTAMENTO_SOURCE_NOTE = 84,
    MIDI_CTRL_RPN_LSB = 100,
    MIDI_CTRL_RPN_MSB = 101,
    MIDI_CTRL_ALL_SOUND_OFF = 120,
    MIDI_CTRL_RESET_CONTROLLERS = 121,
    MIDI_CTRL_ALL_NOTES_OFF = 123
} MidiController;

/// Registered parameter accepted by this engine's data-entry handler.
enum { MIDI_RPN_PITCH_BEND_SENSITIVITY = 0 };

/// Variable-length quantity encoding, separate from MIDI message status.
enum {
    MIDI_VLQ_CONTINUE = 0x80,     ///< Another quantity group follows.
    MIDI_VLQ_PAYLOAD_MASK = 0x7F, ///< Seven payload bits in one group.
    MIDI_VLQ_GROUP_BITS = 7       ///< Payload width per group.
};

#endif
