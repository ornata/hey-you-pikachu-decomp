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
    PROGRESS_FIELD_SPRINGLEAF_ENTRANCE_INTRODUCTION_RECORDED =
        75, ///< Springleaf-entrance introduction processed; does not mark a completed visit.
    PROGRESS_FIELD_ODDISH_MEADOW_INTRODUCTION_RECORDED =
        77, ///< Oddish-meadow legend and Diglett introduction processed.
    PROGRESS_FIELD_SPRINGLEAF_WATERING_INTRODUCTION_RECORDED =
        79, ///< First Springleaf-field watering introduction and demonstration processed.
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
    PROGRESS_FIELD_TREASURE_HUNT_BEST_CHEST_COUNT =
        136, ///< Largest saved number of treasure chests opened during one Cobalt-island visit.
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
    PROGRESS_FIELD_QUIZ_VARIANT_UNLOCK_STAGE =
        162 ///< Zero: picture quiz; one: shadow unlocked; two: strobe-shadow unlocked.
};

/// Area-relative progress selectors resolved against the current gameplay area.
enum {
    PROGRESS_SELECTOR_CURRENT_AREA_SCENE_ENTRY_COUNT =
        0x40000000 ///< Scene-initialization count for the current area, saturating at fifteen.
};

#endif
