/// \file
/// Defines recovered gameplay, menu and transition modes of the runtime dispatcher.

#ifndef RUNTIME_MODE_H
#define RUNTIME_MODE_H

/// Modes with established update, render and transition consumers.
/// Values 9 and 10 remain unresolved and have no retained dispatcher cases.
/// Runtime storage and requests retain signed target words without validation.
typedef enum RuntimeGameMode {
    GAME_MODE_GAMEPLAY = 0,             ///< Active player and actor gameplay.
    GAME_MODE_TOOLBOX = 1,              ///< Toolbox menu.
    GAME_MODE_SYSTEM_MENU = 2,          ///< System menu.
    GAME_MODE_TEXT_WINDOW = 3,          ///< Text-window interaction.
    GAME_MODE_AREA_CARD = 4,            ///< Current-area card presentation.
    GAME_MODE_COLLECTION_RESULTS = 5,   ///< Collection score and chapter results.
    GAME_MODE_FADE_TO_WORLD_MAP = 6,    ///< Fade and stop audio before opening the whole map.
    GAME_MODE_FADE_TO_SAVE_PROMPT = 7,  ///< Fade before displaying save confirmation.
    GAME_MODE_WORLD_MAP = 8,            ///< Whole-map menu.
    GAME_MODE_FADE_TO_AREA_LOAD = 11,   ///< Fade before requesting the runtime program reload.
    GAME_MODE_FADE_TO_COLLECTION = 12,  ///< Fade before presenting completed-visit results.
    GAME_MODE_EYE_CATCH = 13,           ///< Eye-catch interlude.
    GAME_MODE_SAVE_CONFIRMATION = 14,   ///< Confirmation prompt before saving.
    GAME_MODE_SAVE_RECORD = 15,         ///< Progress-record write and result selection.
    GAME_MODE_SAVE_CONTROL = 16,        ///< Save-control write before the progress record.
    GAME_MODE_SAVE_ACKNOWLEDGMENT = 17, ///< Success/failure prompt awaiting its acknowledgment button.
    GAME_MODE_TITLE = 18                ///< Startup checks, title and save-file menu.
} RuntimeGameMode;

#endif
