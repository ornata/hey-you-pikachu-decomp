/// \file
/// Defines selectors for borrowed game event and controller-command messages.

#ifndef GAME_MESSAGE_H
#define GAME_MESSAGE_H

/// Selector stored in the first sixteen bits of a game message record.
/// These names do not define record size: scheduler events and device requests
/// have distinct payloads and lifetimes. Keep wire/storage fields sixteen bits.
/// Queues borrow message storage through consumption or the request acknowledgment.
typedef enum GameMessageKind {
    GAME_MESSAGE_RETRACE = 1,           ///< Scheduler display-retrace notification.
    GAME_MESSAGE_FRAME_COMPLETE = 2,    ///< Completion of a frame-presentation graphics task.
    GAME_MESSAGE_PRE_NMI = 3,           ///< Scheduler notification of impending reset.
    GAME_MESSAGE_GRAPHICS_COMPLETE = 4, ///< Completion of a graphics task without frame presentation.
    GAME_MESSAGE_EEPROM = 5,            ///< EEPROM transfer setup request.
    GAME_MESSAGE_MICROPHONE = 6,        ///< Microphone configuration request.
    GAME_MESSAGE_RUMBLE = 7             ///< Controller rumble request.
} GameMessageKind;

#endif
