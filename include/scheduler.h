/// \file
/// Declares task submission queues and event registrations owned by the game scheduler.

#ifndef GAME_SCHEDULER_H
#define GAME_SCHEDULER_H

#include "scheduler_types.h"

/// Initialize and start audio, graphics and display-event scheduling.
/// \param scheduler Writable scheduler state that remains valid while its threads run.
/// \param id Base identifier for the scheduler threads.
/// \param priority Base scheduling priority for the scheduler threads.
/// \param mode Valid video-mode table index.
/// \param retraces Number of video retraces between client notifications.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FC30 (0x8003FC30).
void schedulerInitialize(GameScheduler *scheduler, s32 id, s32 priority, u8 mode, u8 retraces);

/// Get the queue used to submit audio tasks.
/// \param scheduler Initialized scheduler whose queue is borrowed.
/// \returns Borrowed audio command queue, valid for the scheduler lifetime; submitted tasks and their resources must remain valid until completion.
/// \note Original decompilation identifier: \c func_8003FE34 (0x8003FE34).
OSMesgQueue *schedulerGetAudioCommandQueue(GameScheduler *scheduler);

/// Get the queue used to submit graphics tasks.
/// \param scheduler Initialized scheduler whose queue is borrowed.
/// \returns Borrowed graphics command queue, valid for the scheduler lifetime; submitted tasks and their resources must remain valid until completion.
/// \note Original decompilation identifier: \c func_8003FE3C (0x8003FE3C).
OSMesgQueue *schedulerGetGraphicsCommandQueue(GameScheduler *scheduler);

/// Register a client to receive scheduler event notifications.
/// \param scheduler Initialized scheduler that will notify the client.
/// \param client Writable registration not already linked to a scheduler; retained until removed.
/// \param queue Initialized destination queue that remains valid until the registration is removed and pending messages are consumed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FF3C (0x8003FF3C).
void schedulerAddClient(GameScheduler *scheduler, SchedulerClient *client, OSMesgQueue *queue);

/// Stop sending scheduler events to a client.
/// \param scheduler Initialized scheduler holding the registration.
/// \param client Registration to remove; an unregistered client has no effect. Already queued notifications remain pending.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FF8C (0x8003FF8C).
void schedulerRemoveClient(GameScheduler *scheduler, SchedulerClient *client);

/// Check whether a pre-reset notification has been received.
/// \returns Nonzero after a pre-reset notification, or zero otherwise.
/// \note Original decompilation identifier: \c func_8004042C (0x8004042C).
s32 schedulerIsResetPending(void);

#endif
