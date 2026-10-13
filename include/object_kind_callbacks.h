/// \file
/// Defines runtime object-kind lifecycle, drawing and queued-response hooks.
#ifndef OBJECT_KIND_CALLBACKS_H
#define OBJECT_KIND_CALLBACKS_H
#include "object_response.h"

/// Initialize, update or release kind-specific state according to the selected hook.
/// \param object Live object of the selected runtime kind; update and destroy
/// require the kind state established by initialization.
/// \returns Nothing.
typedef void (*ObjectLifecycleHandler)(u8 *object);
/// Prepare drawing for one object.
/// \param object Live initialized object.
/// \returns Borrowed drawing commands, or null when no commands are supplied;
/// referenced storage must remain valid through graphics-task completion.
typedef Gfx *(*ObjectDrawHandler)(u8 *object);
/// Prepare drawing of a child under its parent.
/// \param parent Live parent object.
/// \param child Live child object.
/// \returns Nothing.
typedef void (*ObjectChildDrawHandler)(u8 *parent, u8 *child);

/// Persistent hooks indexed by runtime object kind; optional hooks may be null.
typedef struct ObjectKindCallbacks {
    ObjectLifecycleHandler init;      ///< Initialize kind-specific resources.
    ObjectLifecycleHandler destroy;   ///< Release kind-specific resources.
    ObjectLifecycleHandler update;    ///< Advance object behavior.
    ObjectDrawHandler draw;           ///< Prepare object drawing.
    ObjectChildDrawHandler drawChild; ///< Prepare child drawing.
    ObjectResponseHandler response;   ///< Consume a borrowed queued response.
} ObjectKindCallbacks;

/// Strided interior view beginning at each kind's destruction hook.
/// Borrows table storage and overlaps the next record; never allocate this view.
typedef struct ObjectKindDestroyEntry {
    ObjectLifecycleHandler destroy; ///< Optional kind-specific resource release hook.
    u8 following[0x14];             ///< Remaining bytes through the next destruction-hook view.
} ObjectKindDestroyEntry;

/// Strided interior view beginning at each kind's response hook.
/// Preserves the original interior symbol and does not describe a separate allocation.
typedef struct ObjectKindResponseEntry {
    ObjectResponseHandler response; ///< Queued-response hook.
    u8 following[0x14];             ///< Bytes belonging to subsequent table hooks.
} ObjectKindResponseEntry;

typedef char ObjectKindCallbacksSizeCheck[(sizeof(ObjectKindCallbacks) == 0x18) ? 1 : -1];
typedef char ObjectKindInitOffsetCheck[(((u32) & ((ObjectKindCallbacks *)0)->init) == 0) ? 1 : -1];
typedef char ObjectKindUpdateOffsetCheck[(((u32) & ((ObjectKindCallbacks *)0)->update) == 8) ? 1 : -1];
typedef char ObjectKindDrawOffsetCheck[(((u32) & ((ObjectKindCallbacks *)0)->draw) == 0xC) ? 1 : -1];
typedef char ObjectKindChildDrawOffsetCheck[(((u32) & ((ObjectKindCallbacks *)0)->drawChild) == 0x10) ? 1 : -1];
typedef char ObjectKindDestroyOffsetCheck[(((u32) & ((ObjectKindCallbacks *)0)->destroy) == 4) ? 1 : -1];
typedef char ObjectKindDestroyEntrySizeCheck[(sizeof(ObjectKindDestroyEntry) == 0x18) ? 1 : -1];
typedef char ObjectKindResponseOffsetCheck[(((u32) & ((ObjectKindCallbacks *)0)->response) == 0x14) ? 1 : -1];
typedef char ObjectKindResponseEntrySizeCheck[(sizeof(ObjectKindResponseEntry) == 0x18) ? 1 : -1];
/// Resident lifecycle, drawing and response hooks indexed by object kind.
/// \note Original identifier: \c D_80078140 (0x80078140).
extern ObjectKindCallbacks gObjectKindCallbacks[];

/// Interior destruction-hook view of the resident callback table.
/// \note Original identifier: \c D_80078144 (0x80078144).
extern ObjectKindDestroyEntry gObjectKindDestroyEntries[];

/// Interior response-hook view of the resident callback table.
/// \note Original identifier: \c D_80078154 (0x80078154).
extern ObjectKindResponseEntry gObjectKindResponseEntries[];

#endif
