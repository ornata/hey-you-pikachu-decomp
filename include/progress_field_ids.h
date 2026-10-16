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
    PROGRESS_FIELD_MUSIC_VOLUME = 10,        ///< Saved sequence/music volume from zero through fifteen.
    PROGRESS_FIELD_SOUND_EFFECT_VOLUME = 11, ///< Saved sound-effect volume from zero through fifteen.
    PROGRESS_FIELD_CHASE_STAGE = 12,         ///< Highest saved player chase stage.
    PROGRESS_FIELD_EARLY_VISIT_COUNT = 13,   ///< Chapter-three visits used for early story progression.
    PROGRESS_FIELD_GAME_CLEARED = 14,        ///< Saved game-clear flag enabling post-clear progression.
    PROGRESS_FIELD_ERRAND_BOX_INSTRUCTIONS_RECORDED =
        16, ///< First shopping-errand cardboard-box instructions recorded.
    PROGRESS_FIELD_PIKACHU_DISAPPEARANCE_ACTIVE =
        17,                                     ///< Pikachu disappearance episode selected and awaiting resolution.
    PROGRESS_FIELD_SHOPPING_ERRAND_ACTIVE = 18, ///< Shopping-errand visit and its alternate controls selected.
    PROGRESS_FIELD_DISAPPEARANCE_TRIGGER_COUNTER = 19, ///< Weighted visit counter influencing disappearance selection.
    PROGRESS_FIELD_ERRAND_TRIGGER_COUNTER = 20, ///< Completed-visit counter influencing shopping-errand selection.
    PROGRESS_FIELD_TOGEPI_EGG_FOUND = 21, ///< Forest egg encounter recorded; enables the unhatched egg at Abra shop.
    PROGRESS_FIELD_TOGEPI_HATCHED = 22,   ///< Togepi egg has completed its transformation into Togepi.
    PROGRESS_FIELD_TOGEPI_EGG_GROWTH_STAGE = 23, ///< Egg growth advanced by qualifying Abra-shop item turn-ins.
    PROGRESS_FIELD_TOGEPI_HATCH_SIZE = 24, ///< Normalized Togepi size captured from voice-message progress at hatching.
    PROGRESS_FIELD_CUPCAKE_ACQUIRED = 27,  ///< Cupcake acquisition recorded; does not imply current possession.
    PROGRESS_FIELD_BEACH_BALL_ACQUIRED = 28, ///< Beach ball acquisition recorded; does not imply current possession.
    PROGRESS_FIELD_FLYING_ACORN_ACQUIRED =
        29, ///< Flying acorn acquisition recorded; does not imply current possession.
    PROGRESS_FIELD_FISHING_HOOK_ACQUIRED =
        30,                                  ///< Fishing hook acquisition recorded; does not imply current possession.
    PROGRESS_FIELD_LUCKY_HOOK_UNLOCKED = 31, ///< Lucky hook and expanded fishing residents available.
    PROGRESS_FIELD_WATERING_CAN_ACQUIRED =
        32, ///< Watering can acquisition recorded; does not imply current possession.
    PROGRESS_FIELD_WATERING_JUG_ACQUIRED =
        33,                                 ///< Watering jug acquisition recorded; does not imply current possession.
    PROGRESS_FIELD_HARMONICA_ACQUIRED = 34, ///< Harmonica acquisition recorded; does not imply current possession.
    PROGRESS_FIELD_MEGAPHONE_ACQUIRED = 35, ///< Megaphone acquisition recorded; does not imply current possession.
    PROGRESS_FIELD_OCHRE_ENTRANCE_RESIDENT_STAGE = 86, ///< Zero through three selects additional entrance residents.
    PROGRESS_FIELD_PIKACHU_DISAPPEARANCE_EPISODES_COMPLETED =
        93, ///< Completed disappearance visits selecting first or repeat search narration.
    PROGRESS_FIELD_VILEPLUME_MEADOW_VISIT_PENDING =
        100,                                     ///< Next meadow visit uses five Vileplume; consumed on arrival.
    PROGRESS_FIELD_ODDISH_GROWTH_PROGRESS = 111, ///< Special meadow Oddish growth and scale progress.
    PROGRESS_FIELD_LUCKY_HOOK_EXPLANATION_RECORDED = 154, ///< Fishing script has processed the lucky-hook explanation.
    PROGRESS_FIELD_FISHING_PROFICIENCY = 155,             ///< Adaptive fishing score, zero through fifteen.
    PROGRESS_FIELD_FISHING_TOTAL_CATCH_COUNT = 156,       ///< Cumulative caught Pokemon across fishing visits.
    PROGRESS_FIELD_FISHING_BEST_DAILY_CATCH_COUNT = 157,  ///< Largest saved single-visit catch count, capped at 255.
    PROGRESS_FIELD_FISHING_ALL_SPECIES_NOTICE_RECORDED = 158, ///< All-species congratulations path processed.
    PROGRESS_FIELD_FISHING_ALL_WORLD_RECORDS_NOTICE_RECORDED =
        159,                                      ///< All-world-record congratulations path processed.
    PROGRESS_FIELD_FRONT_YARD_DOOR_ENABLED = 160, ///< Bedroom passage to the front yard replaces its initial proxy.
    PROGRESS_FIELD_QUIZ_VARIANT_UNLOCK_STAGE =
        162 ///< Zero: picture quiz; one: shadow unlocked; two: strobe-shadow unlocked.
};

#endif
