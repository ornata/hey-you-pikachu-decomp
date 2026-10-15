/// \file
/// Defines game scheduler registrations, task records and retained worker state.
#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H
#include "game_message.h"
#include <ultra64.h>

/// Borrowed event registration linked into one scheduler.
typedef struct SchedulerClient {
    struct SchedulerClient *next; ///< Next registered client, or null.
    OSMesgQueue *queue;           ///< Borrowed queue receiving retrace and reset events.
} SchedulerClient;

/// Scheduler-owned task options, separate from the nested SDK OSTask flags.
enum { SCHEDULER_TASK_PRESENT_FRAMEBUFFER = 0x40 };

/// Task submission shared by graphics producers and scheduler workers.
/// The submitter retains the record and its borrowed resources until completion.
typedef struct SchedulerTask {
    struct SchedulerTask *next; ///< Next task in a submitter's pool.
    u32 state;                  ///< Submitter's task-availability state.
    u32 flags;                  ///< Scheduling flags; SCHEDULER_TASK_PRESENT_FRAMEBUFFER requests presentation.
    void *framebuffer;          ///< Borrowed target framebuffer.
    OSTask task;                ///< SDK task descriptor retained through RSP completion.
    OSMesgQueue *reply;         ///< Borrowed completion queue.
    OSMesg message;             ///< Completion message delivered to the reply queue.
} SchedulerTask;

/// Message slots retained by each scheduler-owned queue.
enum { SCHEDULER_QUEUE_CAPACITY = 8 };

/// An event queue with storage for eight scheduler messages.
typedef struct SchedulerQueue {
    OSMesgQueue queue;                         ///< Event queue retained for the scheduler lifetime.
    OSMesg messages[SCHEDULER_QUEUE_CAPACITY]; ///< Owned message slots for this queue.
} SchedulerQueue;

/// Retained queues, worker threads and task ownership for the game scheduler.
/// The scheduler and all borrowed client/task records must outlive the workers.
typedef struct GameScheduler {
    u16 retraceMessage;                 ///< Client notification value for a display retrace.
    u16 preNmiMessage;                  ///< Client notification value for impending reset.
    SchedulerQueue audioCommand;        ///< Queue accepting submitted audio tasks.
    SchedulerQueue graphicsCommand;     ///< Queue accepting submitted graphics tasks.
    SchedulerQueue retrace;             ///< Queue receiving display and reset events.
    SchedulerQueue rsp;                 ///< Queue receiving RSP completion events.
    SchedulerQueue rdp;                 ///< Queue receiving RDP completion events.
    SchedulerQueue reply;               ///< Queue used while waiting for framebuffer availability.
    OSThread retraceThread;             ///< Worker distributing display and reset notifications.
    OSThread audioThread;               ///< Worker servicing audio submissions.
    OSThread graphicsThread;            ///< Worker servicing graphics submissions.
    SchedulerClient *clients;           ///< Borrowed head of the event-client registrations.
    SchedulerTask *graphicsTask;        ///< Borrowed graphics task currently using the RSP.
    SchedulerTask *audioTask;           ///< Borrowed audio task currently using the RSP.
    SchedulerTask *waitingGraphicsTask; ///< Borrowed graphics task waiting for RSP availability.
    s32 startupBlack;                   ///< Nonzero while startup display blanking remains enabled.
    u32 frameCount;                     ///< Number of completed frame presentations.
    s32 retraceCount;                   ///< Display retraces between client notifications.
} GameScheduler;

#define SCHEDULER_OFFSET_CHECK(name, type, member, offset)                                                             \
    typedef char name[(((u32) & ((type *)0)->member) == (offset)) ? 1 : -1]
typedef char SchedulerClientSizeCheck[(sizeof(SchedulerClient) == 8) ? 1 : -1];
typedef char SchedulerTaskSizeCheck[(sizeof(SchedulerTask) == 0x58) ? 1 : -1];
SCHEDULER_OFFSET_CHECK(SchedulerTaskSdkOffsetCheck, SchedulerTask, task, 0x10);
SCHEDULER_OFFSET_CHECK(SchedulerTaskReplyOffsetCheck, SchedulerTask, reply, 0x50);
SCHEDULER_OFFSET_CHECK(SchedulerAudioQueueOffsetCheck, GameScheduler, audioCommand.queue, 4);
SCHEDULER_OFFSET_CHECK(SchedulerGraphicsQueueOffsetCheck, GameScheduler, graphicsCommand.queue, 0x3C);
SCHEDULER_OFFSET_CHECK(SchedulerRetraceQueueOffsetCheck, GameScheduler, retrace.queue, 0x74);
SCHEDULER_OFFSET_CHECK(SchedulerRspQueueOffsetCheck, GameScheduler, rsp.queue, 0xAC);
SCHEDULER_OFFSET_CHECK(SchedulerRdpQueueOffsetCheck, GameScheduler, rdp.queue, 0xE4);
SCHEDULER_OFFSET_CHECK(SchedulerReplyQueueOffsetCheck, GameScheduler, reply.queue, 0x11C);

SCHEDULER_OFFSET_CHECK(SchedulerClientsOffsetCheck, GameScheduler, clients, 0x668);
SCHEDULER_OFFSET_CHECK(SchedulerGraphicsTaskOffsetCheck, GameScheduler, graphicsTask, 0x66C);
SCHEDULER_OFFSET_CHECK(SchedulerFrameCountOffsetCheck, GameScheduler, frameCount, 0x67C);
#undef SCHEDULER_OFFSET_CHECK
#endif
