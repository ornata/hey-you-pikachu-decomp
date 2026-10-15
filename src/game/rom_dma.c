#include "game_heap.h"
#include "heap_free.h"
#include "common.h"
#include "heap_alloc.h"
#include "rom_dma.h"
#include "heap_types.h"

/// Run the ROM-I/O startup hook, which is empty in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaInitializeHook (0x80037AE0).
void romDmaInitializeHook(void) {
}

/// Persistent peripheral-interface manager queue for cartridge transfers.
/// \note Original identifier: \c D_800F2620 (0x800F2620).
extern OSMesgQueue gRomPiManagerQueue;
/// Backing storage for the peripheral-interface manager message queue.
/// \note Original identifier: \c D_800F2638 (0x800F2638).
extern OSMesg gRomPiManagerMessages[200];

/// Initialize the peripheral-interface manager for cartridge transfers.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaCreatePiManager (0x80037AE8).
void romDmaCreatePiManager(void) {
    osCreatePiManager(0x96, &gRomPiManagerQueue, gRomPiManagerMessages, 0xC8);
}

/// Read ROM into RAM and wait until the entire read completes.
/// \param romAddress Starting cartridge ROM address.
/// \param destination Writable RAM destination with room for the entire read.
/// \param byteCount Nonnegative number of bytes to copy; zero performs no DMA.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaReadBlocking (0x80037B1C).
void romDmaReadBlocking(u32 romAddress, u8 *destination, s32 byteCount) {
    OSMesgQueue completionQueue;
    OSMesg completionMessage;
    OSIoMesg ioMessage;
    s32 chunkBytes;
    s32 remainingBytes;
    osCreateMesgQueue(&completionQueue, &completionMessage, 1);
    osInvalDCache(destination, byteCount);
    remainingBytes = byteCount;
    if (byteCount != 0) {
        do {
            if ((u32)remainingBytes >= 0x4001U) {
                chunkBytes = 0x4000;
            } else {
                chunkBytes = remainingBytes;
            }
            osPiStartDma(&ioMessage, 0, OS_READ, romAddress, destination, (u32)chunkBytes, &completionQueue);
            osRecvMesg(&completionQueue, NULL, OS_MESG_BLOCK);
            remainingBytes -= chunkBytes;
            romAddress += chunkBytes;
            destination += chunkBytes;
        } while (remainingBytes != 0);
    }
}


/// Initialize a bounded asynchronous ROM-transfer queue.
/// \param queueState Writable DMA-queue state (0x28 bytes in this ABI).
/// \param capacity Positive number of concurrent DMA requests to support.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaInitializeQueue (0x80037BEC).
void romDmaInitializeQueue(DmaQueue *queueState, s32 capacity) {
    OSMesg *completionMessages;

    completionMessages = heapAllocLocal(&gGameHeap, capacity * 4, 0x55);
    queueState->messages = heapAllocLocal(&gGameHeap, capacity * 0x18, 0x5A);
    osCreateMesgQueue(&queueState->queue, completionMessages, capacity);
    queueState->pending = 0;
    queueState->cursor = 0;
    queueState->capacity = capacity;
}


/// Wait for outstanding ROM reads, then release the queue's resources.
/// \param queueState Previously initialized DMA-queue state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaDestroyQueue (0x80037C74).
void romDmaDestroyQueue(DmaQueue *queueState) {
    romDmaWaitForQueue(queueState);
    heapFreeLocal(&gGameHeap, queueState->messages);
    heapFreeLocal(&gGameHeap, queueState->queue.msg);
    queueState->pending = 0;
    queueState->cursor = 0;
    queueState->capacity = 0;
}

/// Account for one completion message already removed from a DMA queue.
/// \param pendingCount Pointer to the queue's pending-request counter.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaAcknowledgeCompletion (0x80037CC8).
void romDmaAcknowledgeCompletion(s32 *pendingCount) {
    *pendingCount -= 1;
}

/// Consume all currently available DMA completion messages without blocking.
/// \param queueState Initialized DMA-queue state whose completion status is updated.
/// \returns Number of DMA requests still pending after consuming completions.
/// \note Original decompilation identifier: \c romDmaPollQueue (0x80037CD8).
s32 romDmaPollQueue(DmaQueue *queueState) {
    OSMesgQueue *completionQueue;
    completionQueue = &queueState->queue;
    if (osRecvMesg(completionQueue, NULL, OS_MESG_NOBLOCK) == 0) {
        do {
            romDmaAcknowledgeCompletion(&queueState->pending);
        } while (osRecvMesg(completionQueue, NULL, OS_MESG_NOBLOCK) == 0);
    }
    return queueState->pending;
}

/// Submit one asynchronous ROM read, waiting for a slot if the queue is full.
/// \param queue Initialized queue with positive capacity.
/// \param romAddress Starting cartridge ROM address.
/// \param destination RAM address; storage must remain valid until completion.
/// \param byteCount Number of bytes in this DMA request.
/// \returns Number of outstanding requests after submitting the read.
/// \note Original decompilation identifier: \c romDmaSubmitRead (0x80037D40).
s32 romDmaSubmitRead(DmaQueue *queue, u32 romAddress, u8 *destination, u32 byteCount) {
    /* The ROM gives this saved completion-queue pointer its own stack home.
     * A one-element array reproduces that inferred local shape; only index 0 is used. */
    OSMesgQueue *completionQueue[1];
    if (romDmaPollQueue(queue) == queue->capacity) {
        osRecvMesg(&queue->queue, NULL, OS_MESG_BLOCK);
        romDmaAcknowledgeCompletion(&queue->pending);
    }
    completionQueue[0] = &queue->queue;
    osInvalDCache((void *)destination, byteCount);
    osPiStartDma(&queue->messages[queue->cursor], OS_MESG_PRI_NORMAL, OS_READ, romAddress, (void *)destination,
                 byteCount, completionQueue[0]);
    queue->pending++;
    queue->cursor++;
    if (queue->cursor >= queue->capacity) {
        queue->cursor = 0;
    }
    return queue->pending;
}

/// Queue an asynchronous ROM read.
/// May wait for queue slots; does not wait for all submitted reads to finish.
/// \param queueState Initialized DMA-queue state.
/// \param romAddress Starting cartridge ROM address.
/// \param destination Writable RAM address with room for the entire read; storage
/// must remain valid until all queued reads complete.
/// \param byteCount Number of bytes to copy; zero submits no requests.
/// \returns Current outstanding-request count after submission.
/// \note Original decompilation identifier: \c romDmaReadQueued (0x80037E14).
s32 romDmaReadQueued(DmaQueue *queueState, u32 romAddress, u8 *destination, u32 byteCount) {
    u32 sourceCursor;
    u8 *destinationCursor;
    u32 chunkBytes;
    u32 remainingBytes;

    sourceCursor = romAddress;
    destinationCursor = destination;
    remainingBytes = byteCount;
    if (byteCount != 0) {
        do {
            if (remainingBytes >= 0x4001U) {
                chunkBytes = 0x4000;
            } else {
                chunkBytes = remainingBytes;
            }
            romDmaSubmitRead(queueState, sourceCursor, destinationCursor, chunkBytes);
            remainingBytes -= chunkBytes;
            sourceCursor += chunkBytes;
            destinationCursor += chunkBytes;
        } while (remainingBytes != 0);
    }
    return queueState->pending;
}

/// Block until all outstanding requests in a DMA queue have completed.
/// \param queueState Initialized DMA-queue state with a valid pending count.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaWaitForQueue (0x80037EA0).
void romDmaWaitForQueue(DmaQueue *queueState) {
    if (queueState->pending > 0) {
        do {
            osRecvMesg(&queueState->queue, NULL, OS_MESG_BLOCK);
            romDmaAcknowledgeCompletion(&queueState->pending);
        } while (queueState->pending > 0);
    }
}
