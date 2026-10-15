/// \file
/// Defines area identifiers and the extent of the retained scene overlay catalog.

#ifndef AREA_CATALOG_H
#define AREA_CATALOG_H

/// Area selectors in the matching catalog's translation-unit order.
/// SCENE_AREA_COUNT is the catalog extent and is not an area identifier.
typedef enum SceneAreaId {
    AREA_BEDROOM = 0,             ///< Bedroom overlay.
    AREA_ENTRYWAY = 1,            ///< Entryway overlay.
    AREA_FRONT_YARD = 2,          ///< Front yard overlay.
    AREA_VIRIDIAN_ENTRANCE = 3,   ///< Viridian entrance overlay.
    AREA_VIRIDIAN_FOREST = 4,     ///< Viridian forest overlay.
    AREA_VIRIDIAN_DEEP = 5,       ///< Viridian deep overlay.
    AREA_SPRINGLEAF_ENTRANCE = 6, ///< Springleaf entrance overlay.
    AREA_ODDISH_MEADOW = 7,       ///< Oddish meadow overlay.
    AREA_SPRINGLEAF_FIELD = 8,    ///< Springleaf field overlay.
    AREA_OLIVINE_LAKESHORE = 9,   ///< Olivine lakeshore overlay.
    AREA_OLIVINE_LAKE = 10,       ///< Olivine lake overlay.
    AREA_OCHRE_ENTRANCE = 11,     ///< Ochre entrance overlay.
    AREA_OCHRE_DEEP = 12,         ///< Ochre deep overlay.
    AREA_COBALT_ISLAND = 13,      ///< Cobalt island overlay.
    AREA_COBALT_COAST = 14,       ///< Cobalt coast overlay.
    AREA_ABRA_SHOP = 15,          ///< Abra shop overlay.
    AREA_WILDERNESS_CAMP = 16,    ///< Wilderness camp overlay.
    AREA_OCHRE_FISHING = 17,      ///< Ochre fishing overlay.
    AREA_OLIVINE_FISHING = 18,    ///< Olivine fishing overlay.
    AREA_COBALT_FISHING = 19,     ///< Cobalt fishing overlay.
    AREA_VIRIDIAN_EDGE = 20,      ///< Viridian edge overlay.
    AREA_PLAYGROUND = 21,         ///< Playground overlay.
    AREA_CREDITS = 22,            ///< Credits overlay.
    SCENE_AREA_COUNT = 23         ///< Number of retained areas.
} SceneAreaId;

#endif
