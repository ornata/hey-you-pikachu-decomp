/// \file
/// Provides the retained serial-interface message queue used by controller devices.

#ifndef SERIAL_QUEUE_H
#define SERIAL_QUEUE_H

#include <ultra64.h>

/// Get the initialized serial-interface completion queue.
/// \returns Borrowed shared queue, valid for the controller subsystem lifetime.
/// \note Original address: 0x80002870.
OSMesgQueue *serialGetMessageQueue(void);

#endif
