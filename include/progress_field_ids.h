/// \file
/// Defines recovered identifiers in the saved progress-field catalog.

#ifndef PROGRESS_FIELD_IDS_H
#define PROGRESS_FIELD_IDS_H

/// Saved progress selectors with established producers and consumers.
enum {
    PROGRESS_FIELD_CHAPTER = 0,             ///< Current story chapter.
    PROGRESS_FIELD_PROFILE_POINTS = 1,      ///< Accumulated player profile points.
    PROGRESS_FIELD_FRIENDSHIP_LEVEL = 2,    ///< Player friendship level.
    PROGRESS_FIELD_VOICE_MESSAGE_COUNT = 3, ///< Player voice-message count.
    PROGRESS_FIELD_PLAY_TIME_SECONDS = 4,   ///< Credited whole seconds of runtime play time.
    PROGRESS_FIELD_SCRIPT_TIME_SECONDS = 5, ///< Credited whole seconds from the active actor's script timer.
    PROGRESS_FIELD_DAYS_WITH_PIKACHU = 6,   ///< Completed visits displayed as days spent with Pikachu.
    PROGRESS_FIELD_LOOK_FORWARD_UP = 7,     ///< One makes forward stick input look up.
    PROGRESS_FIELD_COMPANION_TOGGLE_FOLLOW =
        8,                          ///< One enables follow movement with companion control; zero selects view tracking.
    PROGRESS_FIELD_MONO_OUTPUT = 9, ///< One selects mono output; zero selects stereo.
    PROGRESS_FIELD_MUSIC_VOLUME = 10,                  ///< Saved sequence/music volume from zero through fifteen.
    PROGRESS_FIELD_SOUND_EFFECT_VOLUME = 11,           ///< Saved sound-effect volume from zero through fifteen.
    PROGRESS_FIELD_CHASE_STAGE = 12,                   ///< Highest saved player chase stage.
    PROGRESS_FIELD_GAME_CLEARED = 14,                  ///< Saved game-clear flag enabling post-clear progression.
    PROGRESS_FIELD_TOGEPI_HATCHED = 22,                ///< Togepi egg has completed its transformation into Togepi.
    PROGRESS_FIELD_TOGEPI_EGG_GROWTH_STAGE = 23,       ///< Egg growth advanced by qualifying Abra-shop item turn-ins.
    PROGRESS_FIELD_OCHRE_ENTRANCE_RESIDENT_STAGE = 86, ///< Zero through three selects additional entrance residents.
    PROGRESS_FIELD_FRONT_YARD_DOOR_ENABLED = 160, ///< Bedroom passage to the front yard replaces its initial proxy.
    PROGRESS_FIELD_QUIZ_VARIANT_UNLOCK_STAGE =
        162 ///< Zero: picture quiz; one: shadow unlocked; two: strobe-shadow unlocked.
};

#endif
