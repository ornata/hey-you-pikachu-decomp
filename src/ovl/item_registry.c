#include "camera_view.h"
#include "collision_registration.h"
#include "object_distance.h"
#include "search_tree_traversal.h"
#include "object_bounds.h"
#include "interaction.h"
#include "object_flags.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "item_object.h"
#include "heap_free.h"
/* harvested: matched m2c drafts; pending style pass */
/* Item objects. .rodata is the item-kind text (initializer order), then the
 * debug-trace formats in function order, then the late float/jump tables. */
#include "common.h"
#include "direction_animation.h"
#include "item_response.h"
#include "item_text.h"
#include "search_tree.h"
#include "object_audio.h"
#include "item_audio.h"
#include "integer_animation.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "scene_types.h"
#include "item_types.h"
#include "item_kind.h"
#include "object_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/* .bss 0x80134030..0x801340B0, in address order (IDO lays globals out in declaration order) */
/// Search tree indexing live items by kind and object handle.
/// \note Original identifier: \c D_80134030 (0x80134030).
SearchTree gItemSearchTree;
/// Number of live registered items in the current area.
/// \note Original identifier: \c D_80134040 (0x80134040).
s32 gLiveItemCount;
s32 D_80134040_0;
/* Search state initialized by 80113130 and consumed by 80112FC0. The
 * 0x30-byte transform is also addressed through the interior symbol 80134060. */
typedef struct {
    /* 0x00 */ u8 *best;
    /* 0x04 */ f32 bestDistance;
    /* 0x08 */ f32 origin[3];
    /* 0x14 */ f32 maxDistance;
    /* 0x18 */ f32 transform[12];
    /* 0x48 */ f32 cone;
    /* 0x4C */ s32 minKind;
    /* 0x50 */ s32 maxKind;
    /* 0x54 */ s32 filter;
    /* 0x58 */ s32 weightEnabled;
} ItemQueryState;
/// Shared nearest-item query context; traversal must not be reentered.
/// \note Original identifier: \c D_80134048 (0x80134048).
ItemQueryState gItemNearestSearch;

/* .data 0x80123F90..0x8012A660. Each item kind owns its text: IDO emits the
 * initializer string literals into .rodata in field order, ahead of the
 * functions' strings. */



/* Kind-number range per category, tested by itemKindIsInCategory. */
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 lo;
    /* 0x8 */ s32 hi;
} ItemRange;

void baseballCapUpdateHeldVisibility(u8 *);
void baseballCapResetReleasedScale(u8 *, s32);
void lightBulbUpdateHeldGlow(u8 *);
void lightBulbResetReleasedGlow(u8 *, s32);
void balloonUpdateHeldInflation(u8 *);
void balloonBeginReleasedFlight(u8 *);
s32 itemResolveMapCollision(u8 *, u16);
void itemInitializeFlightSpin(u8 *);

// clang-format off
/// Borrowed model hierarchy templates cloned to create item objects.
/// \note Original identifier: \c D_80123F90 (0x80123F90).
ItemModel gItemModelTemplates[122] = {
    /*   0 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 218.2f,
              NULL, NULL, (Gfx *)0x0102DBC0, 0x0, 0x0 },
    /*   1 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 252.5f,
              NULL, NULL, (Gfx *)0x0102E2D0, 0x0, 0x0 },
    /*   2 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 200.0f,
              NULL, NULL, (Gfx *)0x0102EAA8, 0x0, 0x0 },
    /*   3 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[4], NULL, 0x0, 0x0 },
    /*   4 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 78.8f,
              NULL, NULL, (Gfx *)0x0102EDB8, 0x98, 0x0 },
    /*   5 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 319.3f,
              NULL, NULL, (Gfx *)0x0102F3E0, 0x0, 0x0 },
    /*   6 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 318.2f,
              NULL, NULL, (Gfx *)0x0102F9C0, 0x0, 0x0 },
    /*   7 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 274.1f,
              NULL, NULL, (Gfx *)0x0102FF90, 0x0, 0x0 },
    /*   8 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 272.1f,
              NULL, NULL, (Gfx *)0x01030470, 0x0, 0x0 },
    /*   9 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 376.1f,
              NULL, NULL, (Gfx *)0x010307C8, 0x0, 0x0 },
    /*  10 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 427.3f,
              NULL, NULL, (Gfx *)0x01030D38, 0x0, 0x0 },
    /*  11 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 144.6f,
              NULL, NULL, (Gfx *)0x010315E8, 0x0, 0x0 },
    /*  12 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 206.5f,
              NULL, NULL, (Gfx *)0x01031E78, 0x0, 0x0 },
    /*  13 */ { { 1.0f, 1.0f, 1.0f }, { 0.5f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 412.9f,
              NULL, &gItemModelTemplates[14], (Gfx *)0x010326B8, 0x0, 0x0 },
    /*  14 */ { { 1.0f, 1.0f, 1.0f }, { -0.5f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 12558336.0f,
              NULL, NULL, (Gfx *)0x01032DC8, 0x97, 0x0 },
    /*  15 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 275.6f,
              NULL, &gItemModelTemplates[16], (Gfx *)0x01033430, 0x0, 0x0 },
    /*  16 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 212.2f,
              NULL, NULL, (Gfx *)0x010335D8, 0x95, 0x0 },
    /*  17 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 175.1f,
              NULL, NULL, (Gfx *)0x01033A70, 0x0, 0x0 },
    /*  18 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 174.3f,
              NULL, NULL, (Gfx *)0x01034340, 0x0, 0x0 },
    /*  19 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 186.9f,
              NULL, NULL, (Gfx *)0x010347E8, 0x0, 0x0 },
    /*  20 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 184.5f,
              NULL, NULL, (Gfx *)0x01034E60, 0x0, 0x0 },
    /*  21 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 227.3f,
              NULL, NULL, (Gfx *)0x01035340, 0x0, 0x0 },
    /*  22 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 95.6f,
              NULL, &gItemModelTemplates[23], (Gfx *)0x010355F0, 0x0, 0x0 },
    /*  23 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 152.4f,
              NULL, NULL, (Gfx *)0x01035AB8, 0x0, 0x0 },
    /*  24 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 506.2f,
              NULL, NULL, (Gfx *)0x01044938, 0x0, 0x0 },
    /*  25 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 102.1f,
              NULL, NULL, (Gfx *)0x01036040, 0x0, 0x0 },
    /*  26 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 247.5f,
              NULL, NULL, (Gfx *)0x01036A08, 0x0, 0x0 },
    /*  27 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 191.2f,
              NULL, NULL, (Gfx *)0x01036FF8, 0x0, 0x0 },
    /*  28 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 263.7f,
              NULL, NULL, (Gfx *)0x01037500, 0x0, 0x0 },
    /*  29 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 385.6f,
              NULL, NULL, (Gfx *)0x01037CB8, 0x0, 0x0 },
    /*  30 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 312.9f,
              NULL, NULL, (Gfx *)0x01038238, 0x0, 0x0 },
    /*  31 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 779.2f,
              NULL, NULL, (Gfx *)0x01038640, 0x0, 0x0 },
    /*  32 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 356.4f,
              NULL, NULL, (Gfx *)0x01038AE8, 0x0, 0x0 },
    /*  33 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 196.0f,
              NULL, &gItemModelTemplates[34], (Gfx *)0x01038DD8, 0x0, 0x0 },
    /*  34 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 196.0f,
              NULL, NULL, (Gfx *)0x01038F98, 0x0, 0x0 },
    /*  35 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 256.1f,
              NULL, NULL, (Gfx *)0x01039238, 0x0, 0x0 },
    /*  36 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 328.6f,
              NULL, NULL, (Gfx *)0x01039860, 0x0, 0x0 },
    /*  37 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[38], NULL, 0x0, 0x0 },
    /*  38 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 100.0f,
              NULL, NULL, (Gfx *)0x0103A2A8, 0x98, 0x0 },
    /*  39 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 112.8f,
              NULL, &gItemModelTemplates[40], (Gfx *)0x0103A540, 0x0, 0x0 },
    /*  40 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 60.5f,
              NULL, NULL, (Gfx *)0x0103A890, 0x98, 0x0 },
    /*  41 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 195.5f,
              NULL, NULL, (Gfx *)0x0103AB98, 0x0, 0x0 },
    /*  42 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 119.3f,
              NULL, NULL, (Gfx *)0x0103AF00, 0x0, 0x0 },
    /*  43 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 145.2f,
              NULL, NULL, (Gfx *)0x0103B1F0, 0x0, 0x0 },
    /*  44 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 280.4f,
              NULL, NULL, (Gfx *)0x0103B4B8, 0x0, 0x0 },
    /*  45 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 184.6f,
              NULL, NULL, (Gfx *)0x0103B820, 0x0, 0x0 },
    /*  46 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 268.0f,
              NULL, &gItemModelTemplates[47], (Gfx *)0x0103BAA8, 0x0, 0x0 },
    /*  47 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 148.0f,
              &gItemModelTemplates[48], NULL, (Gfx *)0x0103BC88, 0x0, 0x0 },
    /*  48 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 99.7f,
              NULL, NULL, (Gfx *)0x0103BE20, 0x0, 0x0 },
    /*  49 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 210.0f,
              NULL, NULL, (Gfx *)0x0103C510, 0x0, 0x0 },
    /*  50 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 192.7f,
              NULL, NULL, (Gfx *)0x0103C850, 0x0, 0x0 },
    /*  51 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 272.3f,
              NULL, &gItemModelTemplates[52], (Gfx *)0x0103CB58, 0x0, 0x0 },
    /*  52 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 87.6f,
              &gItemModelTemplates[53], NULL, (Gfx *)0x0103CDA8, 0x0, 0x0 },
    /*  53 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 123.4f,
              NULL, NULL, (Gfx *)0x0103CF60, 0x0, 0x0 },
    /*  54 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 209.8f,
              NULL, NULL, (Gfx *)0x0103D200, 0x0, 0x0 },
    /*  55 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 235.1f,
              NULL, NULL, (Gfx *)0x0103D950, 0x0, 0x0 },
    /*  56 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 286.9f,
              NULL, NULL, (Gfx *)0x0103DEA0, 0x0, 0x0 },
    /*  57 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 169.6f,
              NULL, NULL, (Gfx *)0x0103E708, 0x0, 0x0 },
    /*  58 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 29.0f,
              NULL, &gItemModelTemplates[59], (Gfx *)0x0103E908, 0x0, 0x0 },
    /*  59 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 20.0f, 35.8631f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[60], (Gfx *)0x0103E9B8, 0x98, 0x0 },
    /*  60 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 50.0f,
              NULL, NULL, (Gfx *)0x0103EAC0, 0x98, 0x0 },
    /*  61 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 195.0f,
              NULL, NULL, (Gfx *)0x0103ED88, 0x0, 0x0 },
    /*  62 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 84.8f,
              NULL, NULL, (Gfx *)0x0103F070, 0x0, 0x0 },
    /*  63 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 227.8f,
              NULL, NULL, (Gfx *)0x0103F7C0, 0x0, 0x0 },
    /*  64 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 200.0f,
              NULL, NULL, (Gfx *)0x0103FC00, 0x0, 0x0 },
    /*  65 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 212.4f,
              NULL, &gItemModelTemplates[66], (Gfx *)0x01040110, 0x0, 0x0 },
    /*  66 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 36.8f,
              NULL, NULL, (Gfx *)0x01040348, 0x0, 0x0 },
    /*  67 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 94.3f,
              NULL, &gItemModelTemplates[68], (Gfx *)0x01040638, 0x0, 0x0 },
    /*  68 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 125.0f }, 0.0f,
              NULL, &gItemModelTemplates[69], NULL, 0x0, 0x0 },
    /*  69 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -90.0f }, { 0.0f, 0.0f, 0.0f }, 163.6f,
              NULL, &gItemModelTemplates[70], (Gfx *)0x010407C0, 0x0, 0x0 },
    /*  70 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 180.2001f, 0.0f, 0.0f }, 148.0f,
              NULL, &gItemModelTemplates[71], (Gfx *)0x01040988, 0x0, 0x0 },
    /*  71 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 145.9848f, 0.0f, 0.0f }, 130.6f,
              NULL, &gItemModelTemplates[72], (Gfx *)0x01040B50, 0x0, 0x0 },
    /*  72 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 132.2988f, 0.0f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[73], NULL, 0x0, 0x0 },
    /*  73 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 271.4f,
              NULL, &gItemModelTemplates[74], (Gfx *)0x01040F98, 0x0, 0x0 },
    /*  74 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 37.3f,
              &gItemModelTemplates[75], NULL, (Gfx *)0x01041328, 0x0, 0x0 },
    /*  75 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 271.4f,
              &gItemModelTemplates[76], NULL, (Gfx *)0x010417A8, 0x0, 0x0 },
    /*  76 */ { { 1.0f, 1.0f, 1.0f }, { -45.0f, -90.0f, -45.0f }, { 139.581f, 0.0f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[77], NULL, 0x0, 0x0 },
    /*  77 */ { { 1.0f, 1.0f, 1.0f }, { 180.0f, -180.0f, 180.0f }, { 0.0f, 0.0001f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[78], NULL, 0x81, 0x0 },
    /*  78 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[79], NULL, 0x0, 0x0 },
    /*  79 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 78.7f,
              NULL, NULL, (Gfx *)0x010419F8, 0x98, 0x0 },
    /*  80 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 141.4f,
              NULL, NULL, (Gfx *)0x01041AE0, 0x0, 0x0 },
    /*  81 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 94.3f,
              NULL, &gItemModelTemplates[82], (Gfx *)0x01041E58, 0x0, 0x0 },
    /*  82 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 125.0f }, 0.0f,
              NULL, &gItemModelTemplates[83], NULL, 0x0, 0x0 },
    /*  83 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -90.0f }, { 0.0f, 0.0f, 0.0f }, 163.6f,
              NULL, &gItemModelTemplates[84], (Gfx *)0x01041FE0, 0x0, 0x0 },
    /*  84 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 180.2001f, 0.0f, 0.0f }, 148.0f,
              NULL, &gItemModelTemplates[85], (Gfx *)0x010421A8, 0x0, 0x0 },
    /*  85 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 145.9848f, 0.0f, 0.0f }, 130.6f,
              NULL, &gItemModelTemplates[86], (Gfx *)0x01042370, 0x0, 0x0 },
    /*  86 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 132.2988f, 0.0f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[87], NULL, 0x0, 0x0 },
    /*  87 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f,
              NULL, &gItemModelTemplates[88], NULL, 0x0, 0x0 },
    /*  88 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 271.4f,
              &gItemModelTemplates[92], &gItemModelTemplates[89], (Gfx *)0x010427B8, 0x0, 0x0 },
    /*  89 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 59.5f,
              &gItemModelTemplates[90], NULL, (Gfx *)0x01042BA8, 0x0, 0x0 },
    /*  90 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 244.2f,
              NULL, &gItemModelTemplates[91], (Gfx *)0x01042E98, 0x0, 0x0 },
    /*  91 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 271.4f,
              NULL, NULL, (Gfx *)0x010431A0, 0x0, 0x0 },
    /*  92 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 271.4f,
              NULL, &gItemModelTemplates[93], (Gfx *)0x01043620, 0x0, 0x0 },
    /*  93 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 244.2f,
              NULL, &gItemModelTemplates[94], (Gfx *)0x010439B0, 0x0, 0x0 },
    /*  94 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 271.4f,
              NULL, NULL, (Gfx *)0x01043CB8, 0x0, 0x0 },
    /*  95 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 213.3f,
              NULL, NULL, (Gfx *)0x0100C4F8, 0x0, 0x0 },
    /*  96 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 326.3f,
              NULL, NULL, (Gfx *)0x01045068, 0x0, 0x0 },
    /*  97 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 320.6f,
              NULL, NULL, (Gfx *)0x010458D8, 0x0, 0x0 },
    /*  98 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 225.3f,
              NULL, NULL, (Gfx *)0x01046010, 0x0, 0x0 },
    /*  99 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 307.8f,
              NULL, NULL, (Gfx *)0x01046448, 0x0, 0x0 },
    /* 100 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 201.7f,
              NULL, NULL, (Gfx *)0x010469B0, 0x0, 0x0 },
    /* 101 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 192.0f,
              NULL, NULL, (Gfx *)0x01046C68, 0x0, 0x0 },
    /* 102 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 191.2f,
              NULL, NULL, (Gfx *)0x01046FE8, 0x0, 0x0 },
    /* 103 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 200.0f,
              NULL, NULL, (Gfx *)0x010472B8, 0x0, 0x0 },
    /* 104 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 221.3f,
              NULL, NULL, (Gfx *)0x01047928, 0x0, 0x0 },
    /* 105 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 100.0f,
              NULL, NULL, (Gfx *)0x01047BA0, 0x0, 0x0 },
    /* 106 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 1092.1f,
              NULL, NULL, (Gfx *)0x010486C8, 0x0, 0x0 },
    /* 107 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 2800.0f,
              NULL, NULL, (Gfx *)0x01049300, 0x0, 0x0 },
    /* 108 */ { { 1.3524f, 1.3524f, 1.3524f }, { 1.5416f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 287.9f,
              NULL, NULL, (Gfx *)0x01049F80, 0x0, 0x0 },
    /* 109 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 445.1f,
              NULL, NULL, (Gfx *)0x0104A2D8, 0x0, 0x0 },
    /* 110 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 233.5f,
              NULL, NULL, (Gfx *)0x0104ADD0, 0x0, 0x0 },
    /* 111 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 129.9f,
              NULL, NULL, (Gfx *)0x0104B420, 0x0, 0x0 },
    /* 112 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 893.5f,
              NULL, NULL, (Gfx *)0x0104C650, 0x0, 0x0 },
    /* 113 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 101.5f,
              NULL, &gItemModelTemplates[114], (Gfx *)0x0104CE60, 0x0, 0x0 },
    /* 114 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 65.4f,
              &gItemModelTemplates[115], NULL, (Gfx *)0x0104CFC0, 0x0, 0x0 },
    /* 115 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 110.2f,
              NULL, NULL, (Gfx *)0x0104D2F8, 0x0, 0x0 },
    /* 116 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 612.1f,
              NULL, NULL, (Gfx *)0x0104DD50, 0x0, 0x0 },
    /* 117 */ { { 0.025f, 0.025f, 0.025f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 100.0f,
              NULL, NULL, NULL, 0x0, 0x0 },
    /* 118 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 169.9f,
              NULL, NULL, (Gfx *)0x0104E510, 0x0, 0x0 },
    /* 119 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 885.6f,
              NULL, NULL, (Gfx *)0x0104E748, 0x0, 0x0 },
    /* 120 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 250.0f,
              NULL, NULL, (Gfx *)0x0104E9E0, 0x0, 0x0 },
    /* 121 */ { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 120.0f,
              NULL, NULL, (Gfx *)0x0104EDE0, 0x0, 0x0 },
};

/// Per-kind item motion, rendering, response and description configuration.
/// \note Original identifier: \c D_80125A40 (0x80125A40).
ItemKind gItemKinds[166] = {
    /* 0x00: ? */
    { 0x00, 0x400, 0x0, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0x108, 0x6E, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, NULL, { 0x0, 0x0, 0x0, 0x0 },
      { { "?", "?" },
        { "?",
          "?" } },
      0x0, 0x1, 0x0, -1 },
    /* 0x01: apple */
    { 0x01, 0x2008, 0x5, 0x3, 1.0f, 0.1f, { 0xD, 0x11, 0x2, 0x7, 0xD },
      1.0f, 180.0f, 1.0f, 1.0f, 0.6f, 64.0f, 55.0f, 0x108, 0x3, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[0], { 0x14, 0xA, 0xA, 0x3C },
      { { "apple", NULL },
        { "apple",
          "A sweet, red fruit\n^d\"One of ^6PIKACHU^d's favorites.\n^dI wonder... ^4How's it taste^d?\"^^" } },
      0xC8, 0x2, 0x1, -1 },
    /* 0x02: green apple */
    { 0x02, 0x2008, 0x5, 0x3, 1.0f, 0.1f, { 0x11, 0x4, 0x4, 0x7, 0xF },
      1.0f, 180.0f, 1.0f, 1.0f, 0.6f, 75.0f, 114.0f, 0x108, 0x5, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[0], { 0x0, 0xFF, 0x3C, 0x78 },
      { { "apple", NULL },
        { "green apple",
          "An ^4apple^^ that hasn't ripened\n^d\"I bet it's sour. Maybe a\n^4Thundershock^d will help.\"^^" } },
      0x50, 0x0, 0x1, -1 },
    /* 0x03: baked apple */
    { 0x03, 0x8, 0x5, 0x0, 1.0f, 0.1f, { 0x2, 0x12, 0x2, 0x8, 0x2 },
      1.0f, 180.0f, 1.0f, 1.0f, 0.6f, 75.0f, 114.0f, 0x108, 0x6, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[0], { 0x14, 0x1E, 0x1E, 0xB4 },
      { { "apple", NULL },
        { "baked apple",
          "An ^4apple^^ that's been baked\n^d\"I wonder... ^4How's it taste^d?\"^^\n" } },
      0xFA, 0x1, 0x1, -1 },
    /* 0x04: golden apple */
    { 0x04, 0x2008, 0x5, 0x3, 1.0f, 0.1f, { 0x10, 0x11, 0x2, 0x9, 0x2 },
      1.0f, 180.0f, 1.0f, 1.0f, 0.6f, 75.0f, 114.0f, 0x108, 0x4, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[0], { 0xC8, 0xFF, 0x50, 0x78 },
      { { "apple", NULL },
        { "golden apple",
          "A shiny, gold-colored ^4apple^^\n^d\"It's a rare ^4apple^d - don't\n^dlet ^6PIKACHU^d throw it away.\"^^" } },
      0x1F4, 0x2, 0x1, -1 },
    /* 0x05: peach */
    { 0x05, 0x2008, 0x4, 0x5, 1.0f, 0.1f, { 0xD, 0xF, 0x2, 0x7, 0xD },
      1.0f, 150.0f, 1.0f, 1.0f, 0.7f, 71.0f, 105.0f, 0x108, 0x29, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[17], { 0x0, 0x0, 0x0, 0x0 },
      { { "peach", NULL },
        { "peach",
          "A sweet, juicy fruit\n^d\"^6PIKACHU^d loves this flavor,\n^dso tell it to ^4taste it^d.\"^^" } },
      0xC8, 0x2, 0x1, -1 },
    /* 0x06: strawberry */
    { 0x06, 0x10, 0x3, 0x0, 1.0f, 0.1f, { 0x10, 0x11, 0x2, 0x7, 0x10 },
      1.0f, 140.0f, 1.0f, 1.0f, 2.0f, 67.0f, 100.0f, 0x108, 0x24, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[11], { 0x0, 0x0, 0x0, 0x0 },
      { { "strawberry", NULL },
        { "strawberry",
          "A tangy, sweet red fruit\n^d\"^6PIKACHU^d loves this fruit,\n^dso tell it to ^4taste it^d.\"^^" } },
      0xDC, 0x2, 0x1, -1 },
    /* 0x07: raspberry */
    { 0x07, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0x5, 0x5, 0x5, 0x7, 0x5 },
      1.0f, 60.0f, 1.0f, 1.0f, 3.0f, 70.0f, 90.0f, 0x108, 0x3B, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[26], { 0x0, 0x0, 0x0, 0x0 },
      { { "raspberry", NULL },
        { "raspberry",
          "A deep red fruit\n^d\"It looks very sweet.\"^^\n" } },
      0x96, 0x2, 0x1, -1 },
    /* 0x08: banana */
    { 0x08, 0x1000, 0x3, 0x4D, 1.0f, 0.1f, { 0x10, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 210.0f, 1.0f, 1.0f, 2.0f, 55.0f, 100.0f, 0x108, 0x9, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[49], { 0x0, 0x0, 0x0, 0x0 },
      { { "banana", NULL },
        { "banana",
          "A tropical fruit\n^d\"Be sure to throw the\n^dpeel away.\"^^" } },
      0xDC, 0x2, 0x1, 0xCC },
    /* 0x09: cattail */
    { 0x09, 0x0, 0xA, 0x0, 1.0f, 0.1f, { 0x3, 0x3, 0x3, 0x1, 0x3 },
      1.0f, 310.0f, 1.0f, 1.0f, 2.0f, 65.0f, 152.0f, 0x108, 0x45, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[30], { 0x0, 0x0, 0x0, 0x0 },
      { { "cattail", NULL },
        { "cattail",
          "A tall, grassy reed\n^d\"Watch where you ^4throw it^d -^^\n^dit can damage the ^4flower^ds.\"^^" } },
      0x32, 0x1, 0x1, -1 },
    /* 0x0A: wild herb */
    { 0x0A, 0x0, 0x2, 0x0, 0.5f, 0.02f, { 0x7, 0x4, 0x1, 0x7, 0xF },
      1.5f, 40.0f, 0.2f, 0.5f, 3.0f, 50.0f, 68.0f, 0x108, 0x3C, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[80], { 0xFF, 0xF0, 0xF0, 0x80 },
      { { "wild herb", NULL },
        { "wild herb",
          "A sweet smelling ^4leaf^^\n^d\"Boil ^4carrot^ds and ^4onion^ds with\n^dit for a tasty dish.\"^^" } },
      0x14, 0x1, 0x1, -1 },
    /* 0x0B: corn */
    { 0x0B, 0x2000, 0x3, 0xC, 1.0f, 0.1f, { 0x12, 0x8, 0x2, 0x8, 0x12 },
      1.0f, 180.0f, 1.0f, 1.0f, 1.0f, 84.0f, 120.0f, 0x108, 0x38, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[63], { 0x0, 0x0, 0x0, 0x0 },
      { { "corn", NULL },
        { "corn",
          "A bright, yellow grain\n^d\"You'll get a tasty surprise\n^dif you roast it.\"^^" } },
      0xB4, 0x2, 0x1, 0xE0 },
    /* 0x0C: popcorn */
    { 0x0C, 0x0, 0x2, 0x0, 1.2f, 0.2f, { 0xD, 0x2, 0x2, 0x7, 0xD },
      0.9f, 180.0f, 1.0f, 1.0f, 1.0f, 99.0f, 118.0f, 0x108, 0x4A, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[64], { 0x0, 0x0, 0x0, 0x0 },
      { { "popcorn", NULL },
        { "popcorn",
          "Popped corn\n^d\"It sure smells delicious.\n^6PIKACHU^d would love to ^4eat it^d!\"^^" } },
      0xFA, 0x2, 0x1, -1 },
    /* 0x0D: pumpkin */
    { 0x0D, 0x10, 0xA, 0x0, 1.0f, 0.1f, { 0x2, 0xF, 0x2, 0x1, 0xF },
      1.0f, 190.0f, 1.0f, 1.0f, 1.0f, 72.0f, 94.0f, 0x108, 0x25, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[65], { 0xFF, 0x78, 0x0, 0x50 },
      { { "pumpkin", NULL },
        { "pumpkin",
          "A hard, heavy vegetable\n^d\"You have them on Halloween,\n^dbut some people eat them!\"^^" } },
      0xFA, 0x1, 0x1, 0xE1 },
    /* 0x0E: turnip */
    { 0x0E, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0x3, 0x11, 0x3, 0x1, 0xF },
      1.0f, 210.0f, 1.0f, 1.0f, 1.0f, 61.0f, 128.0f, 0x108, 0x26, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[96], { 0x0, 0x0, 0x0, 0x0 },
      { { "turnip", NULL },
        { "turnip",
          "A member of the radish family\n^d\"This is a cooking ingredient.\n^dIt tastes great when boiled.\"^^" } },
      0xB4, 0x1, 0x1, 0xE2 },
    /* 0x0F: cabbage */
    { 0x0F, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0x3, 0x11, 0x3, 0x1, 0xF },
      1.0f, 160.0f, 1.0f, 1.0f, 1.0f, 98.0f, 98.0f, 0x108, 0x28, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[97], { 0x0, 0x0, 0x0, 0x0 },
      { { "cabbage", NULL },
        { "cabbage",
          "A green vegetable\n^d\"This is often cooked\n^dwith other vegetables.\"^^" } },
      0xB4, 0x1, 0x1, 0xE3 },
    /* 0x10: carrot */
    { 0x10, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0x2, 0xF, 0x2, 0x1, 0xF },
      1.0f, 250.0f, 1.0f, 1.0f, 2.0f, 72.0f, 140.0f, 0x108, 0x39, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[99], { 0x0, 0x0, 0x0, 0x0 },
      { { "carrot", NULL },
        { "carrot",
          "A long, orange vegetable\n^d\"^4Carrot^ds are full of vitamins.\n^6PIKACHU^d loves to eat them.\"^^" } },
      0xB4, 0x2, 0x1, 0xE4 },
    /* 0x11: onion */
    { 0x11, 0x10, 0x3, 0x0, 1.0f, 0.1f, { 0xF, 0xB, 0x4, 0xB, 0xF },
      1.0f, 180.0f, 1.0f, 1.0f, 1.0f, 90.0f, 100.0f, 0x108, 0x42, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[100], { 0x0, 0x0, 0x0, 0x0 },
      { { "onion", NULL },
        { "onion",
          "A strong-tasting vegetable\n^d\"It's often used in stews.\n^4How's it smell^d?\"^^" } },
      0xB4, 0x0, 0x1, 0xE5 },
    /* 0x12: sweet potato */
    { 0x12, 0x2000, 0x3, 0x13, 1.0f, 0.1f, { 0x12, 0x3, 0x4, 0x1, 0x12 },
      1.0f, 250.0f, 1.0f, 1.0f, 2.0f, 75.0f, 134.0f, 0x108, 0x3D, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[44], { 0x80, 0x50, 0x20, 0x64 },
      { { "sweet potato", NULL },
        { "sweet potato",
          "A sweet, purple potato\n^d\"It tastes best after being\n^dbaked with ^4Thunderbolt^d.\"^^" } },
      0xB4, 0x1, 0x1, 0xE6 },
    /* 0x13: hot sweet potato */
    { 0x13, 0x0, 0x2, 0x0, 1.2f, 0.2f, { 0xD, 0x7, 0x2, 0x7, 0xD },
      1.0f, 250.0f, 1.0f, 1.0f, 2.0f, 75.0f, 134.0f, 0x108, 0x49, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[44], { 0x0, 0x0, 0x0, 0x60 },
      { { "sweet potato", NULL },
        { "hot sweet potato",
          "A cooked sweet potato\n^d\"Tell ^6PIKACHU^d to ^4taste it^d.\"^^\n" } },
      0xFA, 0x2, 0x1, -1 },
    /* 0x14: smelly fruit */
    { 0x14, 0x2010, 0x3, 0x15, 1.0f, 0.1f, { 0x14, 0x8, 0x14, 0x14, 0x14 },
      1.0f, 150.0f, 1.0f, 1.0f, 1.0f, 64.0f, 108.0f, 0x108, 0x18, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[98], { 0x0, 0x0, 0x0, 0x0 },
      { { "smelly fruit", NULL },
        { "smelly fruit",
          "A strange, ^4smelly fruit^^\n^d\"I bet ^6PIKACHU^d would hate\n^dsomething this smelly.\"^^" } },
      0x96, 0x0, 0x1, -1 },
    /* 0x15: sticky fruit */
    { 0x15, 0x2010, 0x3, 0x16, 1.0f, 0.1f, { 0x14, 0x8, 0x14, 0x14, 0x14 },
      1.0f, 119.0f, 1.0f, 1.0f, 1.0f, 70.0f, 90.0f, 0x108, 0x17, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[42], { 0x0, 0x0, 0x0, 0x0 },
      { { "sticky fruit", NULL },
        { "sticky fruit",
          "Cooked ^4smelly fruit^^\n^d\"It still smells, and it's sticky\n^dtoo. It needs more cooking.\"^^" } },
      0xB4, 0x0, 0x1, -1 },
    /* 0x16: roasted fruit */
    { 0x16, 0x10, 0x3, 0x0, 1.2f, 0.2f, { 0xE, 0x8, 0xE, 0x8, 0xE },
      1.0f, 119.0f, 1.0f, 1.0f, 1.0f, 70.0f, 90.0f, 0x108, 0x19, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[42], { 0xA0, 0x50, 0x10, 0x80 },
      { { "roasted fruit", NULL },
        { "roasted fruit",
          "Cooked ^4sticky fruit^^\n^d\"It still smells bad, but it\n^dtastes great. How strange.\"^^" } },
      0x104, 0x2, 0x1, -1 },
    /* 0x17: walnut */
    { 0x17, 0x6008, 0x5, 0x18, 1.0f, 0.2f, { 0x11, 0xF, 0x1, 0x7, 0xF },
      1.0f, 195.0f, 1.0f, 0.8f, 1.0f, 74.0f, 110.0f, 0x108, 0x27, { 0x97, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[41], { 0x0, 0x0, 0x0, 0x0 },
      { { "walnut", NULL },
        { "walnut",
          "A hard-shelled nut\n^d\"^4Throw it^d to crack its shell.\"^^\n" } },
      0xC8, 0x0, 0x1, -1 },
    /* 0x18: cracked walnut */
    { 0x18, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0xD, 0x7, 0x2, 0x7, 0xD },
      1.0f, 184.0f, 1.0f, 1.0f, 1.0f, 70.0f, 110.0f, 0x108, 0x2A, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[45], { 0x0, 0x0, 0x0, 0x0 },
      { { "walnut", NULL },
        { "cracked walnut",
          "A nutritious nut\n^d\"It's tasty, and it's\n^doften added to sweets.\"^^" } },
      0x104, 0x2, 0x1, -1 },
    /* 0x19: spiky shell */
    { 0x19, 0x6008, 0x6, 0x1B, 1.0f, 0.1f, { 0x12, 0xF, 0x1, 0x1, 0xF },
      1.0f, 180.0f, 1.0f, 0.8f, 1.0f, 75.0f, 117.0f, 0x108, 0x23, { 0x97, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[55], { 0x0, 0x0, 0x0, 0x0 },
      { { "spiky shell", NULL },
        { "spiky shell",
          "The ^4spiky shell^^ of a nut\n^d\"Some people eat these after\n^dremoving the shell.\"^^" } },
      0xC8, 0x0, 0x1, -1 },
    /* 0x1A: spiny shell */
    { 0x1A, 0x6008, 0x6, 0x1C, 1.0f, 0.1f, { 0xF, 0xF, 0x1, 0x8, 0xF },
      1.0f, 180.0f, 1.0f, 0.8f, 1.0f, 75.0f, 117.0f, 0x108, 0x23, { 0x97, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[55], { 0x20, 0x0, 0x20, 0x6C },
      { { "spiny shell", NULL },
        { "spiny shell",
          "A strange, ^4spiny shell^^\n^d\"For some reason, these are\n^dscattered along the beach.\"^^" } },
      0xC8, 0x0, 0x1, -1 },
    /* 0x1B: chestnut */
    { 0x1B, 0x10, 0x2, 0x0, 1.0f, 0.1f, { 0x2, 0x12, 0x2, 0x7, 0x2 },
      1.0f, 110.0f, 0.8f, 1.2f, 1.0f, 72.0f, 72.0f, 0x108, 0x2, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[43], { 0x0, 0x0, 0x0, 0x0 },
      { { "chestnut", NULL },
        { "chestnut",
          "A tasty, sweet nut\n^d\"Mix this with ^4roasted fruit^^\n^dfor a really tasty treat.\"^^" } },
      0x104, 0x2, 0x1, -1 },
    /* 0x1C: beach nut */
    { 0x1C, 0x10, 0x3, 0x0, 1.2f, 0.2f, { 0x2, 0x12, 0x2, 0x7, 0x2 },
      1.0f, 119.0f, 1.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x19, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[42], { 0xFF, 0x80, 0x0, 0xA0 },
      { { "chestnut", NULL },
        { "beach nut",
          "A nut found on the beach\n^d\"A ^4beach nut^d?\n^dIs that right?\"^^" } },
      0x168, 0x1, 0x1, -1 },
    /* 0x1D: coconut */
    { 0x1D, 0x2008, 0x14, 0x1D, 1.0f, 0.1f, { 0xF, 0x14, 0x1, 0x1, 0xF },
      1.0f, 180.0f, 1.0f, 1.0f, 0.4f, 74.0f, 110.0f, 0x108, 0x4B, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[35], { 0x0, 0x0, 0x0, 0x0 },
      { { "coconut", NULL },
        { "coconut",
          "A large, hard-shelled fruit\n^d\"When I think of the tropics,\n^dI think ^4coconut^ds.\"^^" } },
      0x12C, 0x1, 0x1, -1 },
    /* 0x1E: acorn */
    { 0x1E, 0x2010, 0xA, 0x22, 1.0f, 0.1f, { 0x12, 0x3, 0x3, 0x1, 0x3 },
      1.0f, 150.0f, 1.0f, 1.0f, 1.0f, 75.0f, 96.0f, 0x108, 0x12, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[50], { 0x0, 0x0, 0x0, 0x0 },
      { { "acorn", NULL },
        { "acorn",
          "The nut of an oak tree\n^d\"This looks like just a\n^dplain old ^4acorn^d.\"^^" } },
      0x28, 0x1, 0x1, -1 },
    /* 0x1F: acorn */
    { 0x1F, 0x2010, 0xA, 0x22, 1.0f, 0.1f, { 0x11, 0x4, 0x4, 0x8, 0x11 },
      1.0f, 150.0f, 1.0f, 1.0f, 1.0f, 75.0f, 96.0f, 0x108, 0x13, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[50], { 0xFF, 0xFF, 0x80, 0x10 },
      { { "acorn", NULL },
        { "acorn",
          "The nut of an oak tree\n^d\"It will taste good after\n^dbaking it with ^4Thunderbolt^d.\"^^" } },
      0x23, 0x1, 0x1, -1 },
    /* 0x20: acorn */
    { 0x20, 0x2010, 0xA, 0x22, 1.0f, 0.1f, { 0x11, 0xC, 0x4, 0xC, 0xC },
      1.0f, 150.0f, 1.0f, 1.0f, 1.0f, 75.0f, 96.0f, 0x108, 0x14, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[50], { 0x0, 0x80, 0xFF, 0x10 },
      { { "acorn", NULL },
        { "acorn",
          "The nut of an oak tree\n^d\"It has sleeping powder on it.\n^dTell ^6PIKACHU^d to ^4drop it^d.\"^^" } },
      0x1E, 0x1, 0x1, -1 },
    /* 0x21: acorn */
    { 0x21, 0x2010, 0xA, 0x22, 1.0f, 0.1f, { 0x12, 0x4, 0x4, 0x1, 0x4 },
      1.0f, 150.0f, 1.0f, 1.0f, 1.0f, 75.0f, 96.0f, 0x108, 0x11, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[50], { 0xFF, 0x0, 0x0, 0x10 },
      { { "acorn", NULL },
        { "acorn",
          "The nut of an oak tree\n^d\"It will taste good after\n^dcooking it with ^4Thundershock^d.\"^^" } },
      0x28, 0x1, 0x1, -1 },
    /* 0x22: toasted acorn */
    { 0x22, 0x10, 0xA, 0x0, 1.0f, 0.2f, { 0xE, 0x8, 0xE, 0x8, 0xE },
      1.0f, 150.0f, 1.0f, 1.0f, 1.0f, 75.0f, 96.0f, 0x108, 0x15, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[50], { 0x28, 0x14, 0x8, 0x64 },
      { { "acorn", NULL },
        { "toasted acorn",
          "A cooked ^4acorn^^\n^d\"It smells bad but tastes\n^dgreat. Use it to make treats.\"^^" } },
      0xA0, 0x2, 0x1, -1 },
    /* 0x23: flying acorn */
    { 0x23, 0x0, 0xA, 0x0, 1.0f, 0.1f, { 0xF, 0x4, 0x4, 0x1, 0xF },
      1.0f, 70.0f, 0.2f, 0.8f, 4.0f, 69.0f, 172.0f, 0x108, 0xF, { 0x97, 0x97 },
      NULL, NULL, itemInitializeFlightSpin, itemResolveMapCollision, &gItemModelTemplates[56], { 0x0, 0x0, 0x0, 0x0 },
      { { "flying acorn", NULL },
        { "flying acorn",
          "An ^4acorn^^ with wings on it\n^d\"This flies very well. ^4Throw it\n^dat things hanging in trees.\"^^" } },
      0x12C, 0x1, 0x3, 0x1D },
    /* 0x24: acorn top */
    { 0x24, 0x30, 0x14, 0x0, 1.0f, 0.1f, { 0xF, 0xB, 0x2, 0xB, 0xF },
      1.0f, 120.0f, 1.0f, 1.0f, 1.0f, 64.0f, 106.0f, 0x108, 0x10, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[57], { 0x0, 0x0, 0x0, 0x0 },
      { { "acorn top", NULL },
        { "acorn top",
          "A striped ^4acorn^^\n^d\"This ^4acorn^d spins just like\n^da top.\"^^" } },
      0xFA, 0x2, 0x1, 0xD8 },
    /* 0x25: jumbo'shroom */
    { 0x25, 0x2010, 0xA, 0x26, 1.0f, 0.1f, { 0x12, 0x8, 0x4, 0x8, 0x12 },
      0.9f, 186.9f, 1.0f, 1.0f, 1.0f, 93.45f, 93.45f, 0x108, 0x2F, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[19], { 0x0, 0x0, 0x0, 0x0 },
      { { "mushroom", NULL },
        { "jumbo'shroom",
          "An ^4acorn^^-like ^4mushroom^^\n^d\"It's delicious when cooked\n^dusing ^4Thunderbolt^d.\"^^" } },
      0x2D, 0x1, 0x1, -1 },
    /* 0x26: roasted jumbo */
    { 0x26, 0x10, 0x3, 0x0, 1.0f, 0.1f, { 0xD, 0x2, 0x2, 0x7, 0xD },
      1.0f, 186.0f, 1.0f, 1.0f, 1.0f, 63.0f, 93.0f, 0x108, 0x30, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[19], { 0x28, 0x0, 0x0, 0x8C },
      { { "mushroom", NULL },
        { "roasted jumbo",
          "A cooked ^4jumbo'shroom^^\n^d\"^6PIKACHU^d should like this\n^ddelicious cooked ^4mushroom^d.\"^^" } },
      0xF0, 0x2, 0x1, -1 },
    /* 0x27: dreamy'shroom */
    { 0x27, 0x14, 0xA, 0x0, 1.0f, 0.1f, { 0xF, 0xC, 0x4, 0xC, 0xC },
      1.1f, 170.0f, 0.2f, 0.4f, 2.0f, 96.2f, 96.2f, 0x108, 0x31, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[20], { 0x0, 0x0, 0x0, 0x0 },
      { { "mushroom", NULL },
        { "dreamy'shroom",
          "A light, airy ^4mushroom^^\n^d\"It's filled with sleeping gas.\n^dI wonder... ^4How's it smell^d?\"^^" } },
      0x2D, 0x1, 0x1, -1 },
    /* 0x28: truffle top */
    { 0x28, 0x220, 0x14, 0x0, 1.0f, 0.1f, { 0xF, 0x14, 0x14, 0x8, 0x14 },
      0.7f, 170.0f, 1.0f, 1.0f, 1.0f, 96.2f, 96.2f, 0x108, 0x33, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[20], { 0xAA, 0x50, 0x14, 0x64 },
      { { "mushroom", NULL },
        { "truffle top",
          "A small, hard ^4mushroom^^\n^d\"This spins just like a top.\n^4Throw it^d to make it spin.\"^^" } },
      0x78, 0x1, 0x1, 0xEE },
    /* 0x29: blue'shroom */
    { 0x29, 0x2010, 0x14, 0x2A, 0.5f, 0.05f, { 0xF, 0x4, 0x4, 0x8, 0xF },
      1.0f, 180.0f, 1.0f, 1.0f, 1.0f, 80.0f, 100.0f, 0x108, 0x32, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[22], { 0x0, 0x20, 0xFF, 0x60 },
      { { "mushroom", NULL },
        { "blue'shroom",
          "A blue-capped ^4mushroom^^\n^d\"This doesn't taste very good.\n^dCook to improve its flavor.\"^^" } },
      0x78, 0x0, 0x1, -1 },
    /* 0x2A: hot'shroom */
    { 0x2A, 0x10, 0x5, 0x0, 1.2f, 0.15f, { 0xE, 0x8, 0xE, 0x8, 0xE },
      1.1f, 180.0f, 1.0f, 1.0f, 1.0f, 80.0f, 100.0f, 0x108, 0x35, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[22], { 0x0, 0x0, 0x0, 0xA0 },
      { { "mushroom", NULL },
        { "hot'shroom",
          "A cooked ^4mushroom^^\n^d\"This tastes great when mixed\n^dwith other ingredients.\"^^" } },
      0x118, 0x1, 0x1, -1 },
    /* 0x2B: red'shroom */
    { 0x2B, 0x2010, 0x14, 0x2A, 0.5f, 0.05f, { 0x14, 0x6, 0x6, 0xA, 0x14 },
      1.0f, 180.0f, 1.0f, 1.0f, 1.0f, 80.0f, 100.0f, 0x108, 0x34, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[22], { 0x0, 0x0, 0x0, 0x0 },
      { { "mushroom", NULL },
        { "red'shroom",
          "A red-capped ^4mushroom^^\n^d\"This strange ^4mushroom^d\n^dexplodes if you try to ^4eat it^d.\"^^" } },
      0x78, 0x0, 0x1, -1 },
    /* 0x2C: sunny'shroom */
    { 0x2C, 0x2010, 0x14, 0x2A, 0.5f, 0.05f, { 0x10, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 180.0f, 1.0f, 1.0f, 1.0f, 80.0f, 100.0f, 0x108, 0x37, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[22], { 0xFF, 0xFF, 0x0, 0x46 },
      { { "mushroom", NULL },
        { "sunny'shroom",
          "A yellow-capped ^4mushroom^^\n^d\"It's yellow, like the sun.\n^6PIKACHU^d seems to like it.\"^^" } },
      0x8C, 0x0, 0x1, -1 },
    /* 0x2D: mushroom */
    { 0x2D, 0x2010, 0x3, 0x30, 1.0f, 0.1f, { 0x12, 0x3, 0x3, 0x1, 0x12 },
      1.0f, 174.3f, 1.0f, 1.0f, 1.0f, 90.0f, 90.0f, 0x108, 0x2B, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[18], { 0x0, 0x0, 0x0, 0x0 },
      { { "mushroom", NULL },
        { "mushroom",
          "A plain, white ^4mushroom^^\n^d\"This is just a plain old\n^4mushroom^d.\"^^" } },
      0x2D, 0x2, 0x1, -1 },
    /* 0x2E: mushroom */
    { 0x2E, 0x2010, 0x14, 0x30, 1.0f, 0.1f, { 0x12, 0x8, 0x1, 0x8, 0x12 },
      1.0f, 174.0f, 1.0f, 1.0f, 1.0f, 90.0f, 90.0f, 0x108, 0x2D, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[18], { 0xFF, 0xDC, 0x3C, 0x50 },
      { { "mushroom", NULL },
        { "mushroom",
          "A plain, white ^4mushroom^^\n^d\"This ^4mushroom^d smells.\"^^\n" } },
      0x2D, 0x1, 0x1, -1 },
    /* 0x2F: mushroom */
    { 0x2F, 0x2010, 0x14, 0x30, 1.0f, 0.1f, { 0x12, 0xB, 0x4, 0xB, 0xB },
      1.0f, 174.0f, 1.0f, 1.0f, 1.0f, 90.0f, 90.0f, 0x108, 0x2C, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[18], { 0x0, 0x0, 0xFF, 0x1E },
      { { "mushroom", NULL },
        { "mushroom",
          "A plain white ^4mushroom^^\n^d\"This ^4mushroom^d tends to\n^drelease spores.\"^^" } },
      0x28, 0x0, 0x1, -1 },
    /* 0x30: hot mushroom */
    { 0x30, 0x10, 0x3, 0x0, 1.2f, 0.2f, { 0xD, 0x2, 0x2, 0x7, 0xD },
      1.1f, 174.0f, 1.0f, 1.0f, 1.0f, 90.0f, 90.0f, 0x108, 0x2E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[18], { 0x80, 0x50, 0x20, 0x64 },
      { { "mushroom", NULL },
        { "hot mushroom",
          "A cooked ^4mushroom^^\n^d\"This ^4mushroom^d has a\n^ddelicious rich scent.\"^^" } },
      0xBE, 0x1, 0x1, -1 },
    /* 0x31: light bulb */
    { 0x31, 0x200, 0x3, 0x0, 1.0f, 0.1f, { 0xF, 0x10, 0x1, 0x1, 0x1 },
      1.0f, 160.0f, 1.0f, 1.0f, 2.0f, 90.0f, 132.0f, 0xF3, 0xD, { 0x97, 0x97 },
      lightBulbUpdateHeldGlow, lightBulbResetReleasedGlow, NULL, itemResolveMapCollision, &gItemModelTemplates[21], { 0x0, 0x0, 0x0, 0x0 },
      { { "light bulb", NULL },
        { "light bulb",
          "A strange, electric ^4mushroom^^\n^d\"It lights up when ^6PIKACHU^d\n^dholds it. Illuminating!\"^^" } },
      0xF0, 0x0, 0x1, 0xED },
    /* 0x32: cupcake */
    { 0x32, 0x0, 0x3, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 115.0f, 1.0f, 0.2f, 2.0f, 89.0f, 86.0f, 0x108, 0x3A, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[118], { 0x0, 0x0, 0x0, 0x0 },
      { { "cupcake", NULL },
        { "cupcake",
          "Mom's homemade treat\n^d\"Your mom made this snack\n^dfor you. It looks delicious.\"^^" } },
      0x0, 0x2, 0x3, 0x1B },
    /* 0x33: daisy */
    { 0x33, 0x0, 0x2, 0x0, 1.0f, 0.1f, { 0x9, 0x4, 0x4, 0x9, 0x9 },
      1.0f, 180.0f, 0.5f, 0.5f, 4.0f, 70.0f, 125.0f, 0x108, 0x1C, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[6], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "daisy",
          "A small, white ^4flower^^\n^d\"It doesn't stand out much,\n^dbut it's a cute flower.\"^^" } },
      0x78, 0x2, 0x1, 0xC4 },
    /* 0x34: petunia */
    { 0x34, 0x0, 0x2, 0x0, 1.0f, 0.1f, { 0x9, 0x4, 0x4, 0x9, 0x9 },
      1.0f, 274.1f, 0.5f, 0.5f, 4.0f, 60.0f, 137.05f, 0x108, 0x41, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[7], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "petunia",
          "A white ^4flower^^\n^d\"What an elegant looking\n^4flower^d.\"^^" } },
      0xC8, 0x2, 0x1, 0xDA },
    /* 0x35: tulip */
    { 0x35, 0x0, 0x2, 0x0, 1.0f, 0.1f, { 0x3, 0xB, 0x3, 0xB, 0xB },
      1.0f, 272.1f, 0.5f, 0.5f, 4.0f, 60.0f, 136.05f, 0x108, 0x47, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[8], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "tulip",
          "A ^4flower^^ grown from a bulb\n^d\"This is a very lovely ^4flower^d.\"^^\n" } },
      0x12C, 0x1, 0x1, 0xDB },
    /* 0x36: dandelion */
    { 0x36, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0x10, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 150.0f, 0.5f, 0.5f, 4.0f, 80.0f, 100.0f, 0x108, 0x43, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[28], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "dandelion",
          "A yellow spring blossom\n^d\"^6PIKACHU^d loves this, since\n^dit's yellow.\"^^" } },
      0x64, 0x2, 0x1, 0xDC },
    /* 0x37: bluebell */
    { 0x37, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0x3, 0xB, 0x3, 0xB, 0xB },
      1.0f, 252.5f, 0.5f, 0.5f, 4.0f, 80.0f, 120.0f, 0x108, 0x7, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[1], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "bluebell",
          "A bright blue ^4flower^^\n^d\"This is a popular ^4flower^d\n^dfor the garden.\"^^" } },
      0x78, 0x2, 0x1, 0xDD },
    /* 0x38: skunk blossom */
    { 0x38, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0xE, 0x8, 0xE, 0x8, 0x8 },
      1.0f, 310.0f, 0.5f, 0.5f, 4.0f, 80.0f, 150.0f, 0x108, 0xE, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[5], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "skunk blossom",
          "A plant with smelly leaves\n^d\"It smells bad, but you can\n^dactually ^4eat it^d.\"^^" } },
      0xC8, 0x0, 0x1, 0xDE },
    /* 0x39: sunflower */
    { 0x39, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0x10, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 360.0f, 0.5f, 0.5f, 4.0f, 80.0f, 160.0f, 0x108, 0x1B, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[10], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "sunflower",
          "A large, yellow summer ^4flower^^\n^d\"It's the same color of\n^dyellow as ^6PIKACHU^d.\"^^" } },
      0xF0, 0x2, 0x1, 0xDF },
    /* 0x3A: lily */
    { 0x3A, 0x0, 0x3, 0x0, 1.0f, 0.1f, { 0x9, 0x3, 0x3, 0x9, 0x9 },
      1.0f, 300.0f, 0.5f, 0.5f, 4.0f, 80.0f, 160.0f, 0x108, 0x4C, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[29], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "lily",
          "A large, white ^4flower^^\n^d\"I think this one smells nice,\n^dbut I am not quite sure.\"^^" } },
      0x15E, 0x2, 0x1, 0xC5 },
    /* 0x3B: rosebud */
    { 0x3B, 0x6000, 0x1, 0x3C, 1.0f, 0.1f, { 0x12, 0x4, 0x4, 0x12, 0xF },
      1.0f, 100.0f, 0.4f, 0.6f, 4.0f, 70.0f, 110.0f, 0x108, 0xC, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[111], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "rosebud",
          "A large red ^4flower^^ from a bush\n^d\"Feed it to a hungry\n^4CATERPIE^d.\"^^" } },
      0x1E, 0x1, 0x1, -1 },
    /* 0x3C: red rose */
    { 0x3C, 0x0, 0xA, 0x3C, 1.0f, 0.1f, { 0x9, 0x4, 0x4, 0x9, 0xF },
      1.0f, 100.0f, 0.4f, 0.5f, 6.0f, 70.0f, 110.0f, 0x108, 0xB, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[110], { 0x0, 0x0, 0x0, 0x0 },
      { { "flower", NULL },
        { "red rose",
          "A large red ^4flower^^ from a bush\n^d\"Feed the ^4flower^ds to\n^4CATERPIE^d.\"^^" } },
      0x64, 0x2, 0x1, -1 },
    /* 0x3D: balloon */
    { 0x3D, 0x40006, 0xA, 0x0, 1.0f, 0.1f, { 0xA, 0x6, 0x6, 0xA, 0xF },
      1.0f, 29.0f, 0.8f, 0.5f, 0.4f, 64.0f, 110.0f, 0x108, 0x1E, { 0x98, 0x98 },
      balloonUpdateHeldInflation, NULL, balloonBeginReleasedFlight, itemResolveMapCollision, &gItemModelTemplates[58], { 0x0, 0x10, 0xFF, 0xC8 },
      { { "balloon", NULL },
        { "balloon",
          "A blue ^4balloon^^\n^d\"This strange ^4balloon^d inflates\n^dwhenever ^6PIKACHU^d holds it.\"^^" } },
      0x12C, 0x2, 0x1, 0xE8 },
    /* 0x3E: balloon */
    { 0x3E, 0x40006, 0xA, 0x0, 1.0f, 0.1f, { 0xA, 0x6, 0x6, 0xA, 0xF },
      1.0f, 29.0f, 0.8f, 0.5f, 0.4f, 64.0f, 110.0f, 0x108, 0x20, { 0x98, 0x98 },
      balloonUpdateHeldInflation, NULL, balloonBeginReleasedFlight, itemResolveMapCollision, &gItemModelTemplates[58], { 0x0, 0xFF, 0x0, 0xC8 },
      { { "balloon", NULL },
        { "balloon",
          "A green ^4balloon^^\n^d\"This strange ^4balloon^d inflates\n^dwhenever ^6PIKACHU^d holds it.\"^^" } },
      0x12C, 0x2, 0x1, 0xE9 },
    /* 0x3F: balloon */
    { 0x3F, 0x40006, 0xA, 0x0, 1.0f, 0.1f, { 0xA, 0x6, 0x6, 0xA, 0xF },
      1.0f, 29.0f, 0.8f, 0.5f, 0.4f, 64.0f, 110.0f, 0x108, 0x21, { 0x98, 0x98 },
      balloonUpdateHeldInflation, NULL, balloonBeginReleasedFlight, itemResolveMapCollision, &gItemModelTemplates[58], { 0xFF, 0x0, 0xFF, 0xB4 },
      { { "balloon", NULL },
        { "balloon",
          "A purple ^4balloon^^\n^d\"This strange ^4balloon^d inflates\n^dwhenever ^6PIKACHU^d holds it.\"^^" } },
      0x12C, 0x2, 0x1, 0xEA },
    /* 0x40: balloon */
    { 0x40, 0x40006, 0xA, 0x0, 1.0f, 0.1f, { 0xA, 0x6, 0x6, 0xA, 0xF },
      1.0f, 29.0f, 0.8f, 0.5f, 0.4f, 64.0f, 110.0f, 0x108, 0x22, { 0x98, 0x98 },
      balloonUpdateHeldInflation, NULL, balloonBeginReleasedFlight, itemResolveMapCollision, &gItemModelTemplates[58], { 0xFF, 0x10, 0x0, 0xC8 },
      { { "balloon", NULL },
        { "balloon",
          "A red ^4balloon^^\n^d\"This strange ^4balloon^d inflates\n^dwhenever ^6PIKACHU^d holds it.\"^^" } },
      0x12C, 0x2, 0x1, 0xEB },
    /* 0x41: balloon */
    { 0x41, 0x40006, 0xA, 0x0, 1.0f, 0.1f, { 0xA, 0x6, 0x6, 0xA, 0xF },
      1.0f, 29.0f, 0.8f, 0.5f, 0.4f, 64.0f, 110.0f, 0x108, 0x1F, { 0x98, 0x98 },
      balloonUpdateHeldInflation, NULL, balloonBeginReleasedFlight, itemResolveMapCollision, &gItemModelTemplates[58], { 0xFF, 0xFF, 0x0, 0xB4 },
      { { "balloon", NULL },
        { "balloon",
          "A yellow ^4balloon^^\n^d\"This strange ^4balloon^d inflates\n^dwhenever ^6PIKACHU^d holds it.\"^^" } },
      0x12C, 0x2, 0x1, 0xEC },
    /* 0x42: ? */
    { 0x42, 0x400, 0x1E, 0x0, 1.0f, 0.1f, { 0xF, 0x14, 0x1, 0x1, 0xF },
      1.0f, 40.0f, 0.2f, 0.6f, 10.0f, 75.0f, 75.0f, 0x108, 0x1D, { 0xC3, 0xC3 },
      NULL, NULL, itemInitializeFlightSpin, itemResolveMapCollision, &gItemModelTemplates[61], { 0x0, 0x0, 0x0, 0x0 },
      { { "Pok\xA1\xAD" "disc", NULL },
        { "Pok\xA1\xAD" "disc",
          "A disc that's fun to throw\n^d\"It spins and spins as it\n^dflies through the air.\"^^" } },
      0x1E0, 0x2, 0x1, 0xE7 },
    /* 0x43: beach ball */
    { 0x43, 0x40C, 0x1, 0x0, 1.0f, 0.1f, { 0xF, 0x12, 0x1, 0x1, 0xF },
      1.0f, 200.0f, 0.9f, 1.0f, 0.3f, 100.0f, 100.0f, 0x108, 0x4D, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[2], { 0x0, 0x0, 0x0, 0x0 },
      { { "beach ball", NULL },
        { "beach ball",
          "A colorful, bouncy ball\n^d\"It's fun to ^4throw it^d around.\n^dTry playing catch with it!\"^^" } },
      0x0, 0x1, 0x3, 0x1C },
    /* 0x44: watering can */
    { 0x44, 0x120400, 0x3, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      0.6f, 150.0f, 1.0f, 0.1f, 10.0f, 100.0f, 150.0f, 0x2, 0x5D, { 0x9F, 0xC3 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[13], { 0xC8, 0x0, 0x0, 0x80 },
      { { "watering can", NULL },
        { "watering can",
          "A can for watering plants\n^d\"Give this to ^6PIKACHU^d right\n^daway whenever it wants it.\"^^" } },
      0x0, 0x1, 0x3, 0x20 },
    /* 0x45: watering jug */
    { 0x45, 0x120400, 0x1E, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 130.0f, 1.0f, 0.1f, 10.0f, 100.0f, 150.0f, 0x2, 0x5D, { 0x9F, 0xC3 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[13], { 0x0, 0x0, 0xC8, 0x80 },
      { { "watering can", NULL },
        { "watering jug",
          "A jug that holds a lot of water\n^d\"With this, your water should\n^dlast you longer.\"^^" } },
      0x5DC, 0x1, 0x3, 0x21 },
    /* 0x46: yellow ball */
    { 0x46, 0x8, 0x3, 0x0, 0.2f, 0.01f, { 0x14, 0x8, 0x14, 0x8, 0x14 },
      1.0f, 84.0f, 1.0f, 1.0f, 1.0f, 60.0f, 88.0f, 0x108, 0x61, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[62], { 0x0, 0x0, 0x0, 0x0 },
      { { "yellow ball", NULL },
        { "yellow ball",
          "A yellow ball of unknown origin\n^d\"What on earth is this?\"^^\n" } },
      0x0, 0x0, 0x1, -1 },
    /* 0x47: charcoal */
    { 0x47, 0x0, 0x3, 0x0, 0.3f, 0.05f, { 0xF, 0x4, 0x4, 0x1, 0xF },
      0.5f, 84.0f, 1.0f, 1.0f, 1.0f, 90.0f, 140.0f, 0x108, 0x5F, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[62], { 0x40, 0x20, 0x0, 0xA0 },
      { { "charcoal", NULL },
        { "charcoal",
          "Burnt ashes\n^d\"It's the burnt remains of\n^dsome item. It's just garbage.\"^^" } },
      0x0, 0x0, 0x1, -1 },
    /* 0x48: leaf */
    { 0x48, 0x100C0, 0x2, 0x0, 0.5f, 0.02f, { 0x7, 0x4, 0x1, 0x7, 0xF },
      1.5f, 40.0f, 0.2f, 0.5f, 3.0f, 50.0f, 68.0f, 0x108, 0x3C, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[80], { 0x80, 0x0, 0x0, 0xC0 },
      { { "leaf", NULL },
        { "leaf",
          "A fallen, wilted leaf\n^d\"It's just a plain, wilted leaf.\"^^\n" } },
      0x5, 0x1, 0x1, -1 },
    /* 0x49: leaf whistle */
    { 0x49, 0x0, 0xA, 0x0, 1.0f, 0.1f, { 0x13, 0xF, 0x4, 0x1, 0x13 },
      1.0f, 40.0f, 0.2f, 0.5f, 4.0f, 70.0f, 70.0f, 0x108, 0x64, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[80], { 0x0, 0x0, 0x0, 0x0 },
      { { "leaf", NULL },
        { "leaf whistle",
          "A whistle made from a ^4leaf^^\n^d\"I wonder how you ^4play it^d.\"^^\n" } },
      0xA, 0x2, 0x1, -1 },
    /* 0x4A: harmonica */
    { 0x4A, 0x400, 0xA, 0x0, 1.0f, 0.1f, { 0x13, 0x8, 0x1, 0x8, 0x13 },
      1.0f, 10.0f, 1.0f, 0.5f, 4.0f, 45.0f, 120.0f, 0x108, 0x57, { 0xBE, 0xBE },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[54], { 0x0, 0x0, 0x0, 0x0 },
      { { "harmonica", NULL },
        { "harmonica",
          "A musical instrument\n^d\"I wonder if ^6PIKACHU^d can\n^4play it^d for you.\"^^" } },
      0x320, 0x2, 0x3, 0x22 },
    /* 0x4B: trumpet */
    { 0x4B, 0x400, 0xA, 0x0, 1.0f, 0.1f, { 0x13, 0x8, 0x1, 0x8, 0x13 },
      1.0f, 70.0f, 1.0f, 0.5f, 4.0f, 70.0f, 130.0f, 0x108, 0x69, { 0xBE, 0xBE },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[51], { 0x0, 0x0, 0x0, 0x0 },
      { { "trumpet", NULL },
        { "trumpet",
          "A loud, brass instrument\n^d\"This makes a pretty loud\n^dnoise.\"^^" } },
      0x4B0, 0x2, 0x1, 0xD9 },
    /* 0x4C: tissue */
    { 0x4C, 0x0, 0x1, 0x0, 0.4f, 0.05f, { 0xF, 0x12, 0x3, 0x1, 0xF },
      1.0f, 5.0f, 0.1f, 0.3f, 10.0f, 53.0f, 100.0f, 0x108, 0x44, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[101], { 0x0, 0x0, 0x0, 0x0 },
      { { "tissue", NULL },
        { "tissue",
          "Thin, light paper\n^d\"Be sure to ^4clean it up^d after\n^dscattering it about.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x4D: banana peel */
    { 0x4D, 0x0, 0x2, 0x0, 0.5f, 0.02f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 60.0f, 0.8f, 0.4f, 4.0f, 90.0f, 111.0f, 0x108, 0xA, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[32], { 0x0, 0x0, 0x0, 0x0 },
      { { "banana peel", NULL },
        { "banana peel",
          "The skin of a ^4banana^^\n^d\"^6PIKACHU^d can slip on this, so\n^dbe careful where you leave it.\"^^" } },
      0x0, 0x1, 0x1, -1 },
    /* 0x4E: ? */
    { 0x4E, 0x404A, 0x3, 0x4F, 1.0f, 0.1f, { 0xF, 0x12, 0x1, 0x1, 0xF },
      1.0f, 1289.0f, 3.0f, 0.5f, 0.8f, 64.0f, 144.0f, 0x108, 0x40, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[67], { 0x0, 0x0, 0x0, 0x0 },
      { { "pi\xA1\xB1" "ata", NULL },
        { "pi\xA1\xB1" "ata",
          "A decorated container\n^d\"If you break it, ^6PIKACHU^d\n^dfinds ^4treasure^d inside.\"^^" } },
      0x9B0, 0x2, 0x1, -1 },
    /* 0x4F: ? */
    { 0x4F, 0x40, 0x2, 0x0, 0.5f, 0.02f, { 0x2, 0xF, 0x2, 0x1, 0x1 },
      1.0f, 1289.5f, 2.0f, 0.6f, 4.0f, 100.0f, 75.0f, 0x108, 0x48, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[81], { 0x0, 0x0, 0x0, 0x0 },
      { { "pi\xA1\xB1" "ata", NULL },
        { "cracked pi\xA1\xB1" "ata",
          "A ^4pi\xA1\xB1" "ata^^ that's been broken\n^d\"It looks like a lot of stuff\n^dcame out. Did you get any?\"^^" } },
      0x1E0, 0x1, 0x1, -1 },
    /* 0x50: baseball cap */
    { 0x50, 0x10440, 0xA, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 180.0f, 1.0f, 1.0f, 1.0f, 58.0f, 120.0f, 0x108, 0x6E, { 0x98, 0x98 },
      baseballCapUpdateHeldVisibility, baseballCapResetReleasedScale, NULL, itemResolveMapCollision, &gItemModelTemplates[108], { 0x0, 0x0, 0x0, 0x0 },
      { { "cap", NULL },
        { "baseball cap",
          "A replica of a trainer's cap\n^d\"This is the trademark of a\n^dtrainer. It's pretty stylish.\"^^" } },
      0xF8C, 0x2, 0x1, -1 },
    /* 0x51: megaphone */
    { 0x51, 0x400, 0x14, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 160.0f, 1.0f, 0.1f, 10.0f, 60.0f, 113.0f, 0x108, 0x65, { 0xC3, 0xC3 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[15], { 0x0, 0x0, 0x0, 0x0 },
      { { "megaphone", NULL },
        { "megaphone",
          "The very first Pok\xA1\xADHelper\n^d\"I used this to talk to Pok\xA1\xADmon\n^dbefore I had the Pok\xA1\xADHelper.\"^^" } },
      0x0, 0x1, 0x3, 0x23 },
    /* 0x52: fishing hook */
    { 0x52, 0x0, 0x5, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0xF },
      1.0f, -100000.0f, 1.0f, 0.2f, 3.0f, -100000.0f, -100000.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[113], { 0xFF, 0x20, 0x0, 0x80 },
      { { "hook", NULL },
        { "fishing hook",
          "An item for catching Pok\xA1\xADmon\n^d\"This is a safety hook that\n^dwon't hurt Pok\xA1\xADmon.\"^^" } },
      0x0, 0x1, 0x2, 0x1E },
    /* 0x53: lucky hook */
    { 0x53, 0x0, 0x5, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0xF },
      1.0f, -100000.0f, 1.0f, 0.2f, 3.0f, -100000.0f, -100000.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[113], { 0x0, 0x20, 0xFF, 0x80 },
      { { "hook", NULL },
        { "lucky hook",
          "A better fishing hook\n^d\"You can catch a wider variety\n^dof Pok\xA1\xADmon with this hook.\"^^" } },
      0x4B0, 0x1, 0x2, 0x1F },
    /* 0x54: fishing flag */
    { 0x54, 0x2C0, 0xA, 0x0, 1.0f, 0.1f, { 0xF, 0xF, 0x1, 0x1, 0xF },
      0.5f, 1.0f, 0.1f, 0.4f, 6.0f, 300.0f, 300.0f, 0xF7, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[119], { 0x0, 0x0, 0x0, 0x0 },
      { { "flag", NULL },
        { "fishing flag",
          "A prize for catching Pok\xA1\xADmon\n^d\"That's from\n^dCobalt Fishing Hole.\"^^" } },
      0x1388, 0x2, 0x1, 0xF3 },
    /* 0x55: fishing flag */
    { 0x55, 0x2C0, 0xA, 0x0, 1.0f, 0.1f, { 0xF, 0xF, 0x1, 0x1, 0xF },
      0.5f, 1.0f, 0.1f, 0.4f, 6.0f, 300.0f, 300.0f, 0xF5, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[119], { 0x0, 0x0, 0x0, 0x0 },
      { { "flag", NULL },
        { "fishing flag",
          "A prize for catching Pok\xA1\xADmon\n^d\"That's from\n^dOchre Fishing Hole.\"^^" } },
      0x1388, 0x2, 0x1, 0xF1 },
    /* 0x56: fishing flag */
    { 0x56, 0x2C0, 0xA, 0x0, 1.0f, 0.1f, { 0xF, 0xF, 0x1, 0x1, 0xF },
      0.5f, 1.0f, 0.1f, 0.4f, 6.0f, 300.0f, 300.0f, 0xF6, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[119], { 0x0, 0x0, 0x0, 0x0 },
      { { "flag", NULL },
        { "fishing flag",
          "A prize for catching Pok\xA1\xADmon\n^d\"That's from\n^dOlivine Fishing Hole.\"^^" } },
      0x1388, 0x2, 0x1, 0xF2 },
    /* 0x57: pirate flag */
    { 0x57, 0x2C0, 0xA, 0x0, 1.0f, 0.1f, { 0xF, 0xF, 0x1, 0x8, 0xF },
      0.8f, 1.0f, 0.1f, 0.4f, 6.0f, 400.0f, 400.0f, 0xF8, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[119], { 0x0, 0x0, 0x0, 0x0 },
      { { "flag", NULL },
        { "pirate flag",
          "Symbol of a pirate ship\n^d\"That's a prize from the\n^4pi\xA1\xB1" "ata^d party.\"^^" } },
      0x2710, 0x2, 0x1, 0xEF },
    /* 0x58: gold coin */
    { 0x58, 0x20200, 0x8, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      1.0f, 40.0f, 1.0f, 1.0f, 1.0f, 56.0f, 80.0f, 0x100, 0x62, { 0x9B, 0x9B },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[121], { 0x0, 0x0, 0x0, 0x0 },
      { { "coin", NULL },
        { "gold coin",
          "A coin made of gold\n^d\"Look at it glitter. It's\n^dbeautiful. I want one too!\"^^" } },
      0x5DC, 0x0, 0x1, 0xC8 },
    /* 0x59: chocolate coin */
    { 0x59, 0x200, 0x8, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      1.0f, 40.0f, 1.0f, 1.0f, 1.0f, 56.0f, 80.0f, 0x101, 0x62, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[121], { 0x0, 0x0, 0x0, 0x0 },
      { { "coin", NULL },
        { "chocolate coin",
          "A coin made of chocolate\n^d\"Don't mistake it for a\n^4gold coin^d!\"^^" } },
      0x50, 0x2, 0x1, 0xC9 },
    /* 0x5A: silver coin */
    { 0x5A, 0x20200, 0x8, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      1.0f, 40.0f, 1.0f, 1.0f, 1.0f, 56.0f, 80.0f, 0x102, 0x0, { 0x9B, 0x9B },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[121], { 0x0, 0x0, 0x0, 0x0 },
      { { "coin", NULL },
        { "silver coin",
          "A coin made of silver\n^d\"It's worth about @Y@Z100.\"^^\n" } },
      0x64, 0x0, 0x1, 0xCA },
    /* 0x5B: copper coin */
    { 0x5B, 0x20200, 0x8, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      1.25f, 32.0f, 1.0f, 1.0f, 1.0f, 56.0f, 80.0f, 0x103, 0x1, { 0x9B, 0x9B },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[121], { 0x0, 0x0, 0x0, 0x0 },
      { { "coin", NULL },
        { "copper coin",
          "A coin made of copper\n^d\"It's worth about @Y@Z10.\"^^\n" } },
      0xA, 0x0, 0x1, 0xCB },
    /* 0x5C: ruby */
    { 0x5C, 0x0, 0x14, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      0.8f, 70.0f, 1.0f, 1.0f, 1.0f, 55.0f, 90.0f, 0x108, 0x59, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[37], { 0xFF, 0x0, 0x0, 0x80 },
      { { "jewel", NULL },
        { "ruby",
          "A hard red gem\n^d\"You can see through it.\n^dIsn't it beautiful?\"^^" } },
      0x1388, 0x2, 0x1, 0xCD },
    /* 0x5D: sapphire */
    { 0x5D, 0x0, 0x14, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      0.9f, 70.0f, 1.0f, 1.0f, 1.0f, 55.0f, 90.0f, 0x108, 0x58, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[37], { 0x0, 0x0, 0xFF, 0x80 },
      { { "jewel", NULL },
        { "sapphire",
          "A hard, clear blue gem\n^d\"That's an amazing color\n^dof blue.\"^^" } },
      0x2710, 0x2, 0x1, 0xCE },
    /* 0x5E: topaz */
    { 0x5E, 0x0, 0x5, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      1.0f, 70.0f, 1.0f, 1.0f, 1.0f, 55.0f, 90.0f, 0x108, 0x5A, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[37], { 0xFF, 0xC8, 0x0, 0x60 },
      { { "jewel", NULL },
        { "topaz",
          "A delicate yellow gem\n^d\"It's a very valuable gem\n^dthat's really hard to come by.\"^^" } },
      0x4E20, 0x2, 0x1, 0xCF },
    /* 0x5F: red marble */
    { 0x5F, 0x40002, 0xA, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      2.0f, 78.8f, 1.0f, 1.0f, 1.0f, 25.0f, 60.0f, 0x108, 0x50, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[3], { 0xE6, 0x10, 0x0, 0x80 },
      { { "jewel", NULL },
        { "red marble",
          "A red ball of glass\n^d\"I used to play with ^4marble^ds,\n^dtoo, when I was a kid.\"^^" } },
      0x64, 0x1, 0x1, 0xD0 },
    /* 0x60: blue marble */
    { 0x60, 0x40002, 0xA, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      2.0f, 78.8f, 1.0f, 1.0f, 1.0f, 25.0f, 60.0f, 0x108, 0x4F, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[3], { 0x0, 0x10, 0xF0, 0x80 },
      { { "jewel", NULL },
        { "blue marble",
          "A blue ball of glass\n^d\"I used to play with ^4marble^ds,\n^dtoo, when I was a kid.\"^^" } },
      0x64, 0x1, 0x1, 0xD1 },
    /* 0x61: yellow marble */
    { 0x61, 0x40002, 0xA, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      2.0f, 78.8f, 1.0f, 1.0f, 1.0f, 25.0f, 60.0f, 0x108, 0x51, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[3], { 0xC8, 0xC8, 0x0, 0x60 },
      { { "jewel", NULL },
        { "yellow marble",
          "A yellow ball of glass\n^d\"I used to play with ^4marble^ds,\n^dtoo, when I was a kid.\"^^" } },
      0x64, 0x2, 0x1, 0xD2 },
    /* 0x62: ruby ring */
    { 0x62, 0x0, 0x14, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      1.0f, 112.0f, 1.0f, 1.0f, 1.0f, 56.0f, 101.0f, 0x108, 0x70, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[39], { 0x0, 0x0, 0x0, 0x0 },
      { { "ring", NULL },
        { "ruby ring",
          "A ring with a gem on it\n^d\"A symbol of a couple's\n^deverlasting love.\"^^" } },
      0x7530, 0x0, 0x1, 0xD4 },
    /* 0x63: bottle cap ring */
    { 0x63, 0x0, 0x5, 0x0, 1.0f, 0.1f, { 0x19, 0x19, 0x19, 0x19, 0x19 },
      1.0f, 60.0f, 1.0f, 1.0f, 1.0f, 70.0f, 100.0f, 0x108, 0x6A, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[33], { 0x0, 0x0, 0x0, 0x0 },
      { { "ring", NULL },
        { "bottle cap ring",
          "The seal from a plastic bottle\n^d\"Don't litter.\"^^\n" } },
      0x5, 0x0, 0x1, -1 },
    /* 0x64: pirate sword */
    { 0x64, 0x0, 0x5, 0x0, 1.0f, 0.1f, { 0x18, 0x18, 0x18, 0x18, 0x18 },
      1.0f, 170.0f, 1.0f, 1.0f, 1.0f, 57.0f, 143.0f, 0x108, 0x5E, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[46], { 0x0, 0x0, 0x0, 0x0 },
      { { "sword", NULL },
        { "pirate sword",
          "A sword once used by pirates\n^d\"That's from the pirates'\n^dplunder on Cobalt Island.\"^^" } },
      0x16A8, 0x2, 0x1, 0xC7 },
    /* 0x65: toy sword */
    { 0x65, 0x0, 0x5, 0x0, 1.0f, 0.1f, { 0x18, 0x18, 0x18, 0x18, 0x18 },
      1.0f, 170.0f, 1.0f, 1.0f, 1.0f, 57.0f, 140.0f, 0x108, 0x6B, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[36], { 0x0, 0x0, 0x0, 0x0 },
      { { "sword", NULL },
        { "toy sword",
          "A ^4toy sword^^ made of wood\n^d\"En garde! I used to play with\n^d^4toy sword^ds when I was little.\"^^" } },
      0x1F4, 0x2, 0x1, 0xC6 },
    /* 0x66: bird feather */
    { 0x66, 0x0, 0xA, 0x0, 1.0f, 0.1f, { 0x18, 0x18, 0x18, 0x18, 0x18 },
      1.0f, 50.0f, 0.1f, 0.4f, 3.0f, 51.0f, 70.0f, 0x108, 0x6D, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[9], { 0x0, 0x0, 0x0, 0x0 },
      { { "feather", NULL },
        { "bird feather",
          "A feather from a Pok\xA1\xADmon\n^d\"This light feather should fly\n^dpretty well.\"^^" } },
      0x78, 0x2, 0x1, 0xD5 },
    /* 0x67: bolt */
    { 0x67, 0x30, 0x14, 0x0, 1.0f, 0.1f, { 0x18, 0x18, 0x18, 0x18, 0x18 },
      1.0f, 75.1f, 1.0f, 1.0f, 1.0f, 80.0f, 120.0f, 0x108, 0x67, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[25], { 0x0, 0x0, 0x0, 0x0 },
      { { "bolt", NULL },
        { "bolt",
          "An item used to fasten things\n^d\"That's just junk.\"^^\n" } },
      0xA, 0x2, 0x1, 0xD7 },
    /* 0x68: magnet */
    { 0x68, 0x0, 0x5, 0x0, 1.0f, 0.1f, { 0x18, 0x18, 0x18, 0x18, 0x18 },
      1.0f, 206.5f, 1.0f, 1.0f, 2.0f, 103.2f, 103.2f, 0x108, 0x5C, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[12], { 0x0, 0x0, 0x0, 0x0 },
      { { "magnet", NULL },
        { "magnet",
          "A magnetic piece of metal\n^d\"It's a pretty cool shape, but\n^dit's just junk.\"^^" } },
      0x64, 0x2, 0x1, 0xD6 },
    /* 0x69: marble */
    { 0x69, 0x40002, 0xA, 0x0, 1.0f, 0.1f, { 0x18, 0x18, 0x18, 0x18, 0x18 },
      1.0f, 78.8f, 1.0f, 1.0f, 0.8f, 27.0f, 60.0f, 0x108, 0x4E, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[3], { 0x0, 0x0, 0x0, 0x0 },
      { { "marble", NULL },
        { "marble",
          "A ball made of glass\n^d\"I used to play with ^4marble^ds,\n^dtoo, when I was a kid.\"^^" } },
      0x64, 0x1, 0x1, 0xD3 },
    /* 0x6A: stew */
    { 0x6A, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xF9, 0x63, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0xC8, 0xC8, 0x20, 0x80 },
      { { "stew", NULL },
        { "stew",
          "Tasty stew\n" } },
      0x3D4, 0x2, 0x1, -1 },
    /* 0x6B: stew */
    { 0x6B, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xF9, 0x63, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "stew", NULL },
        { "stew",
          "A typical plate of stew\n" } },
      0x30C, 0x2, 0x1, -1 },
    /* 0x6C: weird stew */
    { 0x6C, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x4, 0x4, 0x4, 0x7, 0x4 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFA, 0x54, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "stew", NULL },
        { "weird stew",
          "Weird tasting stew\n" } },
      0xC8, 0x1, 0x1, -1 },
    /* 0x6D: stir-fry */
    { 0x6D, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFB, 0x6F, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0xC8, 0xC8, 0x20, 0x80 },
      { { "stir-fry", NULL },
        { "stir-fry",
          "Tasty fried vegetables\n" } },
      0x2BC, 0x2, 0x1, -1 },
    /* 0x6E: stir-fry */
    { 0x6E, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFB, 0x6F, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "stir-fry", NULL },
        { "stir-fry",
          "A typical plate of stir-fry\n" } },
      0x1F4, 0x2, 0x1, -1 },
    /* 0x6F: weird stir-fry */
    { 0x6F, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x4, 0x4, 0x4, 0x7, 0x4 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFC, 0x55, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "stir-fry", NULL },
        { "weird stir-fry",
          "Weird tasting stir-fry\n" } },
      0x96, 0x1, 0x1, -1 },
    /* 0x70: soup */
    { 0x70, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFD, 0x5B, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0xC8, 0xC8, 0x20, 0x80 },
      { { "soup", NULL },
        { "soup",
          "Tasty soup\n" } },
      0x30C, 0x2, 0x1, -1 },
    /* 0x71: soup */
    { 0x71, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFD, 0x5B, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "soup", NULL },
        { "soup",
          "A typical bowl of soup\n" } },
      0x244, 0x2, 0x1, -1 },
    /* 0x72: weird soup */
    { 0x72, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x4, 0x4, 0x4, 0x7, 0x4 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFE, 0x56, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "soup", NULL },
        { "weird soup",
          "Weird tasting soup\n" } },
      0xB4, 0x1, 0x1, -1 },
    /* 0x73: goulash */
    { 0x73, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFD, 0x5B, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "goulash", NULL },
        { "goulash",
          "Tasty goulash\n" } },
      0x5DC, 0x2, 0x1, -1 },
    /* 0x74: goulash */
    { 0x74, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFD, 0x5B, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "goulash", NULL },
        { "goulash",
          "Average tasting goulash\n" } },
      0x4B0, 0x2, 0x1, -1 },
    /* 0x75: weird goulash */
    { 0x75, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x4, 0x4, 0x4, 0x7, 0x4 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFE, 0x56, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "goulash", NULL },
        { "weird goulash",
          "Weird tasting goulash\n" } },
      0x190, 0x1, 0x1, -1 },
    /* 0x76: cookie */
    { 0x76, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 45.0f, 1.0f, 1.0f, 2.0f, 60.0f, 60.0f, 0x100, 0x5F, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[121], { 0x78, 0x50, 0x0, 0x96 },
      { { "cookie", NULL },
        { "cookie",
          "A tasty cookie\n" } },
      0x3E8, 0x2, 0x1, -1 },
    /* 0x77: cookie */
    { 0x77, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x2, 0x2, 0x2, 0x7, 0x2 },
      1.0f, 45.0f, 1.0f, 1.0f, 2.0f, 60.0f, 60.0f, 0x103, 0x5F, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[121], { 0x78, 0x50, 0x0, 0xC8 },
      { { "cookie", NULL },
        { "cookie",
          "An average tasting cookie\n" } },
      0x258, 0x2, 0x1, -1 },
    /* 0x78: weird cookie */
    { 0x78, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x4, 0x4, 0x4, 0x7, 0x4 },
      1.0f, 45.0f, 1.0f, 1.0f, 2.0f, 60.0f, 60.0f, 0x103, 0x5F, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[121], { 0x19, 0x14, 0x0, 0xC8 },
      { { "cookie", NULL },
        { "weird cookie",
          "A weird tasting cookie\n" } },
      0x12C, 0x1, 0x1, -1 },
    /* 0x79: mystery soup */
    { 0x79, 0x200, 0x2, 0x0, 1.0f, 0.2f, { 0x4, 0x4, 0x4, 0x7, 0x4 },
      1.0f, 60.0f, 1.0f, 1.0f, 2.0f, 70.0f, 125.0f, 0xFF, 0x56, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[120], { 0x0, 0x0, 0x0, 0x0 },
      { { "mystery soup", NULL },
        { "mystery soup",
          "^4BULBASAUR^^'s special recipe\n" } },
      0x64, 0x0, 0x1, -1 },
    /* 0x7A: strange radish */
    { 0x7A, 0x141, 0x3, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 60.0f, 1.0f, 1.0f, 100.0f, 150.0f, 200.0f, 0x108, 0x66, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[24], { 0x0, 0x0, 0x0, 0x0 },
      { { "ODDISH", "May grow when\nsprinkled with water." },
        { "strange radish",
          "An odd radish\n^d\"It looks like ^4ODDISH^d, but it's\n^dactually a plant.\"^^" } },
      0x0, 0x0, 0x1, -1 },
    /* 0x7B: smelly radish */
    { 0x7B, 0x141, 0x3, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 90.0f, 1.0f, 1.0f, 100.0f, 250.0f, 300.0f, 0x108, 0x66, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[112], { 0x0, 0x0, 0x0, 0x0 },
      { { "GLOOM", "May grow when\nsprinkled with water." },
        { "smelly radish",
          "A radish that looks like ^4GLOOM^^\n^d\"It looks like ^4GLOOM^d, but it's\n^da completely different plant.\"^^" } },
      0x0, 0x0, 0x1, -1 },
    /* 0x7C: crate */
    { 0x7C, 0x2000, 0x5, 0x5B, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"An 'A' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x7D: crate */
    { 0x7D, 0x2000, 0x5, 0x5A, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"A 'B' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x7E: crate */
    { 0x7E, 0x2000, 0x5, 0x5F, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"A 'C' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x7F: crate */
    { 0x7F, 0x2000, 0x5, 0x60, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.01f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"A 'D' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x80: crate */
    { 0x80, 0x2000, 0x5, 0x61, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"An 'E' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x81: crate */
    { 0x81, 0x2000, 0x5, 0x24, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"An 'F' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x82: crate */
    { 0x82, 0x2000, 0x5, 0x31, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"A 'G' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x83: crate */
    { 0x83, 0x2000, 0x5, 0x3D, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"An 'H' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x84: crate */
    { 0x84, 0x2000, 0x5, 0x3E, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"An 'I' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x85: crate */
    { 0x85, 0x2000, 0x5, 0x3F, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"A 'J' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x86: crate */
    { 0x86, 0x2000, 0x5, 0x40, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"A 'K' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x87: crate */
    { 0x87, 0x2000, 0x5, 0x41, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"An 'L' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x88: crate */
    { 0x88, 0x2000, 0x5, 0x4, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"An 'M' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x89: crate */
    { 0x89, 0x2000, 0x5, 0x8, 1.0f, 0.1f, { 0xF, 0xA, 0x6, 0xA, 0xF },
      0.5f, 250.0f, 1.0f, 1.0f, 2.0f, 150.0f, 150.0f, 0x108, 0x52, { 0x98, 0xA7 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[103], { 0x0, 0x0, 0x0, 0x0 },
      { { "crate", NULL },
        { "crate",
          "A wooden box\n^d\"A 'Z' is carved on the lid.\n^dI wonder if anything is inside.\"^^" } },
      0xA, 0x1, 0x1, -1 },
    /* 0x8A: cardboard box */
    { 0x8A, 0x8480, 0x5, 0x0, 1.0f, 0.1f, { 0xF, 0x12, 0x4, 0x7, 0x12 },
      1.0f, 300.0f, 5.0f, 0.1f, 4.0f, 150.0f, 150.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[109], { 0x0, 0x0, 0x0, 0x0 },
      { { "box", NULL },
        { "cardboard box",
          "A box made of cardboard\n^d\"^6PIKACHU^d won't notice you if\n^dyou hide inside this box.\"^^" } },
      0x0, 0x1, 0x6, -1 },
    /* 0x8B: notebook */
    { 0x8B, 0x8240, 0x1, 0x0, 1.0f, 0.1f, { 0xF, 0xF, 0xF, 0xF, 0xF },
      1.0f, 190.0f, 1.0f, 1.0f, 1.0f, 103.2f, 103.2f, 0x104, 0x6C, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[27], { 0x0, 0x0, 0x0, 0x0 },
      { { "notebook", NULL },
        { "notebook",
          "A pad for drawing pictures\n^d\"^6PIKACHU^d draws pictures\n^din this.\"^^" } },
      0x0, 0x1, 0x1, -1 },
    /* 0x8C: notebook */
    { 0x8C, 0x8240, 0x1, 0x0, 1.0f, 0.1f, { 0xF, 0xF, 0xF, 0xF, 0xF },
      1.0f, 190.0f, 1.0f, 1.0f, 1.0f, 103.2f, 103.2f, 0x104, 0x6C, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[102], { 0x0, 0x0, 0x0, 0x0 },
      { { "notebook", NULL },
        { "notebook",
          "A pad for drawing pictures\n^d\"^6PIKACHU^d draws pictures\n^din this.\"^^" } },
      0x0, 0x1, 0x1, -1 },
    /* 0x8D: N64 controller */
    { 0x8D, 0x400, 0xA, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 70.0f, 1.0f, 0.5f, 1.0f, 60.0f, 110.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[104], { 0x0, 0x0, 0x0, 0x0 },
      { { "controller", NULL },
        { "N64 controller",
          "The Controller for your N64\n^d\"That's what you use to play\n^dgames on your Nintendo 64.\"^^" } },
      0x9C4, 0x1, 0x1, -1 },
    /* 0x8E: TV */
    { 0x8E, 0x86C0, 0xA, 0x0, 1.0f, 0.3f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 540.0f, 10.0f, 0.1f, 1.0f, 300.0f, 270.0f, 0x106, 0x6E, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[31], { 0x0, 0x0, 0x0, 0x0 },
      { { "TV", NULL },
        { "TV",
          "A device that displays images\n^d\"^6PIKACHU^d turns it on for\n^4Quiz Time^d.\"^^" } },
      0x110A8, 0x1, 0x1, -1 },
    /* 0x8F: telescope */
    { 0x8F, 0x18440, 0x5, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, -100000.0f, 300.0f, 0.1f, 4.0f, -100000.0f, -100000.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "telescope", NULL },
        { "telescope",
          "A lens for viewing the stars\n^d\"That's the ^4telescope^d ^6PIKACHU^d\n^dbought for you.\"^^" } },
      0x4E20, 0x1, 0x1, 0xF0 },
    /* 0x90: board */
    { 0x90, 0x86C0, 0xA, 0x0, 1.0f, 0.3f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, -100000.0f, 10.0f, 0.1f, 1.0f, -100000.0f, -100000.0f, 0x106, 0x6E, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[105], { 0x0, 0x0, 0x0, 0x0 },
      { { "board", NULL },
        { "board",
          "A board for pasting textures\n" } },
      0x12A, 0x1, 0x1, -1 },
    /* 0x91: signpost */
    { 0x91, 0x88642, 0xA, 0x0, 1.0f, 0.3f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 1000.0f, 10.0f, 0.1f, 1.0f, 150.0f, 400.0f, 0x108, 0x82, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[106], { 0x0, 0x0, 0x0, 0x0 },
      { { "signpost", NULL },
        { "signpost",
          "A board with words on it\n^d\"It seems to be designed to\n^dreact to special actions.\"^^" } },
      0x2283, 0x1, 0x1, -1 },
    /* 0x92: The Ending Book */
    { 0x92, 0x86C2, 0xA, 0x0, 1.0f, 0.3f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 1000.0f, 10.0f, 0.1f, 1.0f, 150.0f, 400.0f, 0x108, 0x82, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[107], { 0x0, 0x0, 0x0, 0x0 },
      { { "book", NULL },
        { "The Ending Book",
          "The ending" } },
      0x1E0, 0x1, 0x1, -1 },
    /* 0x93: rosebush */
    { 0x93, 0x20C0, 0x1, 0x95, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.1f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "rosebush", NULL },
        { "rosebush",
          "A bush with many roses\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4flower^d. It's a big bush!\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x94: rosebush */
    { 0x94, 0x20C0, 0x1, 0x96, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.1f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "rosebush", NULL },
        { "rosebush",
          "A bush with many roses\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4flower^d. It's a big bush!\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x95: rosebush */
    { 0x95, 0xC0, 0x1, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 2000.0f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "rosebush", NULL },
        { "rosebush",
          "A bush with many roses\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4flower^d. It's a big bush!\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x96: rosebush */
    { 0x96, 0xC0, 0x1, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 3500.0f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "rosebush", NULL },
        { "rosebush",
          "A bush with many roses\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4flower^d. It's a big bush!\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x97: apple tree */
    { 0x97, 0x20C0, 0x1, 0x9D, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.1f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "apple tree", NULL },
        { "apple tree",
          "A tree with many ^4apple^^s on it\n^d\"A ^4Thunderbolt^d should knock\n^ddown an ^4apple^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x98: peach tree */
    { 0x98, 0x20C0, 0x1, 0x9E, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.1f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "peach tree", NULL },
        { "peach tree",
          "A tree with many ^4peach^^es on it\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4peach^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x99: chestnut tree */
    { 0x99, 0x20C0, 0x1, 0x9F, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.1f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "chestnut tree", NULL },
        { "chestnut tree",
          "A tree with many ^4spiky shell^^s\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4spiky shell^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x9A: smelly fruit tree */
    { 0x9A, 0x20C0, 0x1, 0xA0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.1f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "smelly fruit tree", NULL },
        { "smelly fruit tree",
          "A tree with lots of ^4smelly fruit^^\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4smelly fruit^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x9B: walnut tree */
    { 0x9B, 0x20C0, 0x1, 0xA1, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.1f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "walnut tree", NULL },
        { "walnut tree",
          "A tree with many ^4walnut^^s on it\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4walnut^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x9C: palm tree */
    { 0x9C, 0x20C0, 0x1, 0xA2, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.1f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "palm tree", NULL },
        { "palm tree",
          "A tree with lots of ^4coconut^^s\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4coconut^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x9D: apple tree */
    { 0x9D, 0x0, 0x1, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 3000.0f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "apple tree", NULL },
        { "apple tree",
          "A tree with many ^4apple^^s on it\n^d\"A ^4Thunderbolt^d should knock\n^ddown an ^4apple^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x9E: peach tree */
    { 0x9E, 0x0, 0x1, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 3000.0f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "peach tree", NULL },
        { "peach tree",
          "A tree with many ^4peach^^es on it\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4peach^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0x9F: chestnut tree */
    { 0x9F, 0x0, 0x1, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 3000.0f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "chestnut tree", NULL },
        { "chestnut tree",
          "A tree with many ^4spiky shell^^s\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4spiky shell^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0xA0: smelly fruit tree */
    { 0xA0, 0x0, 0x1, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 3000.0f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "smelly fruit tree", NULL },
        { "smelly fruit tree",
          "A tree with lots of ^4smelly fruit^^\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4smelly fruit^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0xA1: walnut tree */
    { 0xA1, 0x0, 0x1, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 3000.0f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "walnut tree", NULL },
        { "walnut tree",
          "A tree with many ^4walnut^^s on it\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4walnut^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0xA2: palm tree */
    { 0xA2, 0x0, 0x1, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 3000.0f, 10.0f, 1.0f, 1.0f, 80.0f, 80.0f, 0x108, 0x6E, { 0x98, 0x98 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[117], { 0x0, 0x0, 0x0, 0x0 },
      { { "palm tree", NULL },
        { "palm tree",
          "A tree with lots of ^4coconut^^s\n^d\"A ^4Thunderbolt^d should knock\n^ddown a ^4coconut^d.\"^^" } },
      0x0, 0x2, 0x1, -1 },
    /* 0xA3: tool box */
    { 0xA3, 0x400, 0xA, 0x0, 1.0f, 0.3f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      0.3f, -100000.0f, 10.0f, 0.1f, 1.0f, 0.0f, 0.0f, 0x108, 0x6E, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[116], { 0x0, 0x0, 0x0, 0x0 },
      { { "tool box", NULL },
        { "tool box",
          "The pack you keep your items in\n" } },
      0x0, 0x1, 0x6, -1 },
    /* 0xA4: ? */
    { 0xA4, 0x8000, 0xA, 0x0, 1.0f, 0.3f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 200.0f, 10.0f, 0.1f, 1.0f, 100.0f, 100.0f, 0x108, 0x68, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, &gItemModelTemplates[95], { 0x0, 0x0, 0x0, 0x0 },
      { { "Pok\xA1\xADHelper", NULL },
        { "Pok\xA1\xADHelper",
          "A computer with a microphone\n^d\"It's my pride and joy.\"^^\n" } },
      0x0, 0x1, 0x5, -1 },
    /* 0xA5: ? */
    { 0xA5, 0x400, 0x0, 0x0, 1.0f, 0.1f, { 0x1, 0x1, 0x1, 0x1, 0x1 },
      1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0x108, 0x6E, { 0x97, 0x97 },
      NULL, NULL, NULL, itemResolveMapCollision, NULL, { 0x0, 0x0, 0x0, 0x0 },
      { { "?", "?" },
        { "?",
          "" } },
      0x0, 0x1, 0x0, -1 },
};

/// Inclusive item-kind ranges used by category membership queries.
/// \note Original identifier: \c D_8012A578 (0x8012A578).
ItemRange gItemCategoryRanges[19] = {
    { 0x0, 0x0, 0x0 },
    { 0x1, 0x1, 0x8 },
    { 0x2, 0x1, 0x4 },
    { 0x3, 0x9, 0x13 },
    { 0x4, 0x14, 0x24 },
    { 0x5, 0x1E, 0x24 },
    { 0x6, 0x25, 0x31 },
    { 0x7, 0x2D, 0x2F },
    { 0x8, 0x33, 0x3C },
    { 0x9, 0x3D, 0x43 },
    { 0xA, 0x3D, 0x41 },
    { 0xB, 0x49, 0x4B },
    { 0xC, 0x58, 0x65 },
    { 0xD, 0x5C, 0x61 },
    { 0xE, 0x58, 0x69 },
    { 0xF, 0x66, 0x69 },
    { 0x10, 0x6A, 0x79 },
    { 0x11, 0x7C, 0x89 },
    { 0x12, 0x93, 0xA2 },
};
// clang-format on

/// Matching exception: the node result is discarded here; the canonical pointer-return
/// prototype changes IDO allocation. See search_tree_lookup.h and TYPE_RECOVERY.md.
void searchTreeFind(void *, void *); /* extern */

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s16 id;
    /* 0x06 */ u8 pad6[0xEE];
} ItemMsg; /* size 0xF4 */

typedef struct {
    /* 0x00 */ u8 pad0[0x38];
    /* 0x38 */ s32 mode;
    /* 0x3C */ u8 pad3C[0x18];
    /* 0x54 */ ItemMsg *msg;
    /* 0x58 */ s32 unk58;
} ItemReq; /* size 0x5C */

/// Prepare the item registry for searching from a requested item kind.
/// \param kind Starting item kind from zero through 165; 165 requests the beginning of the kind range.
/// \returns Nothing.
///
/// \note Requires an initialized item registry. Searching changes the shared registry traversal state.
/// \note Original decompilation identifier: \c func_80112F60 (0x80112F60).
void itemPrepareKindSearch(s32 kind) {
    ItemReq req;
    ItemMsg msg;

    if (kind == 0xA5) {
        msg.id = 0;
    } else {
        msg.id = kind;
    }
    req.msg = &msg;
    req.mode = -1;
    searchTreeFind(&gItemSearchTree, &req);
    req.mode = 0;
    searchTreeFind(&gItemSearchTree, &req);
}

/// Separate interior view of the nearest-item query world-space origin.
/// \note Original identifier: \c D_80134050 (0x80134050).
extern f32 gItemSearchOrigin[];
/// Separate interior view of the nearest-item query view transform.
/// \note Original identifier: \c D_80134060 (0x80134060).
extern f32 gItemSearchViewTransform[];
f32 matrix34GetTransformedPointYawDegrees(f32 *, f32 *);
f32 mathMapRangeClamped(f32, f32, f32, f32, f32);
f32 mathWrapSignedDegrees(f32);
f32 fabsf(f32);
#pragma intrinsic(fabsf)
/// Consider an item for the current nearest-item query and retain the best eligible candidate.
/// \param node Registered item search node to evaluate.
/// \param context Unused traversal context.
/// \returns One when the kind exceeds the query range and traversal should stop; zero otherwise.
///
/// \note Requires query state prepared by itemFindNearest and a search node referencing a live, initialized
/// item.
/// \note Original decompilation identifier: \c func_80112FC0 (0x80112FC0).
s32 itemConsiderSearchCandidate(u8 *node, s32 context) {
    f32 rawDistance;
    u8 *state;
    f32 distance;
    u8 *item;

    item = (u8 *)((TreeNode *)node)->value;
    state = ((RuntimeObjectPrefix *)item)->state.bytes;
    if (gItemNearestSearch.maxKind < ((ItemStatePrefix *)state)->kind)
        return 1;
    if (gItemNearestSearch.filter != 6 && gItemNearestSearch.filter != ((ItemStatePrefix *)state)->state)
        return 0;
    if (gItemNearestSearch.cone < 360.0f &&
        gItemNearestSearch.cone < fabsf(mathWrapSignedDegrees(matrix34GetTransformedPointYawDegrees(
                                      gItemSearchViewTransform, ((RuntimeObjectPrefix *)item)->worldPosition))))
        return 0;
    rawDistance = objectGetApproxDistanceToPoint(item, (Vec3f *)gItemSearchOrigin);
    distance = rawDistance;
    if (gItemNearestSearch.maxDistance < rawDistance)
        return 0;
    if (gItemNearestSearch.weightEnabled != 0) {
        distance += mathMapRangeClamped(0.0f, 200.0f, 2.0f, 0, ((ItemStatePrefix *)state)->searchPriority);
    }
    if (distance < gItemNearestSearch.bestDistance) {
        gItemNearestSearch.best = item;
        gItemNearestSearch.bestDistance = distance;
    }
    return 0;
}

void matrix34Identity(void *);

/// Find the best nearby item satisfying the requested kind, hold-state, and viewing constraints.
/// \param origin Readable query world position in X, Y, Z order; copied during this call.
/// \param maxDistance Nonnegative maximum approximate world-space distance from the origin.
/// \param pitch Viewing pitch in degrees.
/// \param yaw Viewing yaw in degrees.
/// \param cone Maximum absolute viewing yaw offset in degrees; values of at least 180 allow every direction.
/// \param minKind Inclusive lower item kind; 165 selects the default lower bound of one.
/// \param maxKind Inclusive upper item kind; 165 selects the default upper bound of 164.
/// \param filter Required item hold state from zero through five, or six to accept any hold state.
/// \param weightEnabled Nonzero includes search weighting in candidate selection and resets the selected
/// item's search weight.
/// \returns Borrowed best eligible live item, or null when none qualifies; valid only while the item remains
/// live.
///
/// \note Requires an initialized registry and finite query values. Uses shared query state and must not be
/// reentered during traversal. Portability: the view-transform address is passed as a 32-bit integer; the
/// selected-state assignment preserves the original IDO register allocation.
/// \note Original decompilation identifier: \c func_80113130 (0x80113130).
u8 *itemFindNearest(f32 *origin, f32 maxDistance, f32 pitch, f32 yaw, f32 cone, s32 minKind, s32 maxKind, s32 filter,
                  s32 weightEnabled) {
    u8 *object;
    u8 *state;
    f32 rotation[3];

    if (minKind == 0xA5)
        minKind = 1;
    if (maxKind == 0xA5)
        maxKind = 0xA4;
    if (maxKind < minKind)
        TRACE("ERROR: Invalid itemType. typeMin = %d  typeMax = %d\n", minKind, maxKind);
    gItemNearestSearch.best = NULL;
    gItemNearestSearch.origin[0] = origin[0];
    gItemNearestSearch.origin[1] = origin[1];
    gItemNearestSearch.origin[2] = origin[2];
    gItemNearestSearch.maxDistance = maxDistance;
    gItemNearestSearch.minKind = minKind;
    gItemNearestSearch.maxKind = maxKind;
    gItemNearestSearch.bestDistance = 2147483648.0f;
    gItemNearestSearch.filter = filter;
    gItemNearestSearch.weightEnabled = weightEnabled;
    if (cone >= 180.0f) {
        matrix34Identity(gItemSearchViewTransform);
        gItemNearestSearch.cone = 360.0f;
    } else {
        rotation[0] = pitch;
        rotation[1] = yaw;
        rotation[2] = 0.0f;
        matrix34MakeViewFromEulerDegrees(origin, rotation, (f32 *)gItemSearchViewTransform);
        gItemNearestSearch.cone = cone;
    }
    itemPrepareKindSearch(minKind);
    if (gItemSearchTree.root == 0)
        return NULL;
    object = (u8 *)gItemSearchTree.root->value;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (maxKind < ((ItemStatePrefix *)state)->kind)
        return NULL;
    if (((ItemStatePrefix *)state)->kind >= minKind)
        itemConsiderSearchCandidate(state + 0x54, 0);
    searchTreeVisitAscending(((ItemStatePrefix *)state)->registryNode.links[1], itemConsiderSearchCandidate);
    if (weightEnabled && gItemNearestSearch.best != NULL) {
        object = ((RuntimeObjectPrefix *)gItemNearestSearch.best)->state.bytes;
        ((ItemStatePrefix *)object)->searchPriority = 0.0f;
    }
    return gItemNearestSearch.best;
}


/// Play a spatial sound for an item and retain its playback handle.
/// \param item Live item object address used as the sound source.
/// \param state Initialized state belonging to the item.
/// \param soundId Valid sound-effect identifier to play.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_801132F8 (0x801132F8).
void itemPlayTrackedSound(u8 *item, ItemStatePrefix *state, s32 soundId) {
    TRACE("request-cling:%p type:%d\n", item, soundId);
    ((ItemStatePrefix *)state)->soundHandle = objectPlaySpatialSound(item, soundId, 20.0f, 100.0f);
}

/// Enable gradual shrinking of an item during subsequent updates.
/// \param item Initialized item object to shrink.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80113330 (0x80113330).
void itemEnableShrinking(u8 *item) {
    s32 *itemState;

    itemState = ((RuntimeObjectPrefix *)item)->state.words;
    *itemState |= 0x20000000;
}

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Set an item uniform size relative to its kind and area scale, updating its bounds.
/// \param item Initialized item object to resize.
/// \param scale Finite nonnegative size multiplier relative to the normal item size.
/// \returns Nothing.
///
/// \note Requires a loaded area configuration and valid item-kind scale and bounds.
/// \note Original decompilation identifier: \c func_80113348 (0x80113348).
void itemSetScale(u8 *item, f32 scale) {
    u8 *rec = ((RuntimeObjectPrefix *)item)->state.bytes;

    TRACE("Invalid scale:%f\n", scale);
    ((RuntimeObjectPrefix *)item)->radius = ((ItemStatePrefix *)rec)->baseRadius;
    scale *= ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale *
             gItemKinds[((ItemStatePrefix *)rec)->kind].scale;
    ((RuntimeObjectPrefix *)item)->scale[0] = scale;
    ((RuntimeObjectPrefix *)item)->scale[1] = scale;
    ((RuntimeObjectPrefix *)item)->scale[2] = scale;
    objectScaleHierarchyBounds(item, 1.0f);
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Shrink an item toward thirty percent of its normal size.
/// \param item Initialized item object to resize.
/// \param rate Nonnegative reduction in relative size per second.
/// \returns Nothing.
///
/// \note Requires positive area and item-kind scales and a finite nonnegative frame duration.
/// \note Original decompilation identifier: \c func_801133C0 (0x801133C0).
void itemShrink(u8 *item, f32 rate) {
    u8 *rec = ((RuntimeObjectPrefix *)item)->state.bytes;
    f32 step;
    f32 cur = ((RuntimeObjectPrefix *)item)->scale[0] /
              (((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale *
               gItemKinds[((ItemStatePrefix *)rec)->kind].scale);

    if (!(cur <= 0.3f)) {
        step = gRuntimeFrameSeconds * rate;
        cur -= step;
        if (cur < 0.3f) {
            cur = 0.3f;
        }
        itemSetScale(item, cur);
    }
}

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Release an item collision registration and owned state resources during destruction.
/// \param item Item owning initialized state and rendering resources; those resources become invalid after
/// release.
/// \returns Nothing.
///
/// \note Remove the item from its search registry before releasing state, and wait until rendering no longer
/// uses its resources. Portability: collision registration uses a 32-bit integer object address.
/// \note Original decompilation identifier: \c func_80113464 (0x80113464).
void itemDestroyState(u8 *item) {
    ItemStatePrefix *state;

    state = (ItemStatePrefix *)((RuntimeObjectPrefix *)item)->state.bytes;
    collisionGridUnregisterObject(item, &state->collisionKey);
    heapFreeLocal(&gGameHeap, state->lighting[0]);
    heapFreeLocal(&gGameHeap, state);
}

/// Partial view of the direction animation embedded in item state.
/// This view excludes the allocation tail and must not determine allocation size.
typedef struct ItemDirectionView {
    u8 prefix[0x70];
    DirectionAnimation direction;
} ItemDirectionView;
typedef char ItemDirectionOffsetCheck[((u32) & ((ItemDirectionView *)0)->direction == 0x70) ? 1 : -1];
typedef char
    ItemDirectionPoseIndexOffsetCheck[((u32) & ((ItemDirectionView *)0)->direction.poseIndex == 0xE2) ? 1 : -1];
/// Prepare an item lighting and environment color for rendering.
/// \param state Initialized state with allocated drawing resources, a valid item kind, and lighting index
/// zero or one.
/// \param index Destination buffer, zero or one; must be available for writing and remain live until
/// rendering finishes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801134B0 (0x801134B0).
void itemBuildLightingDisplayList(u8 *state, s32 index) {
    Gfx *g;
    u8 *color;
    g = ((ItemStatePrefix *)state)->lighting[index];
    color = gItemKinds[((ItemStatePrefix *)state)->kind].color;
    gSPSetLights2(g++, (*(Lights2 *)((ItemDirectionView *)state)
                             ->direction.poses[((ItemDirectionView *)state)->direction.poseIndex]
                             .raw));
    gDPPipeSync(g++);
    gDPSetEnvColor(g++, color[0], color[1], color[2], color[3]);
    gSPEndDisplayList(g++);
}

/// Provide an inactive item update hook; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801135CC (0x801135CC).
void itemUnusedUpdateHook(void) {
}


s32 itemCanBePickedUp(u8 *, s32);
void itemUpdateModeAndOrientation(u8 *);
/// Separate symbol view of the primary player record's held item handle.
/// \note Original identifier: \c D_800EF724 (0x800EF724).
extern s32 gPlayerHeldItemHandle;

/// Advance an item simulation and refresh its visibility, interactions, and appearance for the current frame.
/// \param object Live item object with initialized state and hierarchy.
/// \returns Nothing.
///
/// \note Portability: the mode-four debug branch preserves the original IDO register allocation.
///
/// \note Requires initialized area, interaction, and collision systems and valid per-kind resources. The
/// inactive lighting buffer must be available for writing.
/// \note Original decompilation identifier: \c func_801135D4 (0x801135D4).
void itemUpdate(u8 *object) {
    s32 flags;
    s8 mode;
    u8 *child;
    u8 *state;
    s32 changed;
    u8 *record;
    u8 *subject;
    state = *(u8 **)(object + 0x54);

    TRACE("player->holdItem = %d\n", gPlayerHeldItemHandle);
    if (*(s8 *)(state + 6) == 4) {
        subject = *(u8 **)(*(u8 **)(object + 0x2C) + 0x54);
        if (subject == NULL) {
            TRACE("Hold subject has been deleted.\n");
        } else if (*(s8 *)(subject + 6) != 4) {
            TRACE("subject->holdItem = %d\n", *(s32 *)(subject + 0x190));
        }
    } else if (*(s8 *)(state + 6) != 0) {
        TRACE("ERROR: Strange state.\n");
    }
    collisionGridUnregisterObject(object, (s32 *)(state + 0x50));
    mode = *(s8 *)(state + 6);
    if ((mode != 5 && !(*(u16 *)(object + 0x36) & 0x2000)) || mode == 1) {
        if (interactionConsiderObject(object) != 0)
            objectClearHierarchyFlags(object, 2);
        else
            objectSetHierarchyFlags(object, 2);
    }
    *(f32 *)(state + 0x68) -= gRuntimeFrameSeconds;
    if (*(f32 *)(state + 0x68) < 0.0f)
        itemStartDirectionMotion(object, 0.5f, 0.0f, 720.0f);
    directionAnimationUpdate(state + 0x70);
    if (*(s32 *)state & 0x20000000)
        itemShrink(object, 0.12f);
    if (!(*(u16 *)(object + 0x36) & 2) &&
        ((flags = *(s32 *)state, (*(s16 *)(state + 0x3C) != 0 && (flags & 0x200))) || (flags & 0x100000))) {
        animationIntegerTrackEvaluate(&((ItemStatePrefix *)state)->materialTrack, &changed, gRuntimeFrameSeconds);
        child = *(u8 **)(object + 0x2C);
        if (child != NULL && !(*(u16 *)(object + 0x36) & 2) && (*(s32 *)state & 0x100000)) {
            if (changed != 0)
                objectClearHierarchyFlags(child, 2);
            else
                objectSetHierarchyFlags(child, 2);
        }
    }
    record = (u8 *)&gItemKinds[*(s16 *)(state + 4)];
    if (*(f32 *)(state + 0x38) < *(f32 *)(record + 0x10)) {
        *(f32 *)(state + 0x38) = *(f32 *)(record + 0x14) * gRuntimeFrameSeconds + *(f32 *)(state + 0x38);
        if (*(f32 *)(record + 0x10) < *(f32 *)(state + 0x38))
            *(f32 *)(state + 0x38) = *(f32 *)(record + 0x10);
    }
    itemUpdateModeAndOrientation(object);
    if (*(s8 *)(state + 6) == 0 && !(*(u16 *)(object + 0x36) & 0x2000) && !(*(s32 *)state & 0x10000) &&
        (collisionGridFindOverlap(object), itemCanBePickedUp(object, 1) != 0 || itemCanBePickedUp(object, 0) != 0)) {
        *(s32 *)(state + 0x50) = collisionGridRegisterObject(object);
    }
    itemBuildLightingDisplayList(state, ((*(u16 *)(object + 0x36) & 0x8000) ? 1 : 0) ^ 1);
}

void _bzero();                                         /* extern */
void itemBuildLightingDisplayList(u8 *, s32);                  /* extern */

/// Allocate and initialize the state resources for a new item.
/// \returns Newly allocated item state, owned by the item that receives it.
/// \note Original decompilation identifier: \c func_8011386C (0x8011386C).
u8 *itemAllocateState(void) {
    s32 bufferIndex;
    u8 *allocation;
    u8 *lightingStorage;

    allocation = heapAllocLocal(&gGameHeap, 0xF0, 0x2C7);
    _bzero(allocation, 0xF0);
    ((ItemStatePrefix *)allocation)->kind = 0;
    ((ItemStatePrefix *)allocation)->renderMode = 0;
    directionAnimationReset(allocation + 0x70);
    ((ItemStatePrefix *)allocation)->idleMotionDelay = 5.5f;
    lightingStorage = heapAllocLocal(&gGameHeap, 0x70, 0x2D4);
    ((ItemStatePrefix *)allocation)->lighting[0] = (Gfx *)lightingStorage;
    ((ItemStatePrefix *)allocation)->lighting[1] = (Gfx *)(lightingStorage + 0x38);
    bufferIndex = 0;
    do {
        itemBuildLightingDisplayList(allocation, bufferIndex);
        bufferIndex += 1;
    } while (bufferIndex != 2);
    return allocation;
}

/// Give a new item its initialized, owned state resources.
/// \param item New item object without existing owned state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011391C (0x8011391C).
void itemInitializeState(u8 *item) {
    ((RuntimeObjectPrefix *)item)->state.bytes = itemAllocateState();
}

f32 terrainGetWorldHeight(f32, f32, f32, u8 **);
/// Shared integer animation programs selecting texture frames for actors, effects and interface elements.
/// \note Original identifier: \c D_8006B090 (0x8006B090).
extern s32 gTextureFrameAnimationPrograms[];
/// Whether newly created items receive hierarchy flag two; broader policy remains unresolved.
/// \note Original identifier: \c D_80088F20 (0x80088F20).
extern s32 gItemCreationHierarchyFlag2Enabled;
/* Object type halfword at +0x34 (the maze block placer sets 0x88). */
#define SET_OBJECT_TYPE(obj, type)                                                                                     \
    do {                                                                                                               \
        *(u16 *)((obj) + 0x34) = (type);                                                                               \
    } while (0)
/// Create and register an item at the requested world position and orientation.
/// \param x World X coordinate.
/// \param y World Y coordinate; a negative value requests ground placement using its magnitude as the terrain
/// query height.
/// \param z World Z coordinate.
/// \param pitch Initial rotation about the X axis in degrees.
/// \param yaw Initial rotation about the Y axis in degrees.
/// \param kind Item kind identifier from zero through 165 with a valid model and animation configuration.
/// \returns Registered item managed by the object system.
///
/// \note Requires initialized registry, object resources, and area configuration; ground placement also
/// requires initialized terrain. The caller must enforce any item-count limit. Portability: registry keys use
/// 32-bit integer object addresses.
/// \note Original decompilation identifier: \c func_80113944 (0x80113944).
u8 *itemCreate(f32 x, f32 y, f32 z, f32 pitch, f32 yaw, s32 kind) {
    u8 *object;
    ItemStatePrefix *state;
    TRACE("makeItem(%.2f, %.2f, %.2f, %d) ", x, y, z, kind);
    TRACE("(%s)\n", gItemKinds[kind].text[0].name);
    object = objectCreateHierarchyFromTemplate((ObjectTemplate *)gItemKinds[kind].model);
    SET_OBJECT_TYPE(object, 0x86);
    itemInitializeState(object);
    state = (ItemStatePrefix *)((RuntimeObjectPrefix *)object)->state.bytes;
    state->kind = kind;
    state->flags = gItemKinds[kind].stateFlags | 0x1000000;
    state->bounceCount = gItemKinds[kind].unk8;
    state->searchPriority = gItemKinds[kind].growthMax;
    state->state = 0;
    state->owner = 0;
    state->mode = 0;
    animationIntegerTrackInitialize(&state->materialTrack, (ValueKey *)(((s32 *)&gTextureFrameAnimationPrograms)[gItemKinds[kind].animation]));
    if (gItemKinds[kind].size != -100000.0f)
        *(f32 *)(object + 0x24) = gItemKinds[kind].size;
    state->baseRadius = *(f32 *)(object + 0x24);
    if (gItemKinds[kind].radiusX == -100000.0f)
        gItemKinds[kind].radiusX = *(f32 *)(object + 0x24) * 0.5f;
    if (gItemKinds[kind].radiusZ == -100000.0f)
        gItemKinds[kind].radiusZ = *(f32 *)(object + 0x24) * 0.5f;
    objectRegisterActiveHierarchy(object);
    searchTreeInsert(&gItemSearchTree, &state->registryNode, (s32)object);
    TRACE("itemType %d has a sibling\n", kind);
    itemSetScale(object, 1.0f);
    if (y < 0.0f) {
        y = terrainGetWorldHeight(x, -y, z, 0);
        y += *(f32 *)(object + 0x24);
    }
    *(f32 *)(object + 0xC) = pitch;
    *(f32 *)(object + 0x10) = yaw;
    *(f32 *)(object + 0x14) = 0.0f;
    *(f32 *)(object + 0x18) = x;
    *(f32 *)(object + 0x1C) = y;
    *(f32 *)(object + 0x20) = z;
    *(f32 *)(object + 0x44) = x;
    *(f32 *)(object + 0x48) = y;
    *(f32 *)(object + 0x4C) = z;
    gLiveItemCount = (s32)((u32)gLiveItemCount + 1U);
    if (gItemCreationHierarchyFlag2Enabled)
        objectSetHierarchyFlags(object, 2);
    return object;
}

s32 itemTryRelease(u8 *, s32);   /* extern */
/// Number of live registered items in the current area.
/// \note Original identifier: \c D_80134040 (0x80134040).
extern s32 gLiveItemCount;

/// Remove an item from world participation and request its destruction.
/// \param item Live, registered item to destroy; must not already be pending destruction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80113B78 (0x80113B78).
void itemDestroy(u8 *item) {
    ItemStatePrefix *state;

    TRACE("destroy item. item rest = %d\n", gLiveItemCount - 1);
    state = (ItemStatePrefix *)((RuntimeObjectPrefix *)item)->state.bytes;
    if ((state->owner != 0) || (state->state == 1)) {
        itemTryRelease(item, 1);
    }
    collisionGridUnregisterObject(item, &state->collisionKey);
    searchTreeRemove(&gItemSearchTree, (s32)item);
    objectRequestHierarchyDestruction(item);
    gLiveItemCount -= 1;
}

/// Compare an item with a registered item by kind, then by unsigned object handle.
/// \param node Search node containing the registered item to compare against.
/// \param itemAddress Target-width address of the live item being compared.
/// \returns Minus one if the item precedes the node's item, one if it follows, or zero if they have equal
/// kind and handle.
///
/// \note Both items must have initialized state. Portability: the otherwise unused handle read preserves the
/// original IDO register allocation.
/// \note Original decompilation identifier: \c func_80113C04 (0x80113C04).
s32 itemCompareSearchOrder(TreeNode *node, s32 itemAddress) {
    u8 *item = (u8 *)itemAddress;
    u8 *other;
    u8 *a;
    u8 *b;
    u32 ia;
    u32 ib;

    other = (u8 *)node->value;
    b = ((RuntimeObjectPrefix *)other)->state.bytes;
    a = ((RuntimeObjectPrefix *)item)->state.bytes;
    if (((ItemStatePrefix *)a)->kind < ((ItemStatePrefix *)b)->kind) {
        return -1;
    }
    if (((ItemStatePrefix *)b)->kind < ((ItemStatePrefix *)a)->kind) {
        return 1;
    }
    ib = (u32)((RuntimeObjectPrefix *)other)->handle;
    ia = (u32)((RuntimeObjectPrefix *)item)->handle;
    if (ia < (u32)((RuntimeObjectPrefix *)other)->handle) {
        return -1;
    }
    if ((u32)((RuntimeObjectPrefix *)other)->handle < ia) {
        return 1;
    }
    return 0;
}


/// Initialize an empty item registry for the current area.
/// \returns Nothing.
///
/// \note Release all previous registry entries before reinitializing; existing item search nodes must no
/// longer be linked.
/// \note Original decompilation identifier: \c func_80113C78 (0x80113C78).
void itemInitializeRegistry(void) {
    gLiveItemCount = 0;
    searchTreeInitialize(&gItemSearchTree, itemCompareSearchOrder);
}

/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;

/// Apply an item prepared lighting and environment color to the current drawing context.
/// \param item Initialized item whose lighting is applied.
/// \returns Nothing.
///
/// \note Requires prepared lighting for the active render buffer and an active display-list cursor with
/// capacity. Referenced drawing resources must remain live until rendering finishes.
/// \note Original decompilation identifier: \c func_80113CAC (0x80113CAC).
void itemDrawLighting(u8 *item) {
    gSPDisplayList(gRenderDisplayListCursor++,
                   ((RuntimeObjectPrefix *)item)
                       ->state.displayLists[0x18 + ((((RuntimeObjectPrefix *)item)->flags & 0x8000) ? 1 : 0)]);
}

typedef struct {
    u8 r, g, b, a;
} ItemColor13CFC;
Gfx *textureLoadForDraw(Gfx **, s32, s32);
/// Apply an item material, rendering style, and tint to the current drawing context when the item uses a
/// material.
/// \param item Initialized item being prepared for drawing.
/// \returns Borrowed address of the object's drawing continuation list.
///
/// \note Requires an active display-list cursor with sufficient capacity and, when a material is enabled, the
/// loaded item texture bank and valid material frame. Drawing resources remain owned by the item.
/// \note Original decompilation identifier: \c func_80113CFC (0x80113CFC).
s32 itemPrepareMaterial(u8 *item) {
    ItemColor13CFC *color;
    u8 *state;
    state = ((RuntimeObjectPrefix *)item)->state.bytes;
    if (((ItemStatePrefix *)state)->flags & ITEM_STATE_MATERIAL_ENABLED) {
        textureLoadForDraw(&gRenderDisplayListCursor, 8, ((ItemStatePrefix *)state)->materialTrack.value);
        switch (((ItemStatePrefix *)state)->renderMode) {
        case 0:
            gDPSetRenderMode(gRenderDisplayListCursor++, 0xCB026238, 0);
            gDPSetCombine(gRenderDisplayListCursor++, 0x357EAC, 0x10FCFC38);
            break;
        case 1:
            gDPSetRenderMode(gRenderDisplayListCursor++, 0xCB026238, 0);
            gDPSetCombine(gRenderDisplayListCursor++, 0x347EAC, 0x10FCFC38);
            break;
        case 2:
            gDPSetRenderMode(gRenderDisplayListCursor++, 0xCB026238, 0);
            gDPSetCombine(gRenderDisplayListCursor++, 0x1417E7, 0x30FDFE3E);
            break;
        case 3:
            gDPSetRenderMode(gRenderDisplayListCursor++, 0xCB026238, 0);
            gDPSetCombine(gRenderDisplayListCursor++, 0x141667, 0x30FDFE3E);
            break;
        case 4:
            gDPSetRenderMode(gRenderDisplayListCursor++, 0xCB026238, 0);
            gDPSetCombine(gRenderDisplayListCursor++, 0x357FFF, 0x1FFFFC38);
            break;
        case 5:
            gDPSetRenderMode(gRenderDisplayListCursor++, 0xCB026238, 0);
            gDPSetCombine(gRenderDisplayListCursor++, 0x1416C7, 0x30FDFE3E);
            break;
        case 6:
            gDPSetRenderMode(gRenderDisplayListCursor++, 0x001049D8, 0);
            gDPSetCombine(gRenderDisplayListCursor++, 0x3566AC, 0x10FC9238);
            break;
        default:
            TRACE("ERROR: Unknown drawMode(%d)\n", ((ItemStatePrefix *)state)->renderMode);
            break;
        }
        {
            color = (ItemColor13CFC *)(state + 0x4C);
            gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, color->r, color->g, color->b, color->a);
        }
    }
    return (s32)((RuntimeObjectPrefix *)item)->displayList;
}


/// Exclude a live item from item searches and world collision queries.
/// \param objectId Handle of a live item to exclude.
/// \returns Nothing.
///
/// \note Requires an initialized registry and collision system; the handle must resolve to an initialized
/// item.
/// \note Original decompilation identifier: \c func_80113FC8 (0x80113FC8).
void itemRemoveFromSearchAndCollision(s32 objectId) {
    u8 *sp1C;
    u8 *liveObject;

    liveObject = objectFindLiveByHandle(objectId);
    sp1C = liveObject;
    searchTreeRemove(&gItemSearchTree, (s32)liveObject);
    collisionGridUnregisterObject(sp1C,
                                  &((ItemStatePrefix *)((RuntimeObjectPrefix *)sp1C)->state.pointer)->collisionKey);
}


/// Make a live item available to item searches.
/// \param objectId Handle of a live item to register.
/// \returns Nothing.
///
/// \note Requires an initialized registry and an item whose embedded search node is not already linked.
/// \note Original decompilation identifier: \c func_8011400C (0x8011400C).
void itemRegisterForSearch(s32 objectId) {
    u8 *liveObject;
    u8 *itemState;

    liveObject = objectFindLiveByHandle(objectId);
    itemState = ((RuntimeObjectPrefix *)liveObject)->state.bytes;
    searchTreeInsert(&gItemSearchTree, (TreeNode *)(itemState + 0x54), (s32)liveObject);
}

void itemSetMode();
/// Release an item from its current holder when its lifecycle permits release.
/// \param object Live initialized item with valid hold state and item-kind callbacks.
/// \param force Nonzero bypasses the holder's release restriction; suspended items and items attached to
/// another item cannot be released.
/// \returns One if the item is free after release, or zero if release is refused.
///
/// \note Portability: the release callback receives the former holder address as a 32-bit integer, or zero
/// when no live holder is found.
/// \note Original decompilation identifier: \c func_80114044 (0x80114044).
s32 itemTryRelease(u8 *object, s32 force) {
    u8 *state;
    u8 *other;
    u8 *found;
    void (*callback)(u8 *, s32);
    state = *(u8 **)(object + 0x54);
    other = NULL;
    switch (*(u8 *)(state + 6)) {
    case 0:
        break;
    case 5:
        return 0;
    case 1:
        if (force == 0 && !(*(s32 *)state & 0x40000000))
            return 0;
        gPlayerHeldItemHandle = 0;
        break;
    case 2:
    case 3:
        found = objectFindLiveByHandle(*(s32 *)(state + 8));
        other = found;
        if (found != NULL) {
            if (force == 0 && !(*(s32 *)state & 0x40000000))
                return 0;
            *(s32 *)(*(u8 **)(found + 0x54) + 0x190) = 0;
        }
        break;
    case 4:
        return 0;
    default:
        TRACE("Error: Unknown hold_type(%d) in item(%p)\n", state[6], object);
        break;
    }
    callback = gItemKinds[*(s16 *)(state + 4)].release;
    if (callback != NULL)
        callback(object, (s32)other);
    state[6] = 0;
    *(s32 *)(state + 8) = 0;
    *(f32 *)(state + 0x2C) = 0.0f;
    itemSetMode(object, 0);
    *(s32 *)state &= 0xBFFFFFFF;
    return 1;
}

/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
s32 terrainConstrainMovement(s32, f32, f32, f32, f32, f32, f32, f32 *, f32 *, f32 *, s32);
/// Launch a live item with collision-aware starting placement and the requested motion.
/// \param id Handle of a live item that permits release; requires the active primary actor.
/// \param origin Readable three-component world position from which starting placement is checked.
/// \param target Requested starting world position in X, Y, Z order; nearby obstacles may limit placement.
/// \param velocity Readable world-space launch velocity in X, Y, Z order, scaled by the item kind's launch
/// speed.
/// \param effect Nonzero requests the throw sound effect.
/// \returns Nothing.
///
/// \note Requires initialized terrain and valid item-kind resources. Launch vectors must remain readable
/// throughout the call and must not overlap item fields modified before they are consumed.
/// \note Original decompilation identifier: \c func_8011418C (0x8011418C).
void itemThrow(s32 id, f32 *origin, f32 *target, f32 *velocity, s32 effect) {
    void (*callback)(u8 *);
    f32 radius;
    f32 delta[3];
    u8 *state;
    u8 *object;
    TRACE("itemThrow itemID:%d\norgPos  (%.2f, %.2f, %.2f)\nthrowPos(%.2f, %.2f, %.2f)\nthrowVel(%.2f, %.2f, %.2f)\n",
          id, origin[0], origin[1], origin[2], target[0], target[1], target[2], velocity[0], velocity[1], velocity[2]);
    object = objectFindLiveByHandle(id);
    state = *(u8 **)(object + 0x54);
    if (*(s32 *)((u8 *)gPikachuActor + 0x38) == *(s32 *)(state + 8)) {
        *(s32 *)state |= 0x08000000;
    } else if (*(s8 *)(state + 6) == 1) {
        *(s32 *)state |= 0x04000000;
    }
    itemTryRelease(object, 1);
    *(s32 *)state |= 0x01000000;
    *(f32 *)(object + 0xC) = 0.0f;
    *(f32 *)(object + 0x14) = 0.0f;
    delta[0] = target[0] - origin[0];
    delta[1] = target[1] - origin[1];
    delta[2] = target[2] - origin[2];
    radius = *(f32 *)(object + 0x24);
    terrainConstrainMovement(7, origin[0], origin[1] - radius, origin[2], radius * 2, radius * 2, radius, &delta[0], &delta[1],
                  &delta[2], 0);
    *(f32 *)(object + 0x18) = origin[0] + delta[0];
    *(f32 *)(object + 0x1C) = origin[1] + delta[1];
    *(f32 *)(object + 0x20) = origin[2] + delta[2];
    *(f32 *)(state + 0x14) = velocity[0] * gItemKinds[*(s16 *)(state + 4)].speed;
    *(f32 *)(state + 0x10) = -velocity[1] * gItemKinds[*(s16 *)(state + 4)].speed;
    *(f32 *)(state + 0x18) = velocity[2] * gItemKinds[*(s16 *)(state + 4)].speed;
    *(f32 *)(state + 0x1C) = 0.0f;
    *(f32 *)(state + 0x20) = 0.0f;
    if (*(s32 *)state & 0x20)
        *(f32 *)(state + 0x28) = 8.0f;
    else
        *(f32 *)(state + 0x28) = 0.0f;
    callback = gItemKinds[*(s16 *)(state + 4)].launch;
    if (callback != NULL)
        callback(object);
    if (effect != 0)
        itemPlayTrackedSound(object, (ItemStatePrefix *)state, 0x99);
}

/// Apply a velocity change to a free item that permits motion.
/// \param objectId Handle of a live item.
/// \param velocity Readable three-component world-space velocity change in X, Y, Z order; must not overlap
/// the modified item state.
/// \returns Nothing.
///
/// \note Portability: the unprototyped mode-change call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_801143BC (0x801143BC).
void itemAddVelocity(s32 objectId, f32 *velocity) {
    u8 *liveObject;
    u8 *itemState;

    liveObject = objectFindLiveByHandle(objectId);
    itemState = ((RuntimeObjectPrefix *)liveObject)->state.bytes;
    if ((((ItemStatePrefix *)itemState)->state == 0) && (((ItemStatePrefix *)itemState)->mode != 6)) {
        ((ItemStatePrefix *)itemState)->velocityX += velocity[0];
        ((ItemStatePrefix *)itemState)->downwardVelocity -= velocity[1];
        ((ItemStatePrefix *)itemState)->velocityZ += velocity[2];
        itemSetMode(liveObject, 0, velocity);
    }
}


/// Change an item's behavior mode and apply the mode's participation rules.
/// \param item Initialized item object to configure.
/// \param mode Supported item behavior mode from zero through six.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011443C (0x8011443C).
void itemSetMode(u8 *item, s32 mode) {
    s32 *rec = ((RuntimeObjectPrefix *)item)->state.words;

    if (mode == 6) {
        rec[0] &= 0x1FFFFEFF;
        rec[0] |= 0x8000C0;
    }
    if ((mode == 2) && !(rec[0] & 2)) {
        objectSetHierarchyFlags(item, 0x400);
    } else if (((RuntimeObjectPrefix *)item)->flags & 0x700) {
        objectClearHierarchyFlags(item, 0x700);
    }
    ((s8 *)rec)[7] = mode;
}

/// Check whether an item's current state permits pickup by the requested holder.
/// \param item Initialized item object to query.
/// \param byPlayer Nonzero checks player pickup rules; zero checks actor pickup rules.
/// \returns One if pickup is permitted, or zero otherwise.
/// \note Original decompilation identifier: \c func_801144EC (0x801144EC).
s32 itemCanBePickedUp(u8 *item, s32 byPlayer) {
    s32 itemFlags;
    s8 itemModeState;
    u8 *itemState;

    itemState = ((RuntimeObjectPrefix *)item)->state.bytes;
    itemModeState = ((ItemStatePrefix *)itemState)->state;
    if ((itemModeState == 4) || (itemModeState == 5)) {
        return 0;
    }
    if (byPlayer != 0) {
        if (((itemModeState == 2) || (itemModeState == 3)) && !(((ItemStatePrefix *)itemState)->flags & 0x40000000)) {
            return 0;
        }
        if (((ItemStatePrefix *)itemState)->flags & 0x04000040) {
            return 0;
        }
        return 1;
    }
    itemFlags = ((ItemStatePrefix *)itemState)->flags;
    if ((itemFlags & 0x80) || ((itemModeState == 1) && !(itemFlags & 0x40000000))) {
        return 0;
    }
    return 1;
}

/// Check whether an item permits requesting Pikachu cooperation.
/// \param item Initialized item object to query.
/// \returns One if cooperation is permitted, or zero otherwise.
/// \note Original decompilation identifier: \c func_801145A0 (0x801145A0).
s32 itemAllowsCooperation(u8 *item) {
    s32 itemFlags;
    s32 *itemState = ((RuntimeObjectPrefix *)item)->state.words;

    itemFlags = *itemState;
    if ((itemFlags & 0x100) && (itemFlags & 0x80000000)) {
        return 1;
    }
    return 0;
}


/// Start an item material animation from its beginning.
/// \param item Initialized item object receiving the animation.
/// \param animation Valid integer animation-track identifier.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801145D0 (0x801145D0).
void itemStartMaterialAnimation(u8 *item, s32 animation) {
    ItemStatePrefix *itemState;

    TRACE("itemChangeAnime(%p, %d)\n", item, animation);
    itemState = (ItemStatePrefix *)((RuntimeObjectPrefix *)item)->state.bytes;
    animationIntegerTrackInitialize(&itemState->materialTrack, (ValueKey *)(*((s32 *)((u8 *)&gTextureFrameAnimationPrograms + (animation * 4)))));
    itemState->materialAnimationActive = 1;
}

/// Select a fixed item material frame and stop material animation.
/// \param item Initialized item object receiving the material selection.
/// \param frame Valid material texture frame identifier representable as a signed halfword.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114614 (0x80114614).
void itemSetMaterialFrame(u8 *item, s32 frame) {
    ItemStatePrefix *itemState;

    TRACE("itemChangeMat(%p, %d)\n", item, frame);
    itemState = (ItemStatePrefix *)((RuntimeObjectPrefix *)item)->state.bytes;
    itemState->materialTrack.value = frame;
    itemState->materialAnimationActive = 0;
}

/// Select the rendering style of an item; a missing item is ignored.
/// \param objectId Item object handle.
/// \param mode Supported item draw mode from zero through six.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114624 (0x80114624).
void itemSetDrawMode(s32 objectId, s32 mode) {
    u8 *liveObject;
    ItemStatePrefix *itemState;

    liveObject = objectFindLiveByHandle(objectId);
    if (liveObject != NULL) {
        itemState = (ItemStatePrefix *)((RuntimeObjectPrefix *)liveObject)->state.bytes;
        itemState->renderMode = mode;
    }
}

/// Set the color tint of a live item; has no effect when the item is unavailable.
/// \param id Item object handle.
/// \param r Red intensity from zero through 255.
/// \param g Green intensity from zero through 255.
/// \param b Blue intensity from zero through 255.
/// \param a Opacity from zero through 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114654 (0x80114654).
void itemSetTint(s32 id, u8 r, u8 g, u8 b, u8 a) {
    u8 *obj = objectFindLiveByHandle(id);
    ItemStatePrefix *itemState;

    if (obj != NULL) {
        itemState = (ItemStatePrefix *)((RuntimeObjectPrefix *)obj)->state.bytes;
        itemState->tint[0] = r;
        itemState->tint[1] = g;
        itemState->tint[2] = b;
        itemState->tint[3] = a;
    }
}

/// Look up an item kind's configured response to an interaction category.
/// \param responseType Interaction response category from one through five.
/// \param kind Item kind identifier from zero through 165.
/// \returns The configured response code, or zero for an unsupported category.
/// \note Original decompilation identifier: \c func_801146A4 (0x801146A4).
u8 itemGetKindResponse(s32 responseType, s32 kind) {
    switch (responseType) {
    case 1:
        return gItemKinds[kind].response[0];
    case 2:
        return gItemKinds[kind].response[1];
    case 3:
        return gItemKinds[kind].response[2];
    case 4:
        return gItemKinds[kind].response[3];
    case 5:
        return gItemKinds[kind].response[4];
    default:
        TRACE("ERROR: unknown itemResponseType(%d)\n", responseType);
        return 0U;
    }
}

/// Look up the interaction response of an item that permits responses.
/// \param responseType Interaction response category from one through five.
/// \param objectId Item object handle.
/// \returns The configured response code, or zero if the item is missing, cannot respond, or the category is
/// unsupported.
/// \note Original decompilation identifier: \c func_80114788 (0x80114788).
s32 itemGetResponse(s32 responseType, s32 objectId) {
    u8 *liveObject;
    ItemStatePrefix *itemState;

    liveObject = objectFindLiveByHandle(objectId);
    if (liveObject == NULL) {
        return 0;
    }
    itemState = (ItemStatePrefix *)((RuntimeObjectPrefix *)liveObject)->state.bytes;
    if (itemState->flags & 0x800000) {
        return 0;
    }
    return itemGetKindResponse(responseType, itemState->kind);
}

u8 *wispEffectCreate(s32, f32, f32, f32);
void effectEmitterCreateForStyle(u32, f32 *, f32, s32, s32, f32);
void actorTryHoldItem(u8 *, s32, s32);

/// Apply a transformation cause to an item, retaining, replacing, or removing it as appropriate.
/// \param id Initialized item handle, or a missing handle to return zero.
/// \param mode Supported transformation-cause selector from zero through five.
/// \returns The surviving item handle, which may identify a replacement; zero if the item is missing or
/// removed.
///
/// \note Requires initialized item, effect, and area resources. A replacement invalidates the original item
/// handle; use the returned handle. Held-item replacement requires the recorded actor holder to remain live.
/// \note Original decompilation identifier: \c func_801147E4 (0x801147E4).
s32 itemTransform(s32 id, s32 mode) {
    u8 *object;
    ItemStatePrefix *state;
    u8 *replacement;
    ItemStatePrefix *replacementState;
    s32 replacementKind;
    s32 inheritedFlags;
    f32 effectOpacity = 1.0f;
    u8 *effect;

    object = objectFindLiveByHandle(id);
    if (object == NULL) {
        return 0;
    }
    state = (ItemStatePrefix *)((RuntimeObjectPrefix *)object)->state.pointer;
    TRACE("itemChange(%p(%s), %d)\n", object, gItemKinds[state->kind].text[0].name, mode);
    if ((state->mode == 6) || (state->state == 4)) {
        return id;
    }
    replacementKind = gItemKinds[state->kind].replacement;
    switch (mode) {
    case 0:
        if (!(state->flags & 0x1000) || (replacementKind == 0)) {
            itemDestroy(object);
            return 0;
        }
        break;
    case 1:
        if (state->kind == 0x47) {
            effectEmitterCreateForStyle(0U, (f32 *)(object + 0x44), 1.5f, 0, state->owner, effectOpacity);
            itemDestroy(object);
            return 0;
        }
        if (!(state->flags & 0x2000) || (replacementKind == 0)) {
            replacementKind = 0x47;
        }
        break;
    case 5:
        replacementKind = 0x47;
        break;
    case 2:
        if (!(state->flags & 0x4000) || (replacementKind == 0)) {
            return id;
        }
        TRACE("changeThunder:%d changeTo:%s\n", state->kind, gItemKinds[replacementKind].text[0].name);
        itemPlayTrackedSound(object, state, (s32)gItemKinds[state->kind].effect[1]);
        break;
    case 3:
    case 4:
        effect = wispEffectCreate(mode == 3 ? 4 : 5, ((RuntimeObjectPrefix *)object)->worldPosition[0], ((RuntimeObjectPrefix *)object)->worldPosition[1],
                               ((RuntimeObjectPrefix *)object)->worldPosition[2]);
        effectEmitterCreateForStyle(0U, (f32 *)(object + 0x44), 1.5f, ((RuntimeObjectPrefix *)effect)->handle,
                                    state->owner, effectOpacity);
        itemDestroy(object);
        return 0;
    default:
        TRACE("ERROR: Unknown cause(%d)\n", mode);
        break;
    }
    replacement =
        itemCreate(((RuntimeObjectPrefix *)object)->worldPosition[0], ((RuntimeObjectPrefix *)object)->worldPosition[1], ((RuntimeObjectPrefix *)object)->worldPosition[2],
                      ((RuntimeObjectPrefix *)object)->rotation[0], ((RuntimeObjectPrefix *)object)->rotation[1], replacementKind);
    replacementState = (ItemStatePrefix *)((RuntimeObjectPrefix *)replacement)->state.pointer;
    inheritedFlags = state->flags & 0xDE000000;
    switch (*(u8 *)&state->state) {
    case 0:
    case 1:
    case 4:
        break;
    case 5:
        TRACE("ERROR: Cannot change checked-in item.\n");
        break;
    case 2:
    case 3:
        actorTryHoldItem(objectFindLiveByHandle(state->owner), ((RuntimeObjectPrefix *)replacement)->handle, 0);
        break;
    default:
        TRACE("ERROR: Strange.\n");
        break;
    }
    replacementState->flags = (s32)(replacementState->flags | inheritedFlags);
    itemDestroy(object);
    if ((mode == 1) || (mode == 5)) {
        switch (replacementState->kind) {
        case 0x47:
            effectEmitterCreateForStyle(0U, (f32 *)(replacement + 0x44), 2.0f,
                                        ((RuntimeObjectPrefix *)replacement)->handle, replacementState->owner,
                                        effectOpacity);
            break;
        case 0xC:
            effectEmitterCreateForStyle(4U, (f32 *)(replacement + 0x44), 5.0f,
                                        ((RuntimeObjectPrefix *)replacement)->handle, replacementState->owner,
                                        effectOpacity);
            effectEmitterCreateForStyle(7U, (f32 *)(replacement + 0x44), 1.0f,
                                        ((RuntimeObjectPrefix *)replacement)->handle, replacementState->owner, 0.0f);
            break;
        default:
            effectEmitterCreateForStyle(4U, (f32 *)(replacement + 0x44), 5.0f,
                                        ((RuntimeObjectPrefix *)replacement)->handle, replacementState->owner,
                                        effectOpacity);
        }
    }
    return ((RuntimeObjectPrefix *)replacement)->handle;
}

/// Obtain an item's effective coefficient for gravity and friction.
/// \param item Initialized item whose current motion properties are queried.
/// \returns The effective motion coefficient for the item's current state.
/// \note Original decompilation identifier: \c func_80114B84 (0x80114B84).
f32 itemGetMotionCoefficient(u8 *item) {
    u8 *rec = ((RuntimeObjectPrefix *)item)->state.bytes;

    TRACE("ERROR: Unknown holdFlag(%d)\n", ((ItemStatePrefix *)rec)->flags);
    if (((ItemStatePrefix *)rec)->flags & 0x400000) {
        return gItemKinds[((ItemStatePrefix *)rec)->kind].unk28 * 0.5f;
    }
    return gItemKinds[((ItemStatePrefix *)rec)->kind].unk28;
}

/// Hide an item and suspend its participation in the world when its lifecycle permits it.
/// \param objectId Item handle; a missing item is ignored.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114C00 (0x80114C00).
void itemHideAndSuspend(s32 objectId) {
    u8 *liveObject;
    u8 *sp20;

    liveObject = objectFindLiveByHandle(objectId);
    if (liveObject != NULL) {
        sp20 = ((RuntimeObjectPrefix *)liveObject)->state.bytes;
        TRACE("checkin %p(%s)\n", liveObject, gItemKinds[((ItemStatePrefix *)sp20)->kind].text[0].name);
        if (((ItemStatePrefix *)sp20)->state != 4) {
            collisionGridUnregisterObject(liveObject, (s32 *)(sp20 + 0x50));
            itemTryRelease(liveObject, 1);
            itemSetMode(liveObject, 2);
            objectSetHierarchyFlags(liveObject, 2);
            ((ItemStatePrefix *)sp20)->state = 5;
        }
    }
}

/// Restore an item to visible, unheld participation in the world.
/// \param objectId Item handle; a missing item is ignored.
/// \returns Nothing.
///
/// \note Use for an initialized suspended item whose former holder relationship has already been cleared.
/// \note Original decompilation identifier: \c func_80114C80 (0x80114C80).
void itemRestoreToWorld(s32 objectId) {
    u8 *sp1C;
    u8 *itemState;

    sp1C = objectFindLiveByHandle(objectId);
    if (sp1C != NULL) {
        itemState = ((RuntimeObjectPrefix *)sp1C)->state.bytes;
        ((ItemStatePrefix *)itemState)->state = 0;
        objectClearHierarchyFlags(sp1C, 2);
        itemSetMode(sp1C, 0);
        TRACE("checkout %p(%s)\n", sp1C, gItemKinds[((ItemStatePrefix *)itemState)->kind].text[0].name);
    }
}

/// Profile storage for 166 counters with values from zero through fifteen.
/// \note Original identifier: \c D_800F0168 (0x800F0168).
extern u8 gProfilePackedCounters[];

/// Read a saved profile counter.
/// \param index Profile counter index from zero through 165.
/// \returns The counter value from zero through fifteen.
/// \note Original decompilation identifier: \c func_80114CC8 (0x80114CC8).
u8 profileGetCounter(s32 index) {
    u8 v = gProfilePackedCounters[index / 2];

    if (index & 1) {
        v >>= 4;
    }
    return v & 0xF;
}


/// Adjust a saved profile counter within its supported limits.
/// \param idx Profile counter index from zero through 165.
/// \param delta Signed adjustment; its sum with the current counter must fit a signed integer before
/// clamping.
/// \returns The updated counter value, limited to zero through fifteen.
/// \note Original decompilation identifier: \c func_80114D04 (0x80114D04).
u8 profileAddCounter(s32 idx, s32 delta) {
    s32 mask;
    u8 bits;
    u8 *p;

    delta += profileGetCounter(idx);
    if (delta < 0) {
        delta = 0;
    } else if (delta >= 16) {
        delta = 15;
    }
    bits = delta;
    mask = 0xF;
    if (idx & 1) {
        bits <<= 4;
        mask = 0xF0;
    }
    p = &gProfilePackedCounters[idx / 2];
    *p &= ~mask;
    *p |= bits;
    return delta;
}


/// Start timed visual direction motion when the item permits it, postponing its idle motion.
/// \param item Initialized item receiving the visual motion.
/// \param duration Positive finite motion duration in seconds.
/// \param pitchRate Pitch angular rate in degrees per second.
/// \param yawRate Yaw angular rate in degrees per second.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114DA4 (0x80114DA4).
void itemStartDirectionMotion(u8 *item, f32 duration, f32 pitchRate, f32 yawRate) {
    u8 *rec = ((RuntimeObjectPrefix *)item)->state.bytes;
    DirectionAngularRates pts[2];
    s32 i;

    ((ItemStatePrefix *)rec)->idleMotionDelay = 5.5f;
    if (!(((ItemStatePrefix *)rec)->flags & 0x8000)) {
        for (i = 0; i < 2; i++) {
            pts[i].pitch = pitchRate;
            pts[i].yaw = yawRate;
            directionAnimationSetNextPoseAngles(rec + 0x70, -40.0f, 0.0f, i);
        }
        directionAnimationStartMotion(rec + 0x70, duration, pts);
    }
}

/// Look up an item kind display name with the requested naming preference.
/// \param kind Item kind identifier from zero through 165.
/// \param nameType Zero prefers the alternate name and falls back to the default; one requests the default
/// name.
/// \returns The persistent name string, or null for an unsupported preference or missing name.
/// \note Original decompilation identifier: \c func_80114E74 (0x80114E74).
char *itemGetDisplayName(s32 kind, s32 nameType) {
    char *text;

    switch (nameType) {
    case 0:
        text = gItemKinds[kind].text[0].name;
        if (text != NULL) {
            return text;
        }
    case 1:
        return gItemKinds[kind].text[1].name;
    default:
        TRACE("Invalid itemNameType(%d) itemType = %d\n", nameType, kind);
        return NULL;
    }
}

/// Look up the label suffix used to identify an item kind class.
/// \param kind Item kind identifier from zero through 165.
/// \returns The persistent item-label suffix, an empty string for classes without a suffix, or null for an
/// unspecified or invalid class.
/// \note Original decompilation identifier: \c func_80114EE8 (0x80114EE8).
u8 *itemGetClassLabelSuffix(s32 kind) {
    switch (gItemKinds[kind].itemClass) {
    case 1:
    case 4:
    case 6:
        return (u8 *)"";
    case 2:
    case 3:
    case 5:
        return (u8 *)"(Item)";
    default:
        TRACE("Error: Invalid contentType:%d\n", gItemKinds[kind].itemClass);
    case 0:
        return NULL;
    }
}

/// Look up an item kind help text with the requested description preference.
/// \param kind Item kind identifier from zero through 165.
/// \param helpType Zero prefers alternate help text and falls back to the default; one requests the default
/// help text.
/// \returns The persistent help string, or null for an unsupported preference or missing text.
/// \note Original decompilation identifier: \c func_80114F4C (0x80114F4C).
char *itemGetHelpText(s32 kind, s32 helpType) {
    char *text;

    switch (helpType) {
    case 0:
        text = gItemKinds[kind].text[0].help;
        if (text != NULL) {
            return text;
        }
    case 1:
        return gItemKinds[kind].text[1].help;
    default:
        TRACE("Invalid itemHelpType(%d) itemType = %d\n", helpType, kind);
        return NULL;
    }
}


/// Read the remaining capacity for live items.
/// \returns Sixty minus the live item count; negative if callers have exceeded the nominal capacity.
/// \note Original decompilation identifier: \c func_80114FC0 (0x80114FC0).
s32 itemGetAvailableSlots(void) {
    return 0x3C - gLiveItemCount;
}

/// Check whether an item kind belongs to a configured category.
/// \param kind Item kind identifier to test.
/// \param category Category identifier from zero through eighteen.
/// \returns One if the kind belongs to the category, or zero otherwise.
/// \note Original decompilation identifier: \c func_80114FD4 (0x80114FD4).
s32 itemKindIsInCategory(s32 kind, s32 category) {
    ItemRange *r = &gItemCategoryRanges[category];

    if ((kind >= r->lo) && (r->hi >= kind)) {
        return 1;
    }
    return 0;
}

/// Look up an item kind class.
/// \param kind Item kind identifier from zero through 165.
/// \returns The configured item class identifier.
/// \note Original decompilation identifier: \c func_80115020 (0x80115020).
s8 itemGetClass(s32 kind) {
    return gItemKinds[kind].itemClass;
}

/// Look up the progress-field selector controlling an item kind’s availability.
/// \param kind Item kind identifier from zero through 165.
/// \returns The configured availability progress-field selector.
/// \note Original decompilation identifier: \c func_80115044 (0x80115044).
s16 itemGetAvailabilityFlag(s32 kind) {
    return gItemKinds[kind].unk72;
}
