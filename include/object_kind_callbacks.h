/// \file
/// Defines runtime object-kind lifecycle, drawing and queued-response hooks.
#ifndef OBJECT_KIND_CALLBACKS_H
#define OBJECT_KIND_CALLBACKS_H
#include "object_response.h"

/// Initialize, update or release kind-specific state according to the selected hook.
/// \param object Live object of the selected runtime kind.
/// \returns Nothing.
typedef void (*ObjectLifecycleHandler)(u8 *object);
/// Prepare drawing for one object.
/// \param object Live initialized object.
/// \returns Display list used for the object's drawing.
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

/// Strided interior view beginning at each kind's response hook.
/// Preserves the original interior symbol and does not describe a separate allocation.
typedef struct ObjectKindResponseEntry {
    ObjectResponseHandler response; ///< Queued-response hook.
    u8 following[0x14];             ///< Bytes belonging to subsequent table hooks.
} ObjectKindResponseEntry;

typedef char ObjectKindCallbacksSizeCheck[(sizeof(ObjectKindCallbacks) == 0x18) ? 1 : -1];
typedef char ObjectKindResponseOffsetCheck[(((u32) & ((ObjectKindCallbacks *)0)->response) == 0x14) ? 1 : -1];
typedef char ObjectKindResponseEntrySizeCheck[(sizeof(ObjectKindResponseEntry) == 0x18) ? 1 : -1];
#endif
