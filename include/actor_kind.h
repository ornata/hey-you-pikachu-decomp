/// \file
/// Names retained actor configurations used by creation, queries and script searches.

#ifndef ACTOR_KIND_H
#define ACTOR_KIND_H

/// Actor configuration identifiers supported by the retained actor-kind consumers.
/// The domain includes props as well as creatures. Names follow the configured
/// inspection identities; equal names do not merge distinct configurations.
typedef enum ActorKind {
    ACTOR_KIND_BULBASAUR = 1,
    ACTOR_KIND_VENUSAUR = 2,
    ACTOR_KIND_SQUIRTLE = 3,
    ACTOR_KIND_CHARMANDER = 4,
    ACTOR_KIND_ODDISH = 5,
    ACTOR_KIND_GLOOM = 6,
    ACTOR_KIND_VILEPLUME = 7,
    ACTOR_KIND_CATERPIE = 8,
    ACTOR_KIND_METAPOD = 9,
    ACTOR_KIND_DIGLETT = 10,
    ACTOR_KIND_LAPRAS = 11,
    ACTOR_KIND_POLIWAG = 12,
    ACTOR_KIND_POLIWHIRL = 13,
    ACTOR_KIND_BUTTERFREE = 14,
    ACTOR_KIND_ABRA = 15,
    ACTOR_KIND_TREASURE_CHEST = 16,
    ACTOR_KIND_HAUNTER = 17,
    ACTOR_KIND_SLOWPOKE = 18,
    ACTOR_KIND_MAGNEMITE = 19,
    ACTOR_KIND_FISHING_ROD = 20,
    ACTOR_KIND_TELESCOPE = 21,
    ACTOR_KIND_TOGEPI = 22,
    ACTOR_KIND_TOGEPI_EGG = 23,
    ACTOR_KIND_SEAKING = 24,
    ACTOR_KIND_KRABBY = 25,
    ACTOR_KIND_TENTACRUEL = 26,
    ACTOR_KIND_STARYU = 27,
    ACTOR_KIND_KINGLER = 28,
    ACTOR_KIND_MAGIKARP = 29,
    ACTOR_KIND_TENTACOOL = 30,
    ACTOR_KIND_DRATINI = 31,
    ACTOR_KIND_SEADRA = 32,
    ACTOR_KIND_STARMIE = 33,
    ACTOR_KIND_HORSEA = 34,
    ACTOR_KIND_GOLDEEN = 35,
    ACTOR_KIND_PIKACHU_ALTERNATE = 36, ///< Distinct Pikachu configuration used by scene-authored residents.
    ACTOR_KIND_PIKACHU = 37            ///< Configuration used for the active companion.
} ActorKind;

/// Context-dependent endpoint alias accepted by actor-range searches.
/// As a minimum it selects Bulbasaur; as a maximum it selects companion Pikachu.
/// This alias is not an actor configuration identifier for creation.
enum { ACTOR_SEARCH_RANGE_ENDPOINT = 38 };

#endif
