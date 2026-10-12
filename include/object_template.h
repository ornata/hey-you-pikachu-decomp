/// \file
/// Defines the model and hierarchy template copied when runtime objects are created.
#ifndef OBJECT_TEMPLATE_H
#define OBJECT_TEMPLATE_H
#include <ultra64.h>

/// Borrowed object definition; template links describe an acyclic hierarchy.
/// Runtime creation copies this prefix and initializes its own ownership state.
typedef struct ObjectTemplate {
    f32 scale[3];                   ///< Local scale along each axis.
    f32 rotation[3];                ///< Local rotation in degrees.
    f32 position[3];                ///< Local position relative to the parent.
    f32 radius;                     ///< Visibility bound used during hierarchy setup.
    struct ObjectTemplate *sibling; ///< Borrowed next sibling definition.
    struct ObjectTemplate *child;   ///< Borrowed first child definition.
    Gfx *displayList;               ///< Borrowed model drawing commands.
    u16 kind;                       ///< Runtime kind selecting initialization and lifecycle hooks.
    u16 flags;                      ///< Template flags; runtime creation supplies its initial flags.
} ObjectTemplate;

typedef char ObjectTemplateSizeCheck[(sizeof(ObjectTemplate) == 0x38) ? 1 : -1];
typedef char ObjectTemplateChildOffsetCheck[(((u32) & ((ObjectTemplate *)0)->child) == 0x2C) ? 1 : -1];
typedef char ObjectTemplateKindOffsetCheck[(((u32) & ((ObjectTemplate *)0)->kind) == 0x34) ? 1 : -1];
#endif
