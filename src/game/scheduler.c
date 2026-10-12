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
void schedulerRetraceThread(void *);
void schedulerAudioThread(void *);
void schedulerGraphicsThread(void *);

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
    osCreateThread(&s->retraceThread, id + 2, schedulerRetraceThread, s, gSchedulerAudioStack,
                   priority + 10);
    osStartThread(&s->retraceThread);
    osCreateThread(&s->audioThread, id + 1, schedulerAudioThread, s, gSchedulerGraphicsStack,
                   priority + 5);
    osStartThread(&s->audioThread);
    osCreateThread(&s->graphicsThread, id, schedulerGraphicsThread, s, gVoiceRecognitionDevicePort, priority);
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
void schedulerNotifyClients(GameScheduler *, OSMesg);

/// Process display retraces and notify clients of display and reset events.
/// \param scheduler Scheduler state owned by this thread.
/// \returns Does not return.
///
/// \note NONPORTABLE: for (;;) leaves IDO's dead epilogue in place
/// \note Original decompilation identifier: \c func_8003FE44 (0x8003FE44).
void schedulerRetraceThread(void *scheduler) {
    OSMesg msg = NULL;

    for (;;) {
        osRecvMesg((OSMesgQueue *)((u8 *)scheduler + 0x74), &msg, OS_MESG_BLOCK);
        gSchedulerDisplayEventCount++;
        switch ((s32)msg) {
        case 0x29A:
            schedulerNotifyClients(scheduler, scheduler);
            break;
        case 0x29D:
            gSchedulerResetPending = 1;
            schedulerNotifyClients(scheduler, (u8 *)scheduler + 2);
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
void schedulerNotifyClients(GameScheduler *scheduler, OSMesg message) {
    SchedulerClient *client;

    client = scheduler->clients;
    if (client != NULL) {
        do {
            osSendMesg(client->queue, message, 0);
            client = client->next;
        } while (client != NULL);
    }
}

/// Run submitted audio tasks and report their completion.
/// \param scheduler Scheduler state owned by this thread.
/// \returns Does not return.
///
/// \note Original decompilation identifier: \c func_80040064 (0x80040064).
void schedulerAudioThread(void *scheduler) {
    OSMesg completionMessage;
    OSMesg taskMessage;
    OSMesgQueue *q;
    OSTask *t;
    SchedulerTask *graphicsTask;
    u32 state;

    completionMessage = NULL;
    taskMessage = NULL;
    state = 0;
    for (;;) {
        osRecvMesg(&((GameScheduler *)scheduler)->audioCommand.queue, &taskMessage, OS_MESG_BLOCK);
        osWritebackDCache(((SchedulerTask *)taskMessage)->task.t.data_ptr, ((SchedulerTask *)taskMessage)->task.t.data_size);
        graphicsTask = ((GameScheduler *)scheduler)->graphicsTask;
        if (graphicsTask != 0) {
            osSpTaskYield();
            osRecvMesg(&((GameScheduler *)scheduler)->rsp.queue, &completionMessage, OS_MESG_BLOCK);
            if (osSpTaskYielded(&graphicsTask->task) != 0) {
                state = 1;
            } else {
                state = 2;
            }
        }
        q = &((GameScheduler *)scheduler)->rsp.queue;
        ((GameScheduler *)scheduler)->audioTask = taskMessage;
        osSpTaskLoad(&((SchedulerTask *)taskMessage)->task);
        osSpTaskStartGo(&((SchedulerTask *)taskMessage)->task);
        osRecvMesg(q, &completionMessage, OS_MESG_BLOCK);
        ((GameScheduler *)scheduler)->audioTask = NULL;
        if (((GameScheduler *)scheduler)->waitingGraphicsTask != 0) {
            osSendMesg(&((GameScheduler *)scheduler)->reply.queue, (OSMesg)&completionMessage, OS_MESG_BLOCK);
        }
        if (state == 1) {
            t = &graphicsTask->task;
            osSpTaskLoad(t);
            osSpTaskStartGo(t);
        } else if (state == 2) {
            osSendMesg(q, (OSMesg)&completionMessage, OS_MESG_BLOCK);
        }
        osSendMesg(((SchedulerTask *)taskMessage)->reply, ((SchedulerTask *)taskMessage)->message, OS_MESG_BLOCK);
        state = 0;
    }
}

void schedulerWaitForFramebuffer(GameScheduler *, SchedulerTask *);

/// Run submitted graphics tasks, present completed frames, and report completion.
/// \param scheduler Scheduler state owned by this thread.
/// \returns Does not return.
/// \note Original decompilation identifier: \c func_80040200 (0x80040200).
void schedulerGraphicsThread(void *scheduler) {
    OSMesg completionMessage;
    OSMesg taskMessage;

    completionMessage = NULL;
    taskMessage = NULL;
loop_1:
    osRecvMesg(&((GameScheduler *)scheduler)->graphicsCommand.queue, &taskMessage, 1);
    schedulerWaitForFramebuffer(scheduler, taskMessage);
    if (((GameScheduler *)scheduler)->audioTask != 0) {
        ((GameScheduler *)scheduler)->waitingGraphicsTask = taskMessage;
        osRecvMesg(&((GameScheduler *)scheduler)->reply.queue, &completionMessage, 1);
        ((GameScheduler *)scheduler)->waitingGraphicsTask = NULL;
    }
    ((GameScheduler *)scheduler)->graphicsTask = taskMessage;
    osSpTaskLoad(&((SchedulerTask *)taskMessage)->task);
    osSpTaskStartGo(&((SchedulerTask *)taskMessage)->task);
    osRecvMesg(&((GameScheduler *)scheduler)->rsp.queue, &completionMessage, 1);
    ((GameScheduler *)scheduler)->graphicsTask = NULL;
    osRecvMesg(&((GameScheduler *)scheduler)->rdp.queue, &completionMessage, 1);
    if (((SchedulerTask *)taskMessage)->flags & 0x40) {
        ((GameScheduler *)scheduler)->frameCount = (u32)(((GameScheduler *)scheduler)->frameCount + 1);
        osViSwapBuffer(((SchedulerTask *)taskMessage)->framebuffer);
        if ((((GameScheduler *)scheduler)->startupBlack != 0) &&
            ((u32)((GameScheduler *)scheduler)->frameCount >= 0x15U)) {
            osViBlack(0U);
            ((GameScheduler *)scheduler)->startupBlack = 0;
        }
    }
    osSendMesg(((SchedulerTask *)taskMessage)->reply, ((SchedulerTask *)taskMessage)->message, 1);
    goto loop_1;
}

/// Wait until a graphics task framebuffer is available for rendering.
/// \param scheduler Scheduler supplying display-event notifications.
/// \param task Graphics task whose framebuffer must be available.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80040374 (0x80040374).
void schedulerWaitForFramebuffer(GameScheduler *scheduler, SchedulerTask *task) {
    OSMesg msg = NULL;
    void *link[2];
    void *buf = task->framebuffer;

    while (osViGetCurrentFramebuffer() == buf || osViGetNextFramebuffer() == buf) {
        schedulerAddClient((u8 *)scheduler, (u8 *)link, (s32)&scheduler->reply.queue);
        osRecvMesg(&scheduler->reply.queue, &msg, OS_MESG_BLOCK);
        schedulerRemoveClient((u8 *)scheduler, link);
    }
}


/// Check whether a pre-reset notification has been received.
/// \returns Nonzero after a pre-reset notification, or zero otherwise.
/// \note Original decompilation identifier: \c func_8004042C (0x8004042C).
s32 schedulerIsResetPending(void) {
    return gSchedulerResetPending;
}
