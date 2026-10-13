#include "controller_pad.h"
#include "common.h"

OSMesgQueue *serialGetMessageQueue(void);
/// Latest hardware samples for the four controller ports.
/// \note Original identifier: \c D_800AD400 (0x800AD400).
extern OSContPad gControllerPadSamples[4];
void _bzero(void *, u32);

/// Poll connected controllers and refresh the cached pad samples.
/// Blocks until the serial-interface read completes. If no controllers are
/// connected, leaves the cached samples unchanged.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerPollPads (0x80003A40).
void controllerPollPads(void) {
    OSMesgQueue *completionQueue;
    OSMesgQueue *serialQueue;

    if (controllerGetConnectedPortMask() != 0) {
        serialQueue = serialGetMessageQueue();
        completionQueue = serialQueue;
        osContStartReadData(serialQueue);
        osRecvMesg(completionQueue, NULL, 1);
        osContGetReadData(gControllerPadSamples);
    }
}

/// Clear all four cached controller samples without reading the hardware.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerClearPadCache (0x80003A98).
void controllerClearPadCache(void) {
    _bzero(gControllerPadSamples, 0x18);
}

/// Copy a controller's latest cached sample without reading the hardware.
/// \param pad Output sample; must point to writable storage. Cleared when the
/// port is invalid or disconnected; an errored connected sample is still copied.
/// \param port Controller port index, normally 0 through 3.
/// \returns 1 if the port is connected and its cached sample has no error,
/// 0 otherwise.
/// \note Original decompilation identifier: \c controllerReadCachedPad (0x80003AC0).
s32 controllerReadCachedPad(OSContPad *pad, s32 port) {
    if (controllerIsPortConnected(port)) {
        *pad = gControllerPadSamples[port];
        return pad->errno == 0;
    }
    _bzero(pad, sizeof(OSContPad));
    return 0;
}
