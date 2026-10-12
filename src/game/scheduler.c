/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "scheduler_types.h"

/// Whether the scheduler has received the pre-NMI reset notification.
/// \note Original identifier: \c D_8009D524 (0x8009D524).
extern volatile s32 gSchedulerResetPending;
/// Canonical SDK video-interface mode table indexed by runtime and scheduler mode selection.
/// \note Original identifier: \c D_8009DCB0 (0x8009DCB0).
extern OSViMode osViModeTable[];
/// Controller port most recently selected for voice initialization or probing.
/// \note Original identifier: \c D_800F8AC0 (0x800F8AC0).
/// \note This address also marks the graphics thread stack top.
/// Audio thread stack storage; its base is also the retrace thread stack top.
/// \note Original identifier: \c D_800F4AC0 (0x800F4AC0).
/// Graphics thread stack storage; its base is also the audio thread stack top.
/// \note Original identifier: \c D_800F6AC0 (0x800F6AC0).
extern u8 gSchedulerAudioStack[], gSchedulerGraphicsStack[], gVoiceRecognitionDevicePort[];
void schedulerRetraceThread(u8 *);
void schedulerAudioThread(u8 *);
void schedulerGraphicsThread(u8 *);

/// Initialize and start audio, graphics, and display-event scheduling.
/// \param s Scheduler state to initialize; must remain valid while its threads run.
/// \param id Base identifier for scheduler threads.
/// \param priority Base scheduling priority for scheduler threads.
/// \param mode Valid index of the video mode to use.
/// \param retraces Number of video retraces between client notifications.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FC30 (0x8003FC30).
void schedulerInitialize(GameScheduler *s, s32 id, s32 priority, u8 mode, u8 retraces) {
    s->retraceCount = retraces;
    gSchedulerResetPending = 0;
    s->graphicsTask = NULL;
    s->audioTask = NULL;
    s->waitingGraphicsTask = NULL;
    s->clients = NULL;
    s->startupBlack = 1;
    s->frameCount = 0;
    s->retraceMessage = 1;
    s->preNmiMessage = 3;
    osCreateMesgQueue(&s->retrace.queue, s->retrace.messages, 8);
    osCreateMesgQueue(&s->rsp.queue, s->rsp.messages, 8);
    osCreateMesgQueue(&s->rdp.queue, s->rdp.messages, 8);
    osCreateMesgQueue(&s->graphicsCommand.queue, s->graphicsCommand.messages, 8);
    osCreateMesgQueue(&s->audioCommand.queue, s->audioCommand.messages, 8);
    osCreateMesgQueue(&s->reply.queue, s->reply.messages, 8);
    osCreateViManager(OS_PRIORITY_VIMGR);
    osViSetMode(&osViModeTable[mode]);
    osViBlack(TRUE);
    osViSetEvent(&s->retrace.queue, (OSMesg)0x29A, retraces);
    osSetEventMesg(OS_EVENT_SP, &s->rsp.queue, (OSMesg)0x29B);
    osSetEventMesg(OS_EVENT_DP, &s->rdp.queue, (OSMesg)0x29C);
    osSetEventMesg(OS_EVENT_PRENMI, &s->retrace.queue, (OSMesg)0x29D);
    osCreateThread(&s->retraceThread, id + 2, (void (*)(void *))schedulerRetraceThread, s, gSchedulerAudioStack,
                   priority + 10);
    osStartThread(&s->retraceThread);
    osCreateThread(&s->audioThread, id + 1, (void (*)(void *))schedulerAudioThread, s, gSchedulerGraphicsStack,
                   priority + 5);
    osStartThread(&s->audioThread);
    osCreateThread(&s->graphicsThread, id, (void (*)(void *))schedulerGraphicsThread, s, gVoiceRecognitionDevicePort, priority);
    osStartThread(&s->graphicsThread);
}

/// Get the queue used to submit audio tasks.
/// \param schedulerAddress RAM address of initialized scheduler state.
/// \returns Borrowed RAM address of the scheduler's audio command queue.
/// \note Original decompilation identifier: \c func_8003FE34 (0x8003FE34).
s32 schedulerGetAudioCommandQueue(s32 schedulerAddress) {
    return schedulerAddress + 4;
}

/// Get the queue used to submit graphics tasks.
/// \param schedulerAddress RAM address of initialized scheduler state.
/// \returns Borrowed RAM address of the scheduler's graphics command queue.
/// \note Original decompilation identifier: \c func_8003FE3C (0x8003FE3C).
s32 schedulerGetGraphicsCommandQueue(s32 schedulerAddress) {
    return schedulerAddress + 0x3C;
}

/// Count of messages consumed by the display-event thread, including pre-NMI notifications.
/// \note Original identifier: \c D_8009D520 (0x8009D520).
extern s32 gSchedulerDisplayEventCount;
void schedulerNotifyClients(u8 *, u8 *);

/// Process display retraces and notify clients of display and reset events.
/// \param scheduler Scheduler state owned by this thread.
/// \returns Does not return.
///
/// \note NONPORTABLE: for (;;) leaves IDO's dead epilogue in place
/// \note Original decompilation identifier: \c func_8003FE44 (0x8003FE44).
void schedulerRetraceThread(u8 *scheduler) {
    OSMesg msg = NULL;

    for (;;) {
        osRecvMesg((OSMesgQueue *)(scheduler + 0x74), &msg, OS_MESG_BLOCK);
        gSchedulerDisplayEventCount++;
        switch ((s32)msg) {
        case 0x29A:
            schedulerNotifyClients(scheduler, scheduler);
            break;
        case 0x29D:
            gSchedulerResetPending = 1;
            schedulerNotifyClients(scheduler, scheduler + 2);
            break;
        }
    }
}

/// Register a client to receive scheduler event notifications.
/// \param scheduler Scheduler that will notify the client.
/// \param client Writable client registration, kept valid until it is removed.
/// \param queueAddress RAM address of an initialized client message queue,
/// kept valid until the registration is removed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FF3C (0x8003FF3C).
void schedulerAddClient(u8 *scheduler, u8 *client, s32 queueAddress) {
    u32 previousInterruptMask;

    previousInterruptMask = osSetIntMask(1U);
    ((SchedulerClient *)client)->queue = (OSMesgQueue *)queueAddress;
    ((SchedulerClient *)client)->next = ((GameScheduler *)scheduler)->clients;
    ((GameScheduler *)scheduler)->clients = (SchedulerClient *)client;
    osSetIntMask(previousInterruptMask);
}

/// Stop sending scheduler events to a registered client.
/// \param scheduler Scheduler holding the registration.
/// \param client Client registration to remove.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FF8C (0x8003FF8C).
void schedulerRemoveClient(u8 *scheduler, void **client) {
    void **p = *(void ***)(scheduler + 0x668);
    void **prev = NULL;
    u32 mask;

    mask = osSetIntMask(1);
    while (p != NULL) {
        if (p == client) {
            if (prev != NULL) {
                *prev = *client;
            } else {
                *(void **)(scheduler + 0x668) = *client;
            }
            break;
        }
        prev = p;
        p = (void **)*p;
    }
    osSetIntMask(mask);
}

/// Send an event notification to all registered scheduler clients.
/// \param scheduler Scheduler holding the client registrations.
/// \param message Event message to deliver; must remain valid until recipients
/// finish using it. Delivery is skipped for a full client queue.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80040014 (0x80040014).
void schedulerNotifyClients(u8 *scheduler, u8 *message) {
    u8 *var_s0;

    var_s0 = (u8 *)((GameScheduler *)scheduler)->clients;
    if (var_s0 != NULL) {
        do {
            osSendMesg(((SchedulerClient *)var_s0)->queue, message, 0);
            var_s0 = (u8 *)((SchedulerClient *)var_s0)->next;
        } while (var_s0 != NULL);
    }
}

/// Run submitted audio tasks and report their completion.
/// \param scheduler Scheduler state owned by this thread.
/// \returns Does not return.
///
/// \note NONPORTABLE: the running task is an s32; the loop never exits.
/// \note Original decompilation identifier: \c func_80040064 (0x80040064).
void schedulerAudioThread(u8 *scheduler) {
    u8 *sp5C;
    u8 *sp58;
    OSMesgQueue *q;
    OSTask *t;
    s32 yt;
    u32 state;

    sp5C = NULL;
    sp58 = NULL;
    state = 0;
    for (;;) {
        osRecvMesg((OSMesgQueue *)(scheduler + 4), (OSMesg *)&sp58, OS_MESG_BLOCK);
        osWritebackDCache(*(u8 **)(sp58 + 0x40), *(s32 *)(sp58 + 0x44));
        yt = *(s32 *)(scheduler + 0x66C);
        if (yt != 0) {
            osSpTaskYield();
            osRecvMesg((OSMesgQueue *)(scheduler + 0xAC), (OSMesg *)&sp5C, OS_MESG_BLOCK);
            if (osSpTaskYielded((OSTask *)(yt + 0x10)) != 0) {
                state = 1;
            } else {
                state = 2;
            }
        }
        q = (OSMesgQueue *)(scheduler + 0xAC);
        *(u8 **)(scheduler + 0x670) = sp58;
        osSpTaskLoad((OSTask *)(sp58 + 0x10));
        osSpTaskStartGo((OSTask *)(sp58 + 0x10));
        osRecvMesg(q, (OSMesg *)&sp5C, OS_MESG_BLOCK);
        *(u8 **)(scheduler + 0x670) = NULL;
        if (*(s32 *)(scheduler + 0x674) != 0) {
            osSendMesg((OSMesgQueue *)(scheduler + 0x11C), (OSMesg)&sp5C, OS_MESG_BLOCK);
        }
        if (state == 1) {
            t = (OSTask *)(yt + 0x10);
            osSpTaskLoad(t);
            osSpTaskStartGo(t);
        } else if (state == 2) {
            osSendMesg(q, (OSMesg)&sp5C, OS_MESG_BLOCK);
        }
        osSendMesg(*(OSMesgQueue **)(sp58 + 0x50), *(OSMesg *)(sp58 + 0x54), OS_MESG_BLOCK);
        state = 0;
    }
}

void schedulerWaitForFramebuffer(u8 *, u8 *);

/// Run submitted graphics tasks, present completed frames, and report completion.
/// \param scheduler Scheduler state owned by this thread.
/// \returns Does not return.
/// \note Original decompilation identifier: \c func_80040200 (0x80040200).
void schedulerGraphicsThread(u8 *scheduler) {
    u8 *sp44;
    u8 *sp40;

    sp44 = NULL;
    sp40 = NULL;
loop_1:
    osRecvMesg(scheduler + 0x3C, &sp40, 1);
    schedulerWaitForFramebuffer(scheduler, sp40);
    if (((GameScheduler *)scheduler)->audioTask != 0) {
        ((GameScheduler *)scheduler)->waitingGraphicsTask = sp40;
        osRecvMesg(scheduler + 0x11C, &sp44, 1);
        ((GameScheduler *)scheduler)->waitingGraphicsTask = NULL;
    }
    ((GameScheduler *)scheduler)->graphicsTask = sp40;
    osSpTaskLoad(sp40 + 0x10);
    osSpTaskStartGo(sp40 + 0x10);
    osRecvMesg(scheduler + 0xAC, &sp44, 1);
    ((GameScheduler *)scheduler)->graphicsTask = NULL;
    osRecvMesg(scheduler + 0xE4, &sp44, 1);
    if (((SchedulerTask *)sp40)->flags & 0x40) {
        ((GameScheduler *)scheduler)->frameCount = (u32)(((GameScheduler *)scheduler)->frameCount + 1);
        osViSwapBuffer(((SchedulerTask *)sp40)->framebuffer);
        if ((((GameScheduler *)scheduler)->startupBlack != 0) &&
            ((u32)((GameScheduler *)scheduler)->frameCount >= 0x15U)) {
            osViBlack(0U);
            ((GameScheduler *)scheduler)->startupBlack = 0;
        }
    }
    osSendMesg(((SchedulerTask *)sp40)->reply, ((SchedulerTask *)sp40)->message, 1);
    goto loop_1;
}

/// Wait until a graphics task framebuffer is available for rendering.
/// \param scheduler Scheduler supplying display-event notifications.
/// \param task Graphics task whose framebuffer must be available.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80040374 (0x80040374).
void schedulerWaitForFramebuffer(u8 *scheduler, u8 *task) {
    OSMesg msg = NULL;
    void *link[2];
    void *buf = *(void **)(task + 0xC);

    while (osViGetCurrentFramebuffer() == buf || osViGetNextFramebuffer() == buf) {
        schedulerAddClient(scheduler, (u8 *)link, (s32)(scheduler + 0x11C));
        osRecvMesg((OSMesgQueue *)(scheduler + 0x11C), &msg, OS_MESG_BLOCK);
        schedulerRemoveClient(scheduler, link);
    }
}


/// Check whether a pre-reset notification has been received.
/// \returns Nonzero after a pre-reset notification, or zero otherwise.
/// \note Original decompilation identifier: \c func_8004042C (0x8004042C).
s32 schedulerIsResetPending(void) {
    return gSchedulerResetPending;
}
