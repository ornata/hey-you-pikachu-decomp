/// \file
/// Names destination requests carried by world-map voice messages.

#ifndef VOICE_MAP_DESTINATION_H
#define VOICE_MAP_DESTINATION_H

/// Destination labels for argument zero of SCRIPT_MESSAGE_WORLD_MAP_DESTINATION.
/// Values are recognized command selectors, not map-menu indices or area IDs.
/// Names describe requests, without guaranteeing availability or final selection.
/// The special-training request is accepted only in the last chase stage.
typedef enum VoiceMapDestinationValue {
    VOICE_MAP_DESTINATION_HOME = 0,
    VOICE_MAP_DESTINATION_VIRIDIAN_FOREST = 1,
    VOICE_MAP_DESTINATION_OCHRE_WOODS = 2,
    VOICE_MAP_DESTINATION_OCHRE_FISHING_HOLE = 3,
    VOICE_MAP_DESTINATION_SPRINGLEAF_FIELD = 4,
    VOICE_MAP_DESTINATION_OLIVINE_LAKE = 5,
    VOICE_MAP_DESTINATION_OLIVINE_FISHING_HOLE = 6,
    VOICE_MAP_DESTINATION_COBALT_ISLAND = 7,
    VOICE_MAP_DESTINATION_COBALT_FISHING_HOLE = 8,
    VOICE_MAP_DESTINATION_COBALT_COAST = 9,
    VOICE_MAP_DESTINATION_SPECIAL_TRAINING = 10,
} VoiceMapDestinationValue;

#endif
