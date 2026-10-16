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
    PROGRESS_FIELD_CHAPTER_FIVE_TOGETHER_NARRATION_RECORDED =
        25, ///< Chapter-five together narration processed; the front-yard sequence resets this gate.
    PROGRESS_FIELD_PIKACHU_NEGATIVE_REACTION_CREDIT =
        26, ///< Probabilistic credit from selected Pikachu negative-reaction effects, used to trigger disappearance; not current anger or one credit per event.
    PROGRESS_FIELD_CUPCAKE_ACQUIRED = 27,    ///< Cupcake acquisition recorded; does not imply current possession.
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
    PROGRESS_FIELD_BEDROOM_SCENE_ENTRY_COUNT =
        36, ///< Bedroom gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_ENTRYWAY_SCENE_ENTRY_COUNT =
        37, ///< Entryway gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_FRONT_YARD_SCENE_ENTRY_COUNT =
        38, ///< Front yard gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_VIRIDIAN_ENTRANCE_SCENE_ENTRY_COUNT =
        39, ///< Viridian entrance gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_VIRIDIAN_FOREST_SCENE_ENTRY_COUNT =
        40, ///< Viridian forest gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_VIRIDIAN_DEEP_SCENE_ENTRY_COUNT =
        41, ///< Viridian deep gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_SCENE_ENTRY_COUNT =
        42, ///< Springleaf entrance gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_ODDISH_MEADOW_SCENE_ENTRY_COUNT =
        43, ///< Oddish meadow gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_SPRINGLEAF_FIELD_SCENE_ENTRY_COUNT =
        44, ///< Springleaf field gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_OLIVINE_LAKESHORE_SCENE_ENTRY_COUNT =
        45, ///< Olivine lakeshore gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_OLIVINE_LAKE_SCENE_ENTRY_COUNT =
        46, ///< Olivine lake gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_OCHRE_ENTRANCE_SCENE_ENTRY_COUNT =
        47, ///< Ochre entrance gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_OCHRE_DEEP_SCENE_ENTRY_COUNT =
        48, ///< Ochre deep gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_COBALT_ISLAND_SCENE_ENTRY_COUNT =
        49, ///< Cobalt island gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_COBALT_COAST_SCENE_ENTRY_COUNT =
        50, ///< Cobalt coast gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_ABRA_SHOP_SCENE_ENTRY_COUNT =
        51, ///< Abra shop gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_WILDERNESS_CAMP_SCENE_ENTRY_COUNT =
        52, ///< Wilderness camp gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_OCHRE_FISHING_SCENE_ENTRY_COUNT =
        53, ///< Ochre fishing gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_OLIVINE_FISHING_SCENE_ENTRY_COUNT =
        54, ///< Olivine fishing gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_COBALT_FISHING_SCENE_ENTRY_COUNT =
        55, ///< Cobalt fishing gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_VIRIDIAN_EDGE_SCENE_ENTRY_COUNT =
        56, ///< Viridian edge gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_PLAYGROUND_SCENE_ENTRY_COUNT =
        57, ///< Playground gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_CREDITS_SCENE_ENTRY_COUNT =
        58, ///< Credits gameplay-scene initializations, saturating at fifteen; not completed visits.
    PROGRESS_FIELD_VIRIDIAN_DEEP_MAP_DESTINATION_UNLOCKED =
        59, ///< Viridian deep world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_OCHRE_ENTRANCE_MAP_DESTINATION_UNLOCKED =
        60, ///< Ochre entrance world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_OCHRE_FISHING_MAP_DESTINATION_UNLOCKED =
        61, ///< Ochre fishing world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_SPRINGLEAF_MAP_DESTINATION_UNLOCKED =
        62, ///< Springleaf world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_OLIVINE_LAKE_MAP_DESTINATION_UNLOCKED =
        63, ///< Olivine lake world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_OLIVINE_FISHING_MAP_DESTINATION_UNLOCKED =
        64, ///< Olivine fishing world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_COBALT_ISLAND_MAP_DESTINATION_UNLOCKED =
        65, ///< Cobalt island world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_COBALT_FISHING_MAP_DESTINATION_UNLOCKED =
        66, ///< Cobalt fishing world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_COBALT_COAST_MAP_DESTINATION_UNLOCKED =
        67, ///< Cobalt coast world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_FRONT_YARD_MAP_DESTINATION_UNLOCKED =
        68, ///< Front yard world-map destination available when the chase stage supplies an area.
    PROGRESS_FIELD_CATERPIE_BABYSITTING_INTRODUCTION_RECORDED =
        72, ///< Initial Viridian-deep Caterpie babysitting instructions processed.
    PROGRESS_FIELD_VIRIDIAN_DEEP_RESULT_SEQUENCE_COUNT =
        74, ///< Credited result sequences, saturating at fifteen; excludes the introduction.
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_INTRODUCTION_RECORDED =
        75, ///< Springleaf-entrance introduction processed; does not mark a completed visit.
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_RESULT_SEQUENCE_COUNT =
        76, ///< Credited activity-result sequences, saturating at fifteen; does not imply success.
    PROGRESS_FIELD_ODDISH_MEADOW_INTRODUCTION_RECORDED =
        77, ///< Oddish-meadow legend and Diglett introduction processed.
    PROGRESS_FIELD_ODDISH_MEADOW_RESULT_SEQUENCE_COUNT =
        78, ///< Credited activity-result sequences, saturating at fifteen; does not imply success.
    PROGRESS_FIELD_SPRINGLEAF_WATERING_INTRODUCTION_RECORDED =
        79, ///< First Springleaf-field watering introduction and demonstration processed.
    PROGRESS_FIELD_SPRINGLEAF_FIELD_RESULT_SEQUENCE_COUNT =
        80, ///< Credited activity-result sequences, saturating at fifteen; does not imply success.
    PROGRESS_FIELD_OLIVINE_LAKESHORE_RESULT_SEQUENCE_COUNT =
        81, ///< Credited activity-result sequences, saturating at fifteen; does not imply success.
    PROGRESS_FIELD_OLIVINE_LAKE_RESULT_SEQUENCE_COUNT =
        82, ///< Credited activity-result sequences, saturating at fifteen; does not imply success.
    PROGRESS_FIELD_OCHRE_PICNIC_INVITATION_RECORDED =
        83, ///< Initial Bulbasaur picnic invitation sequence processed; does not require a successful voice response.
    PROGRESS_FIELD_OCHRE_DEEP_DEPARTURE_RECORDED =
        84, ///< First Ochre-deep departure sequence processed; later departures use the abbreviated path.
    PROGRESS_FIELD_OCHRE_PICNIC_MEALS_COMPLETED =
        85, ///< Food-producing Ochre picnic results, regardless of dish quality; excludes the no-ingredients result.
    PROGRESS_FIELD_OCHRE_ENTRANCE_RESIDENT_STAGE = 86, ///< Zero through three selects additional entrance residents.
    PROGRESS_FIELD_COBALT_TREASURE_HUNTS_COMPLETED =
        87, ///< Completed Cobalt-island treasure-hunt result sequences, including visits with no opened chests.
    PROGRESS_FIELD_PIKACHU_DISAPPEARANCE_EPISODES_COMPLETED =
        93, ///< Completed disappearance visits selecting first or repeat search narration.
    PROGRESS_FIELD_DEMO_VARIANT_2_SELECTION_CREDIT =
        95, ///< Saved selection credit for demo variant two; reset tests the previous variant.
    PROGRESS_FIELD_DEMO_VARIANT_3_SELECTION_CREDIT =
        96, ///< Saved selection credit for demo variant three, reset when selected.
    PROGRESS_FIELD_DEMO_VARIANT_5_SELECTION_CREDIT =
        97, ///< Saved selection credit for demo variant five, reset when selected.
    PROGRESS_FIELD_DEMO_VARIANT_4_SELECTION_CREDIT =
        98, ///< Saved selection credit for demo variant four, reset when selected.
    PROGRESS_FIELD_DEMO_VARIANT_6_SELECTION_CREDIT =
        99, ///< Saved selection credit for demo variant six, reset when selected.
    PROGRESS_FIELD_VILEPLUME_MEADOW_VISIT_PENDING =
        100, ///< Next meadow visit uses five Vileplume; consumed on arrival.
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_GROWTH_STAGE_SLOT_0 =
        101, ///< Springleaf entrance resident slot 0: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_GROWTH_STAGE_SLOT_1 =
        102, ///< Springleaf entrance resident slot 1: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_GROWTH_STAGE_SLOT_2 =
        103, ///< Springleaf entrance resident slot 2: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_GROWTH_STAGE_SLOT_3 =
        104, ///< Springleaf entrance resident slot 3: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_GROWTH_STAGE_SLOT_4 =
        105, ///< Springleaf entrance resident slot 4: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_ODDISH_MEADOW_GROWTH_STAGE_SLOT_0 =
        106, ///< Oddish meadow resident slot 0: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_ODDISH_MEADOW_GROWTH_STAGE_SLOT_1 =
        107, ///< Oddish meadow resident slot 1: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_ODDISH_MEADOW_GROWTH_STAGE_SLOT_2 =
        108, ///< Oddish meadow resident slot 2: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_ODDISH_MEADOW_GROWTH_STAGE_SLOT_3 =
        109, ///< Oddish meadow resident slot 3: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_ODDISH_MEADOW_GROWTH_STAGE_SLOT_4 =
        110, ///< Oddish meadow resident slot 4: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_ODDISH_GROWTH_PROGRESS = 111, ///< Special meadow Oddish growth and scale progress.
    PROGRESS_FIELD_SPRINGLEAF_FIELD_GROWTH_STAGE_SLOT_0 =
        112, ///< Springleaf field resident slot 0: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SPRINGLEAF_FIELD_GROWTH_STAGE_SLOT_1 =
        113, ///< Springleaf field resident slot 1: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SPRINGLEAF_FIELD_GROWTH_STAGE_SLOT_2 =
        114, ///< Springleaf field resident slot 2: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SPRINGLEAF_FIELD_GROWTH_STAGE_SLOT_3 =
        115, ///< Springleaf field resident slot 3: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SPRINGLEAF_FIELD_GROWTH_STAGE_SLOT_4 =
        116, ///< Springleaf field resident slot 4: zero/one Oddish, two/three Gloom, four or above Vileplume.
    PROGRESS_FIELD_SHOPPING_ERRAND_BEST_TIME_SECONDS =
        117, ///< Fastest errand seconds, truncated and clamped to 255; zero is the unset sentinel.
    PROGRESS_FIELD_BUTTERFREE_HATCH_COUNT = 118, ///< Recorded Viridian-deep Metapod-to-Butterfree transformations.
    PROGRESS_FIELD_VIRIDIAN_CATERPIE_GROWTH_STAGE_SLOT_0 =
        119, ///< Viridian-deep Caterpie slot 0 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_CATERPIE_GROWTH_STAGE_SLOT_1 =
        120, ///< Viridian-deep Caterpie slot 1 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_CATERPIE_GROWTH_STAGE_SLOT_2 =
        121, ///< Viridian-deep Caterpie slot 2 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_CATERPIE_GROWTH_STAGE_SLOT_3 =
        122, ///< Viridian-deep Caterpie slot 3 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_CATERPIE_GROWTH_STAGE_SLOT_4 =
        123, ///< Viridian-deep Caterpie slot 4 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_0 =
        124, ///< Viridian-deep Metapod slot 0 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_1 =
        125, ///< Viridian-deep Metapod slot 1 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_2 =
        126, ///< Viridian-deep Metapod slot 2 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_3 =
        127, ///< Viridian-deep Metapod slot 3 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_4 =
        128, ///< Viridian-deep Metapod slot 4 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_5 =
        129, ///< Viridian-deep Metapod slot 5 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_6 =
        130, ///< Viridian-deep Metapod slot 6 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_7 =
        131, ///< Viridian-deep Metapod slot 7 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_8 =
        132, ///< Viridian-deep Metapod slot 8 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_VIRIDIAN_METAPOD_GROWTH_STAGE_SLOT_9 =
        133, ///< Viridian-deep Metapod slot 9 growth stage; cleared when its next evolution is recorded.
    PROGRESS_FIELD_MEGAPHONE_GAME_BEST_TIME_SECONDS =
        134, ///< Fastest eligible megaphone-game seconds; setup can initialize the U8 record to 255.
    PROGRESS_FIELD_TREASURE_HUNT_BEST_CHEST_COUNT =
        136, ///< Largest saved number of treasure chests opened during one Cobalt-island visit.
    PROGRESS_FIELD_TREASURE_CHEST_UNEARTHING_RECORDED =
        135, ///< Cobalt-island treasure-chest unearthing sequence recorded; does not mean the chest was opened.
    PROGRESS_FIELD_PINATA_BREAK_RECORDED = 137, ///< Cobalt-coast pinata-breaking sequence recorded.
    PROGRESS_FIELD_FISHING_MAGIKARP_RECORD_LENGTH =
        138, ///< Largest saved caught Magikarp length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_GOLDEEN_RECORD_LENGTH =
        139, ///< Largest saved caught Goldeen length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_SEAKING_RECORD_LENGTH =
        140, ///< Largest saved caught Seaking length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_TENTACOOL_RECORD_LENGTH =
        141, ///< Largest saved caught Tentacool length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_TENTACRUEL_RECORD_LENGTH =
        142, ///< Largest saved caught Tentacruel length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_STARYU_RECORD_LENGTH =
        143, ///< Largest saved caught Staryu length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_STARMIE_RECORD_LENGTH =
        144, ///< Largest saved caught Starmie length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_HORSEA_RECORD_LENGTH =
        145, ///< Largest saved caught Horsea length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_SEADRA_RECORD_LENGTH =
        146, ///< Largest saved caught Seadra length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_KRABBY_RECORD_LENGTH =
        147, ///< Largest saved caught Krabby length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_KINGLER_RECORD_LENGTH =
        148, ///< Largest saved caught Kingler length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_POLIWAG_RECORD_LENGTH =
        149, ///< Largest saved caught Poliwag length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_POLIWHIRL_RECORD_LENGTH =
        150, ///< Largest saved caught Poliwhirl length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_DRATINI_RECORD_LENGTH =
        151, ///< Largest saved caught Dratini length, truncated to the integral record value.
    PROGRESS_FIELD_FISHING_SQUIRTLE_RECORD_LENGTH =
        152, ///< Largest saved caught Squirtle length, truncated to the integral record value.
    PROGRESS_FIELD_LUCKY_HOOK_EXPLANATION_RECORDED = 154, ///< Fishing script has processed the lucky-hook explanation.
    PROGRESS_FIELD_FISHING_PROFICIENCY = 155,             ///< Adaptive fishing score, zero through fifteen.
    PROGRESS_FIELD_FISHING_TOTAL_CATCH_COUNT = 156,       ///< Cumulative caught Pokemon across fishing visits.
    PROGRESS_FIELD_FISHING_BEST_DAILY_CATCH_COUNT = 157,  ///< Largest saved single-visit catch count, capped at 255.
    PROGRESS_FIELD_FISHING_ALL_SPECIES_NOTICE_RECORDED = 158, ///< All-species congratulations path processed.
    PROGRESS_FIELD_FISHING_ALL_WORLD_RECORDS_NOTICE_RECORDED =
        159,                                      ///< All-world-record congratulations path processed.
    PROGRESS_FIELD_FRONT_YARD_DOOR_ENABLED = 160, ///< Bedroom passage to the front yard replaces its initial proxy.
    PROGRESS_FIELD_CHAPTER_SEVEN_MORNING_GREETING_RECORDED =
        161, ///< Chapter-seven morning-greeting sequence recorded, including its timeout outcome.
    PROGRESS_FIELD_QUIZ_VARIANT_UNLOCK_STAGE =
        162, ///< Zero: picture quiz; one: shadow unlocked; two: strobe-shadow unlocked.
    PROGRESS_FIELD_PICTURE_QUIZ_HIGH_SCORE_BITS_0_7 = 163,  ///< Bits 0 through 7 of the saved picture quiz high score.
    PROGRESS_FIELD_PICTURE_QUIZ_HIGH_SCORE_BITS_8_11 = 164, ///< Bits 8 through 11 of the saved picture quiz high score.
    PROGRESS_FIELD_PICTURE_QUIZ_HIGH_SCORE_BITS_12_13 =
        165,                                               ///< Bits 12 through 13 of the saved picture quiz high score.
    PROGRESS_FIELD_SHADOW_QUIZ_HIGH_SCORE_BITS_0_7 = 166,  ///< Bits 0 through 7 of the saved shadow quiz high score.
    PROGRESS_FIELD_SHADOW_QUIZ_HIGH_SCORE_BITS_8_11 = 167, ///< Bits 8 through 11 of the saved shadow quiz high score.
    PROGRESS_FIELD_SHADOW_QUIZ_HIGH_SCORE_BITS_12_13 = 168, ///< Bits 12 through 13 of the saved shadow quiz high score.
    PROGRESS_FIELD_STROBE_SHADOW_QUIZ_HIGH_SCORE_BITS_0_7 =
        169, ///< Bits 0 through 7 of the saved strobe-shadow quiz high score.
    PROGRESS_FIELD_STROBE_SHADOW_QUIZ_HIGH_SCORE_BITS_8_11 =
        170, ///< Bits 8 through 11 of the saved strobe-shadow quiz high score.
    PROGRESS_FIELD_STROBE_SHADOW_QUIZ_HIGH_SCORE_BITS_12_13 =
        171, ///< Bits 12 through 13 of the saved strobe-shadow quiz high score.
    PROGRESS_FIELD_OCHRE_COOKING_RESULT_RECORDED =
        172, ///< Cooking result-recording point reached, regardless of dish quality.
    PROGRESS_FIELD_ABRA_SHOPPING_WRAP_UP_RECORDED =
        173, ///< Shopping wrap-up reached; does not establish a successful purchase.
    PROGRESS_FIELD_HAUNTER_INTERACTION_RECORDED = 174, ///< Haunter reaction recorded, for either random response.
    PROGRESS_FIELD_DIGLETT_ENCOUNTER_RECORDED = 175, ///< Diglett encounter exit recorded, including cancellation paths.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_0_EARNED =
        176, ///< Saved award flag for friendship milestone 0; initial meeting does not increase friendship level.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_1_EARNED = 177,  ///< Saved award flag for friendship milestone 1.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_2_EARNED = 178,  ///< Saved award flag for friendship milestone 2.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_3_EARNED = 179,  ///< Saved award flag for friendship milestone 3.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_4_EARNED = 180,  ///< Saved award flag for friendship milestone 4.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_5_EARNED = 181,  ///< Saved award flag for friendship milestone 5.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_6_EARNED = 182,  ///< Saved award flag for friendship milestone 6.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_7_EARNED = 183,  ///< Saved award flag for friendship milestone 7.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_8_EARNED = 184,  ///< Saved award flag for friendship milestone 8.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_9_EARNED = 185,  ///< Saved award flag for friendship milestone 9.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_10_EARNED = 186, ///< Saved award flag for friendship milestone 10.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_11_EARNED = 187, ///< Saved award flag for friendship milestone 11.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_12_EARNED = 188, ///< Saved award flag for friendship milestone 12.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_13_EARNED = 189, ///< Saved award flag for friendship milestone 13.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_14_EARNED = 190, ///< Saved award flag for friendship milestone 14.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_15_EARNED = 191, ///< Saved award flag for friendship milestone 15.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_16_EARNED = 192, ///< Saved award flag for friendship milestone 16.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_17_EARNED = 193, ///< Saved award flag for friendship milestone 17.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_18_EARNED = 194, ///< Saved award flag for friendship milestone 18.
    PROGRESS_FIELD_FRIENDSHIP_MILESTONE_19_EARNED = 195, ///< Saved award flag for friendship milestone 19.
    /// Persistent collection records do not describe current possession.
    PROGRESS_FIELD_DAISY_COLLECTION_RECORDED = 196,          ///< Collection record for daisy.
    PROGRESS_FIELD_LILY_COLLECTION_RECORDED = 197,           ///< Collection record for lily.
    PROGRESS_FIELD_TOY_SWORD_COLLECTION_RECORDED = 198,      ///< Collection record for toy sword.
    PROGRESS_FIELD_PIRATE_SWORD_COLLECTION_RECORDED = 199,   ///< Collection record for pirate sword.
    PROGRESS_FIELD_GOLD_COIN_COLLECTION_RECORDED = 200,      ///< Collection record for gold coin.
    PROGRESS_FIELD_CHOCOLATE_COIN_COLLECTION_RECORDED = 201, ///< Collection record for chocolate coin.
    PROGRESS_FIELD_SILVER_COIN_COLLECTION_RECORDED = 202,    ///< Collection record for silver coin.
    PROGRESS_FIELD_COPPER_COIN_COLLECTION_RECORDED = 203,    ///< Collection record for copper coin.
    PROGRESS_FIELD_BANANA_COLLECTION_RECORDED = 204,         ///< Collection record for banana.
    PROGRESS_FIELD_RUBY_COLLECTION_RECORDED = 205,           ///< Collection record for ruby.
    PROGRESS_FIELD_SAPPHIRE_COLLECTION_RECORDED = 206,       ///< Collection record for sapphire.
    PROGRESS_FIELD_TOPAZ_COLLECTION_RECORDED = 207,          ///< Collection record for topaz.
    PROGRESS_FIELD_RED_MARBLE_COLLECTION_RECORDED = 208,     ///< Collection record for red marble.
    PROGRESS_FIELD_BLUE_MARBLE_COLLECTION_RECORDED = 209,    ///< Collection record for blue marble.
    PROGRESS_FIELD_YELLOW_MARBLE_COLLECTION_RECORDED = 210,  ///< Collection record for yellow marble.
    PROGRESS_FIELD_MARBLE_COLLECTION_RECORDED = 211,         ///< Collection record for marble.
    PROGRESS_FIELD_RUBY_RING_COLLECTION_RECORDED = 212,      ///< Collection record for ruby ring.
    PROGRESS_FIELD_BIRD_FEATHER_COLLECTION_RECORDED = 213,   ///< Collection record for bird feather.
    PROGRESS_FIELD_MAGNET_COLLECTION_RECORDED = 214,         ///< Collection record for magnet.
    PROGRESS_FIELD_BOLT_COLLECTION_RECORDED = 215,           ///< Collection record for bolt.
    PROGRESS_FIELD_ACORN_TOP_COLLECTION_RECORDED = 216,      ///< Collection record for acorn top.
    PROGRESS_FIELD_TRUMPET_COLLECTION_RECORDED = 217,        ///< Collection record for trumpet.
    PROGRESS_FIELD_PETUNIA_COLLECTION_RECORDED = 218,        ///< Collection record for petunia.
    PROGRESS_FIELD_TULIP_COLLECTION_RECORDED = 219,          ///< Collection record for tulip.
    PROGRESS_FIELD_DANDELION_COLLECTION_RECORDED = 220,      ///< Collection record for dandelion.
    PROGRESS_FIELD_BLUEBELL_COLLECTION_RECORDED = 221,       ///< Collection record for bluebell.
    PROGRESS_FIELD_SKUNK_BLOSSOM_COLLECTION_RECORDED = 222,  ///< Collection record for skunk blossom.
    PROGRESS_FIELD_SUNFLOWER_COLLECTION_RECORDED = 223,      ///< Collection record for sunflower.
    PROGRESS_FIELD_CORN_COLLECTION_RECORDED = 224,           ///< Collection record for corn.
    PROGRESS_FIELD_PUMPKIN_COLLECTION_RECORDED = 225,        ///< Collection record for pumpkin.
    PROGRESS_FIELD_TURNIP_COLLECTION_RECORDED = 226,         ///< Collection record for turnip.
    PROGRESS_FIELD_CABBAGE_COLLECTION_RECORDED = 227,        ///< Collection record for cabbage.
    PROGRESS_FIELD_CARROT_COLLECTION_RECORDED = 228,         ///< Collection record for carrot.
    PROGRESS_FIELD_ONION_COLLECTION_RECORDED = 229,          ///< Collection record for onion.
    PROGRESS_FIELD_SWEET_POTATO_COLLECTION_RECORDED = 230,   ///< Collection record for sweet potato.
    PROGRESS_FIELD_POKE_DISC_COLLECTION_RECORDED = 231,      ///< Collection record for Poke disc.
    PROGRESS_FIELD_BLUE_BALLOON_COLLECTION_RECORDED = 232,   ///< Collection record for blue balloon.
    PROGRESS_FIELD_GREEN_BALLOON_COLLECTION_RECORDED = 233,  ///< Collection record for green balloon.
    PROGRESS_FIELD_PURPLE_BALLOON_COLLECTION_RECORDED = 234, ///< Collection record for purple balloon.
    PROGRESS_FIELD_RED_BALLOON_COLLECTION_RECORDED = 235,    ///< Collection record for red balloon.
    PROGRESS_FIELD_YELLOW_BALLOON_COLLECTION_RECORDED = 236, ///< Collection record for yellow balloon.
    PROGRESS_FIELD_LIGHT_BULB_COLLECTION_RECORDED = 237,     ///< Collection record for light bulb.
    PROGRESS_FIELD_TRUFFLE_TOP_COLLECTION_RECORDED = 238,    ///< Collection record for truffle top.
    PROGRESS_FIELD_PIRATE_FLAG_COLLECTION_RECORDED = 239,    ///< Collection record for pirate flag.
    PROGRESS_FIELD_TELESCOPE_COLLECTION_RECORDED = 240,      ///< Telescope collection record and final story-gift flag.
    PROGRESS_FIELD_OCHRE_FISHING_FLAG_COLLECTION_RECORDED = 241,   ///< Collection record for the Ochre fishing prize.
    PROGRESS_FIELD_OLIVINE_FISHING_FLAG_COLLECTION_RECORDED = 242, ///< Collection record for the Olivine fishing prize.
    PROGRESS_FIELD_COBALT_FISHING_FLAG_COLLECTION_RECORDED = 243   ///< Collection record for the Cobalt fishing prize.
};

/// Area-relative progress selectors resolved against the current gameplay area.
enum {
    PROGRESS_SELECTOR_CURRENT_AREA_SCENE_ENTRY_COUNT =
        0x40000000 ///< Scene-initialization count for the current area, saturating at fifteen.
};

#endif
