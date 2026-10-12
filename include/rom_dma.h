/// \file
/// Defines bounded asynchronous cartridge reads and completion tracking.

#ifndef ROM_DMA_H
#define ROM_DMA_H

#include <ultra64.h>

/// Owned request storage and completion state for outstanding cartridge reads.
typedef struct DmaQueue {
    s32 pending;        ///< Number of outstanding reads.
    s32 cursor;         ///< Next request slot to submit.
    s32 capacity;       ///< Number of request slots and completion messages.
    OSMesgQueue queue;  ///< Completion queue owning its message buffer.
    OSIoMesg *messages; ///< Owned request records, valid until all reads finish.
} DmaQueue;

typedef char DmaQueueSizeCheck[(sizeof(DmaQueue) == 0x28) ? 1 : -1];
#define DMA_OFFSET(field) ((u32) & (((DmaQueue *)0)->field))
typedef char DmaQueueCompletionOffsetCheck[(DMA_OFFSET(queue) == 0xC) ? 1 : -1];
typedef char DmaQueueMessagesOffsetCheck[(DMA_OFFSET(messages) == 0x24) ? 1 : -1];
#undef DMA_OFFSET

/// Run the ROM-I/O startup hook, which is empty in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaInitializeHook (0x80037AE0).
void romDmaInitializeHook(void);

/// Initialize the peripheral-interface manager for cartridge transfers.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaCreatePiManager (0x80037AE8).
void romDmaCreatePiManager(void);

/// Read ROM into RAM and wait until the entire read completes.
/// \param romAddress Starting cartridge ROM address.
/// \param destination Writable RAM destination with room for the entire read.
/// \param byteCount Nonnegative number of bytes to copy; zero performs no DMA.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaReadBlocking (0x80037B1C).
void romDmaReadBlocking(u32 romAddress, u8 *destination, s32 byteCount);

/// Initialize a bounded asynchronous ROM-transfer queue.
/// \param queueState Writable DMA-queue state (0x28 bytes in this ABI).
/// \param capacity Positive number of concurrent DMA requests to support.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaInitializeQueue (0x80037BEC).
void romDmaInitializeQueue(DmaQueue *queueState, s32 capacity);

/// Wait for outstanding ROM reads, then release the queue's resources.
/// \param queueState Previously initialized DMA-queue state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaDestroyQueue (0x80037C74).
void romDmaDestroyQueue(DmaQueue *queueState);

/// Account for one completion message already removed from a DMA queue.
/// \param pendingCount Pointer to the queue's pending-request counter.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaAcknowledgeCompletion (0x80037CC8).
void romDmaAcknowledgeCompletion(s32 *pendingCount);

/// Consume all currently available DMA completion messages without blocking.
/// \param queueState Initialized DMA-queue state whose completion status is updated.
/// \returns Number of DMA requests still pending after consuming completions.
/// \note Original decompilation identifier: \c romDmaPollQueue (0x80037CD8).
s32 romDmaPollQueue(DmaQueue *queueState);

/// Submit one asynchronous ROM read, waiting for a slot if the queue is full.
/// \param queue Initialized queue with positive capacity.
/// \param romAddress Starting cartridge ROM address.
/// \param destination RAM address; storage must remain valid until completion.
/// \param byteCount Number of bytes in this DMA request.
/// \returns Number of outstanding requests after submitting the read.
/// \note Original decompilation identifier: \c romDmaSubmitRead (0x80037D40).
s32 romDmaSubmitRead(DmaQueue *queue, u32 romAddress, u8 *destination, u32 byteCount);

/// Queue an asynchronous ROM read.
/// May wait for queue slots; does not wait for all submitted reads to finish.
/// \param queueState Initialized DMA-queue state.
/// \param romAddress Starting cartridge ROM address.
/// \param destination Writable RAM address with room for the entire read; storage
/// must remain valid until all queued reads complete.
/// \param byteCount Number of bytes to copy; zero submits no requests.
/// \returns Current outstanding-request count after submission.
/// \note Original decompilation identifier: \c romDmaReadQueued (0x80037E14).
s32 romDmaReadQueued(DmaQueue *queueState, u32 romAddress, u8 *destination, u32 byteCount);

/// Block until all outstanding requests in a DMA queue have completed.
/// \param queueState Initialized DMA-queue state with a valid pending count.
/// \returns Nothing.
/// \note Original decompilation identifier: \c romDmaWaitForQueue (0x80037EA0).
void romDmaWaitForQueue(DmaQueue *queueState);

#endif
