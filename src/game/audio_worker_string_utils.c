#include "audio_worker.h"
#include "audio_dma.h"
#include "scheduler.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "string_bytes.h"
void memoryFillBytes(u8 *p, s32 n, u8 c);
/// Number of sample DMA reads whose completions must be collected this frame.
/// \note Original identifier: \c D_800F8EF0 (0x800F8EF0).
extern u8 gAudioDmaPendingReads;
/// Cache age counter advanced after sample DMA frame completion.
/// \note Original identifier: \c D_800F8EF4 (0x800F8EF4).
extern u32 gAudioDmaCacheFrame;
/// Number of allocated audio sample cache entries.
/// \note Original identifier: \c D_800F8EF8 (0x800F8EF8).
extern u32 gAudioDmaCacheCapacity;
/// Completion queue for audio sample ROM reads.
/// \note Original identifier: \c D_800F8F08 (0x800F8F08).
extern OSMesgQueue gAudioDmaCompletionQueue;
/// Cache-owned sample page retained for audio synthesis and its pending DMA read.
typedef struct AudioDmaCacheEntry {
    u8 *buffer;       ///< Audio-heap allocation containing the page and read-ahead bytes.
    u32 address;      ///< Cartridge page address, or zero before assignment.
    u32 lastUseFrame; ///< Cache frame in which this entry was last used.
} AudioDmaCacheEntry;
typedef char AudioDmaCacheEntrySizeCheck[(sizeof(AudioDmaCacheEntry) == 0xC) ? 1 : -1];
typedef char AudioDmaCacheAddressOffsetCheck[(((u32) & ((AudioDmaCacheEntry *)0)->address) == 4) ? 1 : -1];
/// Owned audio cache entries retaining sample buffers, ROM pages and last-use ages.
/// \note Original identifier: \c D_800F8F24 (0x800F8F24).
extern AudioDmaCacheEntry *gAudioDmaCacheEntries;

s32 audioDmaResolveSample(s32, s32, void *);

/// Provide the shared audio sample DMA callback.
/// \param state Writable callback-state pointer slot, receiving null.
/// \returns The audio sample DMA callback.
/// \note Original decompilation identifier: \c func_80049160 (0x80049160).
ALDMAproc audioDmaCreateCallback(void *state) {
    *(void **)state = NULL;
    return &audioDmaResolveSample;
}

/// Wait for outstanding audio sample reads and advance the cache frame.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80049170 (0x80049170).
void audioDmaFinishFrame(void) {
    s32 var_s0;
    {
        OSMesg sp30;
        sp30 = NULL;
        var_s0 = 0;
        if ((s32)gAudioDmaPendingReads > 0) {
            do {
                osRecvMesg(&gAudioDmaCompletionQueue, &sp30, 1);
                var_s0 += 1;
            } while (var_s0 < (s32)gAudioDmaPendingReads);
        }
    }
    gAudioDmaPendingReads = 0;
    if (gAudioDmaCacheFrame < gAudioDmaCacheCapacity) {
        gAudioDmaCacheEntries[gAudioDmaCacheFrame].lastUseFrame = gAudioDmaCacheFrame;
    }
    gAudioDmaCacheFrame += 1;
}

/// Base size in bytes of an audio sample cache page.
/// \note Original identifier: \c D_800F8EFC (0x800F8EFC).
/// Base-two logarithm of the sample cache page size.
/// \note Original identifier: \c D_800F8F00 (0x800F8F00).
/// Additional bytes allocated and read beyond each sample cache page.
/// \note Original identifier: \c D_8009D940 (0x8009D940).
extern s32 gAudioDmaPageBytes, gAudioDmaPageShift, gAudioDmaReadAheadBytes;
/// Owned ROM-page lookup storing cache slot plus one, or zero for an uncached page.
/// \note Original identifier: \c D_800F8F20 (0x800F8F20).
extern u8 *gAudioDmaPageSlots;
/// Owned I/O message records used by outstanding sample DMA reads.
/// \note Original identifier: \c D_800F8F28 (0x800F8F28).
extern OSIoMesg *gAudioDmaIoMessages;
/// Initialize the shared audio sample cache.
/// \param heap Audio heap supplying cache storage, kept valid while the cache is used.
/// \param slots Positive number of cache entries, at most 255.
/// \param shift Base-two logarithm of the cache page size in bytes, from 0 through 24.
/// \returns 1.
/// \note Original decompilation identifier: \c func_80049234 (0x80049234).
s32 audioDmaInitialize(ALHeap *heap, s32 slots, s32 shift) {
    s32 pages;
    s32 i;
    gAudioDmaCacheFrame = slots;
    gAudioDmaCacheCapacity = slots;
    gAudioDmaPageShift = shift;
    gAudioDmaPageBytes = 1 << shift;
    pages = 0x1000000 / gAudioDmaPageBytes;
    gAudioDmaPageSlots = alHeapDBAlloc(NULL, 0, heap, 1, pages);
    gAudioDmaCacheEntries = alHeapDBAlloc(NULL, 0, heap, 1, gAudioDmaCacheCapacity * sizeof(AudioDmaCacheEntry));
    gAudioDmaIoMessages = alHeapDBAlloc(NULL, 0, heap, 1, gAudioDmaCacheCapacity * sizeof(OSIoMesg));
    for (i = 0; i < pages; i++)
        gAudioDmaPageSlots[i] = 0;
    for (i = 0; i < (s32)gAudioDmaCacheCapacity; i++) {
        gAudioDmaCacheEntries[i].lastUseFrame = gAudioDmaCacheFrame;
        gAudioDmaCacheEntries[i].buffer = alHeapDBAlloc(NULL, 0, heap, 1, gAudioDmaPageBytes + gAudioDmaReadAheadBytes);
        gAudioDmaCacheEntries[i].address = 0;
    }
    osCreateMesgQueue(&gAudioDmaCompletionQueue,
                      alHeapDBAlloc(NULL, 0, heap, 1, gAudioDmaCacheCapacity * sizeof(OSMesg)), gAudioDmaCacheCapacity);
    gAudioDmaPendingReads = 0;
    return 1;
}

/// Request cached audio sample data for a ROM address.
/// Requests between frame completions must fit within the cache capacity.
/// \param address Sample ROM address within the configured ROM address space.
/// \param length Unused requested byte count supplied by the audio DMA interface.
/// \param state Unused callback context.
/// \returns The physical sample-buffer address; newly requested data is ready after audioDmaFinishFrame completes.
/// \note Original decompilation identifier: \c func_800494A4 (0x800494A4).
s32 audioDmaResolveSample(s32 address, s32 length, void *state) {
    s32 page;
    s32 slot;
    s32 i;
    page = address >> gAudioDmaPageShift;
    slot = gAudioDmaPageSlots[page];
    if (slot == 0) {
        for (i = 1, slot = 0; i < (s32)gAudioDmaCacheCapacity; i++) {
            if (gAudioDmaCacheEntries[i].lastUseFrame < gAudioDmaCacheEntries[slot].lastUseFrame)
                slot = i;
        }
        if (gAudioDmaCacheEntries[slot].address != 0) {
            gAudioDmaPageSlots[gAudioDmaCacheEntries[slot].address >> gAudioDmaPageShift] = 0;
        }
        gAudioDmaPageSlots[page] = slot + 1;
        gAudioDmaCacheEntries[slot].address = ~(gAudioDmaPageBytes - 1) & address;
        osPiStartDma(&gAudioDmaIoMessages[gAudioDmaPendingReads++], OS_MESG_PRI_HIGH, OS_READ,
                     gAudioDmaCacheEntries[slot].address, gAudioDmaCacheEntries[slot].buffer,
                     gAudioDmaPageBytes + gAudioDmaReadAheadBytes, &gAudioDmaCompletionQueue);
    } else {
        slot--;
    }
    gAudioDmaCacheEntries[slot].lastUseFrame = gAudioDmaCacheFrame;
    return (s32)gAudioDmaCacheEntries[slot].buffer + address - gAudioDmaCacheEntries[slot].address + 0x80000000;
}

/// Queue acknowledging audio worker termination.
/// \note Original identifier: \c D_800FB118 (0x800FB118).
extern OSMesgQueue gAudioWorkerStopQueue;
/// Nonzero when audio worker shutdown has been requested.
/// \note Original identifier: \c D_800FB134 (0x800FB134).
extern s32 gAudioWorkerStopRequested;

/// Request audio worker shutdown and wait for its acknowledgement.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80049680 (0x80049680).
void audioWorkerStopAndWait(void) {
    gAudioWorkerStopRequested = 1;
    osRecvMesg(&gAudioWorkerStopQueue, NULL, 1);
}

/// Retained audio worker prefix with audio-heap-owned command and sample buffers.
/// Interior linker symbols borrow the embedded thread, queues and stop flag.
typedef struct AudioWorkerState {
    u64 stack[0x400];           ///< Thread stack, whose upper boundary is the embedded thread.
    OSThread thread;            ///< Operating-system worker thread.
    OSMesgQueue aiQueue;        ///< Audio-interface completion queue.
    OSMesg aiMessage;           ///< Owned message slot for aiQueue.
    OSMesgQueue taskQueue;      ///< Synthesis-task completion queue.
    OSMesg taskMessage;         ///< Owned message slot for taskQueue.
    OSMesgQueue stopQueue;      ///< Shutdown acknowledgment queue.
    OSMesg stopMessage;         ///< Owned message slot for stopQueue.
    s32 stop;                   ///< Nonzero requests shutdown after the current buffer.
    AudioRenderCallback render; ///< Borrowed synthesis callback retained until shutdown.
    s32 samples;                ///< Stereo sample frames per output buffer.
    u64 *ucode;                 ///< Borrowed audio microcode text.
    u64 *ucodeData;             ///< Borrowed audio microcode data.
    s32 maxCommands;            ///< Allocated synthesis-command capacity.
    Acmd *commands;             ///< Audio-heap-owned command buffer.
    OSMesgQueue *scheduler;     ///< Borrowed scheduler audio command queue.
    void *buffers[2];           ///< Audio-heap-owned stereo output buffers.
    s32 index;                  ///< Output-buffer slot selected for the next rendering call.
} AudioWorkerState;

typedef char AudioWorkerStateSizeCheck[(sizeof(AudioWorkerState) == 0x2230) ? 1 : -1];
#define AUDIO_WORKER_OFFSET_CHECK(field, offset)                                                                       \
    typedef char AudioWorker_##field##OffsetCheck[(((u32) & ((AudioWorkerState *)0)->field) == (offset)) ? 1 : -1]
AUDIO_WORKER_OFFSET_CHECK(thread, 0x2000);
AUDIO_WORKER_OFFSET_CHECK(aiQueue, 0x21B0);
AUDIO_WORKER_OFFSET_CHECK(taskQueue, 0x21CC);
AUDIO_WORKER_OFFSET_CHECK(stopQueue, 0x21E8);
AUDIO_WORKER_OFFSET_CHECK(stop, 0x2204);
AUDIO_WORKER_OFFSET_CHECK(render, 0x2208);
AUDIO_WORKER_OFFSET_CHECK(samples, 0x220C);
AUDIO_WORKER_OFFSET_CHECK(ucode, 0x2210);
AUDIO_WORKER_OFFSET_CHECK(ucodeData, 0x2214);
AUDIO_WORKER_OFFSET_CHECK(maxCommands, 0x2218);
AUDIO_WORKER_OFFSET_CHECK(commands, 0x221C);
AUDIO_WORKER_OFFSET_CHECK(scheduler, 0x2220);
AUDIO_WORKER_OFFSET_CHECK(buffers, 0x2224);
AUDIO_WORKER_OFFSET_CHECK(index, 0x222C);
#undef AUDIO_WORKER_OFFSET_CHECK

void audioWorkerThreadMain(void *);
/// Shared audio worker storage containing its stack, thread and rendering state.
/// \note Original identifier: \c D_800F8F30 (0x800F8F30).
extern u8 gAudioWorkerStorage[];
/// Audio worker thread object; its address also bounds the worker stack.
/// \note Original identifier: \c D_800FAF30 (0x800FAF30).
extern OSThread gAudioWorkerThread;
/// Queue receiving audio-interface completion events.
/// \note Original identifier: \c D_800FB0E0 (0x800FB0E0).
extern OSMesgQueue gAudioWorkerAiQueue;
/// Single message slot backing the audio-interface completion queue.
/// \note Original identifier: \c D_800FB0F8 (0x800FB0F8).
extern OSMesg gAudioWorkerAiMessage;
/// Queue receiving scheduler completion of audio rendering tasks.
/// \note Original identifier: \c D_800FB0FC (0x800FB0FC).
extern OSMesgQueue gAudioWorkerTaskQueue;
/// Single message slot backing the audio task-completion queue.
/// \note Original identifier: \c D_800FB114 (0x800FB114).
extern OSMesg gAudioWorkerTaskMessage;
/// Single message slot backing the worker shutdown-acknowledgement queue.
/// \note Original identifier: \c D_800FB130 (0x800FB130).
extern OSMesg gAudioWorkerStopMessage;

/// Initialize and start the audio rendering worker.
/// \param scheduler Scheduler supplying the audio task command queue.
/// \param heap Audio heap supplying command and output buffers.
/// \param id Operating-system thread identifier.
/// \param pri Audio worker thread priority.
/// \param render Callback address accepting a command buffer, command-count
/// output, physical output-buffer address, and stereo sample-frame count.
/// It must stay callable until the worker stops; a zero command count ends playback.
/// \param ucode Audio microcode text address, kept valid until the worker stops.
/// \param ucodeData Audio microcode data address, kept valid until the worker stops.
/// \param samples Number of stereo sample frames rendered per output buffer.
/// \param maxCommands Positive command-buffer capacity in audio commands;
/// the render callback must not exceed it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800496B8 (0x800496B8).
void audioWorkerStart(GameScheduler *scheduler, ALHeap *heap, OSId id, OSPri pri, AudioRenderCallback render,
                      u64 *ucode, u64 *ucodeData, s32 samples, s32 maxCommands) {
    s32 i;

    ((AudioWorkerState *)gAudioWorkerStorage)->maxCommands = maxCommands;
    ((AudioWorkerState *)gAudioWorkerStorage)->samples = samples;
    ((AudioWorkerState *)gAudioWorkerStorage)->render = render;
    ((AudioWorkerState *)gAudioWorkerStorage)->ucode = ucode;
    ((AudioWorkerState *)gAudioWorkerStorage)->ucodeData = ucodeData;
    ((AudioWorkerState *)gAudioWorkerStorage)->commands = alHeapDBAlloc(NULL, 0, heap, 1, maxCommands * 8);
    ((AudioWorkerState *)gAudioWorkerStorage)->scheduler = schedulerGetAudioCommandQueue(scheduler);
    for (i = 0; i < 2; i++) {
        ((AudioWorkerState *)gAudioWorkerStorage)->buffers[i] = alHeapDBAlloc(NULL, 0, heap, 1, samples * 4);
    }
    ((AudioWorkerState *)gAudioWorkerStorage)->stop = 0;
    osCreateMesgQueue(&gAudioWorkerAiQueue, &gAudioWorkerAiMessage, 1);
    osCreateMesgQueue(&gAudioWorkerTaskQueue, &gAudioWorkerTaskMessage, 1);
    osCreateMesgQueue(&gAudioWorkerStopQueue, &gAudioWorkerStopMessage, 1);
    osSetEventMesg(OS_EVENT_AI, &gAudioWorkerAiQueue, NULL);
    osCreateThread(&gAudioWorkerThread, id, audioWorkerThreadMain, gAudioWorkerStorage, &gAudioWorkerThread, pri);
    osStartThread(&gAudioWorkerThread);
}

/// Audio RSP microcode text start; also marks the end of the RSP boot code.
/// \note Original identifier: \c D_80060120 (0x80060120).
extern u64 gAudioRspMicrocodeTextStart[];
/// Render and play audio until shutdown or the renderer produces no commands.
/// \param arg Initialized audio worker state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8004982C (0x8004982C).
void audioWorkerThreadMain(void *arg) {
    AudioWorkerState *audio = arg;
    s32 commandCount;
    void *buffer;
    SchedulerTask task;
    OSMesg taskMessage;
    OSMesg aiMessage;
    do {
        buffer = audio->buffers[audio->index];
        audio->index ^= 1;
        audio->render(audio->commands, &commandCount, (s16 *)((u32)buffer + 0x80000000), audio->samples);
        if (commandCount == 0)
            break;
        task.next = NULL;
        task.reply = &audio->taskQueue;
        task.message = NULL;
        task.flags = 0;
        task.task.t.type = M_AUDTASK;
        task.task.t.flags = 0;
        task.task.t.data_ptr = (u64 *)audio->commands;
        task.task.t.data_size = commandCount * sizeof(Acmd);
        task.task.t.ucode_boot = (u64 *)rspbootTextStart;
        task.task.t.ucode_boot_size = (u8 *)gAudioRspMicrocodeTextStart - (u8 *)rspbootTextStart;
        task.task.t.ucode = audio->ucode;
        task.task.t.ucode_data = audio->ucodeData;
        task.task.t.ucode_data_size = SP_UCODE_DATA_SIZE;
        task.task.t.dram_stack = NULL;
        task.task.t.dram_stack_size = 0;
        task.task.t.output_buff = NULL;
        task.task.t.output_buff_size = NULL;
        task.task.t.yield_data_ptr = NULL;
        task.task.t.yield_data_size = 0;
        audioDmaFinishFrame();
        osSendMesg(audio->scheduler, &task, OS_MESG_BLOCK);
        taskMessage = NULL;
        osRecvMesg(&audio->taskQueue, &taskMessage, OS_MESG_BLOCK);
        osAiSetNextBuffer(buffer, audio->samples * 4);
        aiMessage = NULL;
        osRecvMesg(&audio->aiQueue, &aiMessage, OS_MESG_BLOCK);
    } while (audio->stop == 0);
    osSendMesg(&audio->stopQueue, NULL, OS_MESG_BLOCK);
    osStopThread(NULL);
}

/// Provide an always-zero runtime result.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800499C8 (0x800499C8).
s32 runtimeReturnZero(void) {
    return 0;
}

#include "byte_strings.h"
/// Check whether two byte regions have identical contents.
/// \param first First region to compare.
/// \param second Second region to compare.
/// \param count Nonnegative number of bytes to compare.
/// \returns Zero when the regions match, or a nonzero value otherwise.
/// \note Original decompilation identifier: \c func_800499D0 (0x800499D0).
s32 memoryCompareBytes(u8 *first, u8 *second, s32 count) {
    return bcmp(first, second, count);
}

/* memcpy and its twin are bcopy with the operands swapped; one line each,
 * or 5.3 schedules the a0 move after the ra spill */
// clang-format off
/// Copy bytes into a destination region.
/// \param dst Writable destination region.
/// \param src Source region.
/// \param n Number of bytes to copy.
/// \returns The destination pointer.
/// \note Original decompilation identifier: \c memcpy (0x800499F0).
void *memcpy(void *dst, const void *src, size_t n) { bcopy(src, dst, n); return dst; }

/// Copy bytes into a destination region.
/// \param dst Writable destination region.
/// \param src Source region.
/// \param n Number of bytes to copy.
/// \returns The destination pointer.
/// \note Original decompilation identifier: \c func_80049A1C (0x80049A1C).
void *memoryCopyBytes(void *dst, const void *src, size_t n) { bcopy(src, dst, n); return dst; }
// clang-format on

/// Fill a byte region with a repeated value.
/// \param destination Destination region, or NULL for no change.
/// \param value Byte value to store.
/// \param count Number of bytes to fill; nonpositive values make no change.
/// \returns The destination pointer.
/// \note Original decompilation identifier: \c func_80049A48 (0x80049A48).
u8 *memorySetBytes(u8 *destination, u8 value, s32 count) {
    memoryFillBytes(destination, count, value);
    return destination;
}

/// Copy a null-terminated string, including its terminator.
/// \param dst Destination with room for the source string and terminator.
/// \param src Null-terminated source string.
/// \returns The original destination pointer.
/// \note Original decompilation identifier: \c func_80049A78 (0x80049A78).
u8 *stringCopy(u8 *dst, const u8 *src) {
    u8 *d = dst;

    while ((*dst++ = *src++) != 0) {
    }
    return d;
}

/// Measure a null-terminated string.
/// \param s String to measure, or NULL.
/// \returns The number of bytes before the terminator, or zero for NULL.
/// \note Original decompilation identifier: \c strlen (0x80049AAC).
u32 strlen(const u8 *s) {
    s32 n;

    if (s == NULL) {
        return 0;
    }
    n = 0;
    while (*s++) {
        n++;
    }
    return n;
}

/// Fill a byte region with a repeated value.
/// \param p Destination region, or NULL for no change.
/// \param n Number of bytes to fill; nonpositive values make no change.
/// \param c Byte value to store.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80049AF0 (0x80049AF0).
void memoryFillBytes(u8 *p, s32 n, u8 c) {
    u8 *q;

    if (p != NULL) {
        q = p;
        while (n > 0) {
            n--;
            *q++ = c;
        }
    }
}

/// Locate the first character after leading spaces and tabs.
/// \param s String to inspect, or NULL.
/// \returns A pointer to the first non-space, non-tab character, or to the
/// terminator when none remains; NULL for NULL input.
/// \note Original decompilation identifier: \c func_80049B20 (0x80049B20).
u8 *stringSkipHorizontalSpace(u8 *s) {
    if (s == NULL) {
        return NULL;
    }
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

/// Find the first occurrence of a byte in a null-terminated string.
/// \param s String to search, or NULL.
/// \param c Byte to locate; zero selects the terminator.
/// \returns The matching character address, or NULL when absent or when s is NULL.
/// \note Original decompilation identifier: \c strchr (0x80049B74).
u8 *strchr(u8 *s, u8 c) {
    u8 *p;

    if (s == NULL) {
        return NULL;
    }
    p = s;
    while (*p != c) {
        if (*p == 0) {
            return NULL;
        }
        p++;
    }
    return p;
}

/// Convert a packed JIS character code to the font character encoding.
/// \param code Packed JIS row and cell bytes; high bits are ignored.
/// \returns The packed Shift-JIS character code.
/// \note Original decompilation identifier: \c func_80049BD0 (0x80049BD0).
s32 fontConvertJisToShiftJis(s32 code) {
    u8 row;
    u8 cell;
    code &= 0x7F7F;
    cell = code & 0xFF;
    row = (code & 0xFF00) >> 8;
    row += 0x21;
    if (row % 2 == 1) {
        cell += 0x5E;
    }
    if (cell >= 0x21 && cell < 0x60) {
        cell += 0x1F;
    } else {
        cell += 0x20;
    }
    row = (s8)row / 2;
    row ^= 0x20;
    row |= 0x80;
    return row << 8 | cell;
}
