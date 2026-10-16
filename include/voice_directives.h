/// \file
/// Names authored directive selectors carried by category-three command messages.

#ifndef VOICE_DIRECTIVES_H
#define VOICE_DIRECTIVES_H

/// Message category whose argument zero carries a dictionary-defined directive.
enum { VOICE_DIRECTIVE_MESSAGE_TYPE = 3 };

/// Retained directive-label families, independent of action-program and actor-kind IDs.
/// Names identify vocabulary requests, not guaranteed reactions or successful actions.
/// Values also differ from item-response categories and cost-policy selectors.
/// Creature-name labels retain this command domain; they are not ActorKind values.
/// Mixed-purpose values zero, sixteen and seventeen remain numeric.
typedef enum VoiceDirectiveValue {
    VOICE_DIRECTIVE_TOSS_IT = 1,
    VOICE_DIRECTIVE_LOOK_OVER_THERE = 2,
    VOICE_DIRECTIVE_EAT_OR_TASTE = 3,
    VOICE_DIRECTIVE_SMELL = 4,
    VOICE_DIRECTIVE_SOUND_PLAY_OR_TRY = 5,
    VOICE_DIRECTIVE_DROP_IT = 6,
    VOICE_DIRECTIVE_DIG_OR_PULL = 7,
    VOICE_DIRECTIVE_THUNDERSHOCK = 11,
    VOICE_DIRECTIVE_THUNDERBOLT = 12,
    VOICE_DIRECTIVE_THUNDER = 13,
    VOICE_DIRECTIVE_CANNON = 14,
    VOICE_DIRECTIVE_QUIT_OR_DONE = 15,
    VOICE_DIRECTIVE_THERE_OR_SWING = 18,
    VOICE_DIRECTIVE_GO_RIGHT = 19,
    VOICE_DIRECTIVE_GO_LEFT = 20,
    VOICE_DIRECTIVE_BEHIND_YOU = 21,
    VOICE_DIRECTIVE_STOP = 22,
    VOICE_DIRECTIVE_TURN_AROUND = 23,
    VOICE_DIRECTIVE_BACK_UP = 24,
    VOICE_DIRECTIVE_KEEP_GOING_OR_STRAIGHT_AHEAD = 25,
    VOICE_DIRECTIVE_CATCH_REEL_OR_NOW = 26,
    VOICE_DIRECTIVE_PULL_IT = 27,
    VOICE_DIRECTIVE_PULL_HARDER = 28,
    VOICE_DIRECTIVE_LET_GO_OR_RELEASE = 29,
    VOICE_DIRECTIVE_LUNCH_SNACK_OR_EAT = 30,
    VOICE_DIRECTIVE_ITS_RUNNING_AWAY = 31,
    VOICE_DIRECTIVE_LETS_GO = 32,
    VOICE_DIRECTIVE_CLEAN_UP_OR_PUT_AWAY = 33,
    VOICE_DIRECTIVE_BULBASAUR = 34,
    VOICE_DIRECTIVE_VENUSAUR = 35,
    VOICE_DIRECTIVE_SLOWPOKE = 36,
    VOICE_DIRECTIVE_SQUIRTLE = 37,
    VOICE_DIRECTIVE_CHARMANDER = 38,
    VOICE_DIRECTIVE_CATERPIE = 39,
    VOICE_DIRECTIVE_LAPRAS = 40,
    VOICE_DIRECTIVE_TOGEPI = 41,
    VOICE_DIRECTIVE_BOING_BOING = 42,
    VOICE_DIRECTIVE_PUT_THAT_BACK = 43,
    VOICE_DIRECTIVE_NEXT_STAGE = 44,
} VoiceDirectiveValue;

#endif
