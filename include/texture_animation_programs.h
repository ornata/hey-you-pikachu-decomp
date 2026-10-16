/// \file
/// Declares resident integer-animation programs for object and UI texture frames.

#ifndef TEXTURE_ANIMATION_PROGRAMS_H
#define TEXTURE_ANIMATION_PROGRAMS_H

#include "integer_animation_types.h"

/// Identified attached-effect texture programs; selected slots do not establish
/// a bound on the complete resident program table.
typedef enum AttachedEffectTextureAnimationProgram {
    TEXTURE_ANIMATION_EFFECT_EXCLAMATION_MARK = 164,       ///< Held exclamation-mark icon.
    TEXTURE_ANIMATION_EFFECT_QUESTION_MARK = 165,          ///< Held question-mark icon.
    TEXTURE_ANIMATION_REACTION_SKULL_AND_CROSSBONES = 166, ///< Held skull-and-crossbones icon.
    TEXTURE_ANIMATION_REACTION_SCRIBBLE = 167,             ///< Looping scribbled frustration cloud.
    TEXTURE_ANIMATION_EFFECT_BLINKING_LIGHT_BULB = 168,    ///< Alternating light-bulb frames.
    TEXTURE_ANIMATION_EFFECT_FLOWER = 170,                 ///< Held flower icon.
    TEXTURE_ANIMATION_EFFECT_CROSS = 171,                  ///< Held cross icon.
    TEXTURE_ANIMATION_EFFECT_SPIRAL_SUN = 174,             ///< Held spiral-centered sun icon.
    TEXTURE_ANIMATION_REACTION_ANGER_MARK = 175,           ///< Held four-part anger mark.
    TEXTURE_ANIMATION_REACTION_STEAM_PUFF = 179,           ///< Held steam-puff icon.
    TEXTURE_ANIMATION_EFFECT_ZIGZAG = 183                  ///< Held zigzag icon; its emotional meaning is unresolved.
} AttachedEffectTextureAnimationProgram;

/// Borrowed resident key programs indexed by supported texture-animation identifiers.
/// Entries and their key streams outlive animation tracks; the full table bound
/// remains unresolved, so callers must use identifiers established by their resources.
/// \note Original identifier: \c D_8006B090 (0x8006B090).
extern ValueKey *gTextureFrameAnimationPrograms[];

#endif
