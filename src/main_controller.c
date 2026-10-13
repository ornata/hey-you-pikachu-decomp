#include "microphone_session.h"
#include "controller_accessory.h"
#include "controller_service.h"
#include "render_state.h"
#include "game_heap.h"
#include "serial_queue.h"
#include "font_cache.h"
#include "texture_cache.h"
#include "rom_dma.h"
#include "matrix_basic.h"
#include "camera_storage.h"
#include "object_groups.h"
#include "math_angles.h"
#include "controller_pad.h"
#include "controller_rumble.h"
#include "microphone_configuration.h"
#include "controller_commands.h"
#include "text_window.h"
#include "message_bar.h"
#include "matrix_point.h"
#include "matrix_translation.h"
#include "matrix_orientation.h"
#include "actor_script_time.h"
#include "camera_overlay.h"
#include "scheduler.h"
#include "heap_tail.h"
/* Controller startup thread and event handlers, ROM 0x3250..0x3FD0.
 * Separate original object: pass-1 startup strings precede the event jump table.
 * Compiler pin: Makefile IDO 5.3, -O2 -mips2. */
#include <ultra64.h>
#include "common.h"
#include "eeprom_io.h"
#include "runtime_program.h"
#include "heap_alloc.h"

/* Idle thread control block + stack live in BSS (see pikachu.ld /
 * undefined_syms_auto.txt; not yet attributed to a C file). */
extern OSThread gIdleThread; /* 0x800A9688 */
/// Main scheduler context; its address also marks the boot-entry stack top.
/// \note Original identifier: \c D_800A9000 (0x800A9000).
extern u64 gMainSchedulerContext[];
/// One-past-end address of the downward-growing idle-thread stack.
/// \note Original identifier: \c D_800A9DE8 (0x800A9DE8).
extern u64 gIdleThreadStackTop[];

void IdleMain(void *arg);
void runtimeStartMainThread(void *stack);

#define IDLE_THREAD_ID 10
#define IDLE_THREAD_PRI 10



/// Rumble pattern lifecycle selectors used by the controller thread.
typedef enum ControllerPatternLifecycle {
    CONTROLLER_PATTERN_IDLE = 0,
    CONTROLLER_PATTERN_CANCEL = 1,
    CONTROLLER_PATTERN_RUNNING = 2,
    CONTROLLER_PATTERN_RESET = 3
} ControllerPatternLifecycle;

/// Rumble motor lifecycle selectors used by the controller thread.
typedef enum ControllerMotorLifecycle {
    CONTROLLER_MOTOR_IDLE = 0,
    CONTROLLER_MOTOR_WAIT_PULSE = 1,
    CONTROLLER_MOTOR_INITIALIZE = 2,
    CONTROLLER_MOTOR_START = 3,
    CONTROLLER_MOTOR_STOP = 4
} ControllerMotorLifecycle;

/// Per-port rumble pattern timing, owned by the controller thread.
typedef struct ControllerRumblePatternState {
    s32 state; ///< ControllerPatternLifecycle selector.
    u64 deadlineTicks; ///< Time when the complete pattern ends.
    u64 nextPulseTicks; ///< Time when the next pulse may begin.
    u64 intervalTicks; ///< Interval between pulse starts.
    u64 pulseTicks; ///< Duration of each motor pulse.
} ControllerRumblePatternState;

/// Per-port motor timing and accessory-initialization retry state.
typedef struct ControllerRumbleMotorState {
    s32 state; ///< ControllerMotorLifecycle selector.
    u64 deadlineTicks; ///< Pulse expiry or next accessory-initialization attempt, selected by state.
    s32 attempts; ///< Initialization failures or successful stop attempts, selected by state.
} ControllerRumbleMotorState;

typedef char ControllerRumblePatternSizeCheck[(sizeof(ControllerRumblePatternState) == 0x28) ? 1 : -1];
typedef char ControllerRumbleMotorSizeCheck[(sizeof(ControllerRumbleMotorState) == 0x18) ? 1 : -1];
typedef char ControllerRumbleDeadlineOffsetCheck[((u32)&((ControllerRumbleMotorState *)0)->deadlineTicks == 8) ? 1 : -1];
typedef char ControllerRumbleAttemptsOffsetCheck[((u32)&((ControllerRumbleMotorState *)0)->attempts == 0x10) ? 1 : -1];

/// Controller ports enabled by device initialization.
/// Original: D_800ACE80 (0x800ACE80).
u8 gControllerEnabledPortMask;
/// Enabled controller ports with a responding device.
/// Original: D_800ACE81 (0x800ACE81).
u8 gControllerConnectedPortMask;
/// Persistent cached device status for each controller port.
/// Original: D_800ACE88 (0x800ACE88).
OSContStatus gControllerPortStatus[4];
/// Persistent accessory context storage for each controller port.
/// Original: D_800ACE98 (0x800ACE98).
OSPfs gControllerAccessoryStorage[4];
/// Shared serial I/O completion queue owned by the controller subsystem.
/// Original: D_800AD038 (0x800AD038).
OSMesgQueue gSerialCompletionQueue;
/// Backing message slots for the serial I/O completion queue.
/// Original: D_800AD050 (0x800AD050).
OSMesg gSerialCompletionMessages[10];
/// Accessory lifecycle state for each controller port.
/// Original: D_800AD078 (0x800AD078).
s32 gControllerAccessoryState[4];
/// Controller thread queue for scheduler events and device commands.
/// Original: D_800AD088 (0x800AD088).
OSMesgQueue gControllerCommandQueue;
/// Backing message slots for the controller thread command queue.
/// Original: D_800AD0A0 (0x800AD0A0).
OSMesg gControllerCommandMessages[16];
/// Per-port rumble motor state, pulse deadline and initialization retry state.
/// \note Original identifier: \c D_800AD0E0 (0x800AD0E0).
ControllerRumbleMotorState gControllerRumbleMotorState[4];
/// Per-port rumble pattern state, end time, pulse interval and pulse duration.
/// \note Original identifier: \c D_800AD140 (0x800AD140).
ControllerRumblePatternState gControllerRumblePatternState[4];
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
void colorBlendToward(u8 *, u8 *, f32);
void colorStoreClampedRgba(s32, s32, s32, s32, u8 *);
/// Next reusable graphics-task slot for submission.
/// \note Original identifier: \c D_800F2988 (0x800F2988).
extern s32 gGraphicsNextTaskIndex;
/// Persistent per-task viewport storage; also marks the command-buffer pointer table end.
/// \note Original identifier: \c D_800ABDF0 (0x800ABDF0).
extern u8 gRenderTaskViewports[];
/// Persistent per-task projection and view matrix storage.
/// \note Original identifier: \c D_800ABEB0 (0x800ABEB0).
extern u8 gRenderTaskMatrices[];
/* The original caller passes an extra, unused display-list argument. */
void renderApplyAreaFog();
s32 cameraBuildViewProjection(s32, u8 *, u8 *);
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
/// Scale converting an overlay alpha byte to a blend weight.
/// \note Original identifier: \c D_8009FC90 (0x8009FC90).
extern f32 gOverlayAlphaWeightScale;
/* 3A058 copies the twelve words of an affine 3-by-4 matrix. */
typedef struct {
    s32 w[12];
} MatrixBlock00800;
/// Separate symbol view of the primary player record's view yaw.
/// \note Original identifier: \c D_800EF6AC (0x800EF6AC).
extern f32 gPlayerYaw;
/// Separate symbol view of the primary player record's additional view pitch.
/// \note Original identifier: \c D_800EF6B0 (0x800EF6B0).
extern f32 gPlayerPitchOffset;
/// Separate symbol view of the primary player record's view pitch.
/// \note Original identifier: \c D_800EF6B4 (0x800EF6B4).
extern f32 gPlayerPitch;
/// Separate symbol view of the primary player record's horizontal view shake.
/// \note Original identifier: \c D_800EF6B8 (0x800EF6B8).
extern f32 gPlayerViewShake;
void controllerThreadMain(void *);
/// Thread record for controller polling, voice requests and rumble servicing.
/// \note Original identifier: \c D_800AC4C0 (0x800AC4C0).
extern OSThread gControllerThread;
/// Controller scheduler-client registration; its address also marks the controller-thread stack top.
/// \note Original identifier: \c D_800ACE70 (0x800ACE70).
extern SchedulerClient gControllerSchedulerClient;
/// Primary camera additional overlay blend target RGBA color.
/// \note Original identifier: \c D_800ABDB8 (0x800ABDB8).
extern u8 gPrimaryOverlayBlendTarget[];
/// Remaining duration of the primary camera additional overlay blend.
/// \note Original identifier: \c D_800ABDC0 (0x800ABDC0).
extern f32 gPrimaryOverlayBlendRemainingSeconds;
/// Primary camera secondary overlay target RGBA color.
/// \note Original identifier: \c D_800ABDC4 (0x800ABDC4).
extern u8 gPrimarySecondaryOverlayTarget[];
/// Remaining duration of the primary camera secondary overlay transition.
/// \note Original identifier: \c D_800ABDCC (0x800ABDCC).
extern f32 gPrimarySecondaryOverlayRemainingSeconds;

typedef struct CameraDefaults {
    f32 position[3], target[3], up[3];
} CameraDefaults;
/// Default eye, target and up vectors copied into initialized cameras.
/// \note Original identifier: \c D_800633C0 (0x800633C0).
extern CameraDefaults gRenderCameraDefaults;
/// Cartridge ROM start address of the shared rendering asset segment.
/// \note Original identifier: \c D_4B4990 (0x4B4990).
/// Cartridge ROM end address of the shared rendering asset segment.
/// \note Original identifier: \c D_505090 (0x505090).
extern u8 gSharedRenderAssetsRomStart[], gSharedRenderAssetsRomEnd[];
/// Pair of owned command-buffer addresses indexed by framebuffer.
/// \note Original identifier: \c D_800ABDE8 (0x800ABDE8).
extern s32 gRenderCommandBuffers[];
/// Whether the elapsed-script tint needs its initial script-time origin.
/// \note Original identifier: \c D_800AC4B0 (0x800AC4B0).
/// Whether the primary-actor elapsed-script camera tint is enabled.
/// \note Original identifier: \c D_800AC4B4 (0x800AC4B4).
extern s32 gCameraElapsedTintNeedsOrigin, gCameraElapsedTintEnabled;
/// Initial elapsed time in seconds requested when enabling the camera tint.
/// \note Original identifier: \c D_800AC4B8 (0x800AC4B8).
extern f32 gCameraElapsedTintInitialSeconds;
void heapDebugNoOp();

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Whether the next display-list setup must emit the one-time initial render state.
/// \note Original identifier: \c D_800633E4 (0x800633E4).
extern s32 gRenderInitialStatePending;
extern Gfx D_637D8[], D_636B0[], D_63798[];

/// Upper near-zero cosine bound selecting the camera pitch fallback slope.
/// \note Original identifier: \c D_8009FC88 (0x8009FC88).
extern f32 gCameraPitchCosineUpperTolerance;
/// Lower near-zero cosine bound selecting the camera pitch fallback slope.
/// \note Original identifier: \c D_8009FC8C (0x8009FC8C).
extern f32 gCameraPitchCosineLowerTolerance;

/// First color framebuffer storage used for drawing and presentation.
/// \note Original identifier: \c D_803B5000 (0x803B5000).
extern u8 gColorFramebuffer0[];


/// Physical depth-buffer address used in graphics commands.
/// \note Original identifier: \c D_38F800 (0x38F800).
extern u8 gDepthBufferPhysicalAddress[];

/// Segmented shared display list drawing the camera overlay rectangle.
/// \note Original identifier: \c D_100C960 (0x100C960).
extern Gfx gCameraOverlayRectangleDisplayList[];

/// Whether the primary-actor elapsed-script camera tint is enabled.
/// \note Original identifier: \c D_800AC4B4 (0x800AC4B4).
extern s32 gCameraElapsedTintEnabled;

/// Upper bound of values stored as zero by the float tolerance helper.
/// \note Original identifier: \c D_8009FC94 (0x8009FC94).
extern f32 gFloatStoreZeroUpperTolerance;
/// Lower bound of values stored as zero by the float tolerance helper.
/// \note Original identifier: \c D_8009FC98 (0x8009FC98).
extern f32 gFloatStoreZeroLowerTolerance;


f32 viewportSetPixelRectangle(Vp *, s32, s32, s32, s32, s32, s32, f32);
/// Additional aspect scale applied when constructing the camera viewport projection.
/// \note Original identifier: \c D_8009FC9C (0x8009FC9C).
extern f32 gCameraProjectionAspectScale;

/// Primary-actor script-time origin used to calculate elapsed camera tint.
/// \note Original identifier: \c D_800633E8 (0x800633E8).
extern f32 gCameraElapsedTintOriginSeconds;
/// Whether the elapsed-script tint needs its initial script-time origin.
/// \note Original identifier: \c D_800AC4B0 (0x800AC4B0).
extern s32 gCameraElapsedTintNeedsOrigin;

extern Gfx D_100C7E0[];
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;
void hudDraw(Gfx **);
void hudDrawAreaLabel(Gfx **);
void presentationDrawCardScreen(Gfx **);
void presentationDrawCollectionScreen(Gfx **);
void eyeCatchDraw(Gfx **);
void runtimeDrawPromptPanel(void);
void toolboxDrawFrame(s32);
void wholeMapDraw(Gfx **);

/// Identifier of the currently active runtime program.
/// \note Original identifier: \c D_800AE4F0 (0x800AE4F0).
/// Number of startup frames whose full framebuffer has been cleared, capped at two.
/// \note Original identifier: \c D_800633EC (0x800633EC).
extern s32 gRenderInitialClearCount, gRuntimeCurrentProgram;
void renderPrepareDisplayList(void);

/// Whether final frame submission invokes the optional rendering hook.
/// \note Original identifier: \c D_80088F18 (0x80088F18).
extern s32 gRenderOptionalFrameHookEnabled;


/// Caller-owned startup record released after the worker acknowledges initialization.
/// The worker must not retain this record after setting ready.
typedef struct ControllerStartup {
    s32 ready; ///< Zero during startup; one after device services are initialized.
    s32 channelCount; ///< Number of enabled controller channels.
    s32 microphonePortMask; ///< Ports eligible to host the microphone.
} ControllerStartup;
typedef char ControllerStartupSizeCheck[(sizeof(ControllerStartup) == 0xC) ? 1 : -1];
void controllerInitializeDevices(s32);
void controllerInitializeRumbleState(void);
void controllerQueryDevices(void);
void controllerUpdateRumble(void);
/* Orphan strings of the compiled-out mpak error traces. */
static const char sNoMpakModuleMsg[] = "ERROR: no mpak module.\n";
static const char sUnknownMessageTypeMsg[] = "ERROR: unknown message type(%d)\n";

/// Run controller, microphone, EEPROM, and rumble services in response to scheduler and device messages.
/// \param argument Writable startup record containing a readiness flag, enabled controller-channel count, and eligible microphone-port mask; consumed before readiness is signaled.
/// \returns Never returns.
/// \note Original decompilation identifier: \c func_80002650 (0x80002650).
void controllerThreadMain(void *argument) {
    struct {
        ControllerStartup *args;
        OSMesg message;
    } frame;
    frame.message = NULL;
    frame.args = argument;
    controllerInitializeDevices(((ControllerStartup *)argument)->channelCount);
    controllerClearPadCache();
    micSessionInitialize(((ControllerStartup *)argument)->microphonePortMask);
    eepromInitializeTransfers();
    controllerInitializeRumbleState();
    frame.args->ready = 1;
    for (;;) {
        osRecvMesg((OSMesgQueue *)controllerGetCommandQueue(), &frame.message, OS_MESG_BLOCK);
        switch (*(s16 *)frame.message) {
        case GAME_MESSAGE_RETRACE:
            controllerQueryDevices();
            controllerPollPads();
            eepromUpdateTransfer();
            micSessionUpdate();
            controllerUpdateRumble();
            break;
        case GAME_MESSAGE_EEPROM:
            eepromDispatchRequest(frame.message);
            break;
        case GAME_MESSAGE_MICROPHONE:
            micSessionDispatchRequest(frame.message);
            break;
        case GAME_MESSAGE_RUMBLE:
            controllerDispatchRumbleRequest(frame.message);
            break;
        case GAME_MESSAGE_FRAME_COMPLETE:
        case GAME_MESSAGE_PRE_NMI:
        case GAME_MESSAGE_GRAPHICS_COMPLETE:
        case 8:
            break;
        }
    }
}

/// Start the controller service thread and wait until device initialization completes.
/// \param scheduler Initialized scheduler retained for frame events.
/// \param channelCount Number of controller channels to enable, from one through four.
/// \param microphonePortMask Bit mask of controller ports eligible to host the microphone.
/// \returns Nothing after device initialization completes.
/// \note Original decompilation identifier: \c func_80002764 (0x80002764).
void controllerStartThread(GameScheduler *scheduler, s32 channelCount, s32 microphonePortMask) {
    ControllerStartup args;

    schedulerAddClient(scheduler, &gControllerSchedulerClient, controllerGetCommandQueue());
    args.ready = 0;
    args.channelCount = channelCount;
    args.microphonePortMask = microphonePortMask;
    osCreateThread(&gControllerThread, 0x28, controllerThreadMain, &args, &gControllerSchedulerClient, 0x3C);
    osStartThread(&gControllerThread);
    while (args.ready == 0) {
        osYieldThread();
    }
}

/// Get the cached device status for a controller port.
/// \param port Controller port index from zero through three.
/// \returns Borrowed pointer to the port’s persistent cached status.
OSContStatus *controllerGetPortStatus(s32 port) {
    return &gControllerPortStatus[port];
}

/// Read a controller port’s accessory lifecycle state.
/// \param port Controller port index from zero through three.
/// \returns The recorded accessory state: zero for unavailable, two for initialized, three for awaiting initialization, or four after initialization failure.
s32 controllerGetAccessoryState(s32 port) {
    return gControllerAccessoryState[port];
}

/// Record a controller port’s accessory lifecycle state.
/// \param port Controller port index from zero through three.
/// \param state Accessory lifecycle state to record; no validation is performed.
/// \returns Nothing.
void controllerSetAccessoryState(s32 port, s32 state) {
    gControllerAccessoryState[port] = state;
}

/// Get the persistent accessory context for a controller port.
/// \param port Controller port index from zero through three.
/// \returns Borrowed storage suitable for the port’s accessory context.
OSPfs *controllerGetAccessoryStorage(s32 port) {
    return &gControllerAccessoryStorage[port];
}

/// Get the shared serial I/O completion queue.
/// \returns Borrowed pointer to the persistent queue, usable after controller initialization.
OSMesgQueue *serialGetMessageQueue(void) {
    return &gSerialCompletionQueue;
}

/// Get the controller thread’s queue for scheduler events and device commands.
/// \returns Borrowed pointer to the persistent controller message queue.
OSMesgQueue *controllerGetCommandQueue(void) {
    return &gControllerCommandQueue;
}

/// Refresh connected-port availability and accessory lifecycle state from cached device status.
/// \returns Nothing.
void controllerRefreshConnectionState(void) {
    s32 i;
    u8 mask;

    for (gControllerConnectedPortMask = 0, i = 0, mask = 1; i < 4; i++) {
        if (gControllerPortStatus[i].errno == 0) {
            gControllerConnectedPortMask |= mask;
        }
        if (gControllerPortStatus[i].errno != 0 || !(gControllerPortStatus[i].type & 4) || gControllerPortStatus[i].status != 1) {
            gControllerAccessoryState[i] = CONTROLLER_ACCESSORY_UNAVAILABLE;
        } else if (gControllerAccessoryState[i] == CONTROLLER_ACCESSORY_UNAVAILABLE) {
            gControllerAccessoryState[i] = CONTROLLER_ACCESSORY_PENDING;
        }
        mask <<= 1;
    }
    gControllerConnectedPortMask &= gControllerEnabledPortMask;
}

/// Initialize controller device communication and discover the enabled controller ports.
/// \param count Number of controller channels to enable, from one through four.
/// \returns Nothing.
void controllerInitializeDevices(s32 count) {
    s32 i;

    for (i = 0; i < 4; i++) {
        gControllerAccessoryState[i] = CONTROLLER_ACCESSORY_PENDING;
    }
    osCreateMesgQueue(&gControllerCommandQueue, gControllerCommandMessages, 16);
    osCreateMesgQueue(&gSerialCompletionQueue, gSerialCompletionMessages, 10);
    osSetEventMesg(OS_EVENT_SI, &gSerialCompletionQueue, (OSMesg)1);
    osContReset(&gSerialCompletionQueue, gControllerPortStatus);
    osContSetCh(count);
    gControllerEnabledPortMask = (1 << count) - 1;
    if (osContInit(&gSerialCompletionQueue, &gControllerConnectedPortMask, gControllerPortStatus)) {
    }
    gControllerConnectedPortMask &= gControllerEnabledPortMask;
    controllerRefreshConnectionState();
}

/// Query controller hardware and refresh cached connection and accessory status.
/// \returns Nothing; waits for serial I/O completion and requires initialized controller communication.
void controllerQueryDevices(void) {
    osContStartQuery(&gSerialCompletionQueue);
    osRecvMesg(&gSerialCompletionQueue, NULL, 1);
    osContGetQuery(gControllerPortStatus);
    controllerRefreshConnectionState();
}

/// Get the cached availability mask for enabled controller ports.
/// \returns A bit mask with bit zero through three identifying available ports.
u8 controllerGetConnectedPortMask(void) {
    return gControllerConnectedPortMask;
}

/// Check cached availability for a controller port.
/// \param port Controller port index; indices outside zero through three are unavailable.
/// \returns One when the enabled port is available, or zero otherwise.
s32 controllerIsPortConnected(s32 port) {
    if ((port < 0) || (port >= 4)) {
        return 0;
    }
    return (gControllerConnectedPortMask & (1 << port)) != 0;
}

/// Attempt to initialize a controller port’s Rumble Pak and update its accessory state.
/// \param port Controller port index from zero through three; requires initialized serial communication.
/// \returns One on successful initialization, or zero on failure.
s32 controllerInitializeRumblePak(s32 port) {
    s32 pad[2]; /* unused, sizes the frame */
    OSMesgQueue *sp24;

    sp24 = (OSMesgQueue *)serialGetMessageQueue();
    if (osMotorInit(sp24, controllerGetAccessoryStorage(port), port) == 0) {
        controllerSetAccessoryState(port, CONTROLLER_ACCESSORY_READY);
        return 1;
    }
    if (controllerGetAccessoryState(port) == CONTROLLER_ACCESSORY_READY) {
        controllerSetAccessoryState(port, CONTROLLER_ACCESSORY_PENDING);
    }
    return 0;
}

/// Reset a rumble control record for an immediate accessory initialization attempt.
/// \param control Writable rumble control record to reset.
/// \returns Nothing.
void controllerScheduleRumbleInitialization(ControllerRumbleMotorState *control) {
    control->state = CONTROLLER_MOTOR_INITIALIZE;
    control->attempts = 0;
    control->deadlineTicks = osGetTime();
}

/* Release-build debug print: the call is compiled out, but its format
 * strings stay in the boot rodata blob (ROM 0xA08F0..0xA0B30). uopt still
 * counts the dead blocks, which raises its callee-saved first-use cost. */
#define DEBUG_PRINTF(args)                                                                                             \
    do {                                                                                                               \
        if (0) {                                                                                                       \
            osSyncPrintf args;                                                                                         \
        }                                                                                                              \
    } while (0)
/// Diagnostic format for rumble initialization on an unsupported accessory.
/// \note Original identifier: \c D_8009FDE0 (0x8009FDE0).
extern char gControllerRumbleWrongAccessoryFormat[];
/// Diagnostic format for successful rumble accessory initialization.
/// \note Original identifier: \c D_8009FE00 (0x8009FE00).
extern char gControllerRumbleInitializedFormat[];
/// Diagnostic format for failed rumble accessory initialization.
/// \note Original identifier: \c D_8009FE20 (0x8009FE20).
extern char gControllerRumbleNotInitializedFormat[];
/// Diagnostic format for scheduling another rumble initialization attempt.
/// \note Original identifier: \c D_8009FE3C (0x8009FE3C).
extern char gControllerRumbleRetryFormat[];
/// Diagnostic format for exhausting rumble initialization attempts.
/// \note Original identifier: \c D_8009FE54 (0x8009FE54).
extern char gControllerRumbleGiveUpFormat[];
/// Advance pending Rumble Pak initialization attempts and record success or final failure.
/// \param port Controller port index from zero through three with a pending initialization record.
/// \returns Nothing.
void controllerRetryRumbleInitialization(s32 port) {
    ControllerRumbleMotorState *p;

    if ((controllerGetAccessoryState(port) != CONTROLLER_ACCESSORY_READY) && (controllerGetAccessoryState(port) != CONTROLLER_ACCESSORY_PENDING)) {
        DEBUG_PRINTF((gControllerRumbleWrongAccessoryFormat, port));
        gControllerRumbleMotorState[port].state = CONTROLLER_MOTOR_IDLE;
        return;
    }
    p = &gControllerRumbleMotorState[port];
    if (p->deadlineTicks <= osGetTime()) {
        if (controllerInitializeRumblePak(port)) {
            DEBUG_PRINTF((gControllerRumbleInitializedFormat, port));
            p->state = CONTROLLER_MOTOR_STOP;
            p->attempts = 0;
        } else {
            DEBUG_PRINTF((gControllerRumbleNotInitializedFormat, port));
            if (++p->attempts < 3) {
                DEBUG_PRINTF((gControllerRumbleRetryFormat, port));
                p->deadlineTicks = osGetTime() + OS_USEC_TO_CYCLES(1000000);
            } else {
                DEBUG_PRINTF((gControllerRumbleGiveUpFormat, port));
                controllerSetAccessoryState(port, CONTROLLER_ACCESSORY_FAILED);
                p->state = CONTROLLER_MOTOR_IDLE;
            }
        }
    }
}

/// Start a port’s initialized rumble motor, scheduling reinitialization if access fails.
/// \param port Controller port index from zero through three; requires initialized controller communication.
/// \returns Nothing.
void controllerStartRumbleMotor(s32 port) {
    if ((controllerGetAccessoryState(port) == CONTROLLER_ACCESSORY_READY) && (__osMotorAccess(controllerGetAccessoryStorage(port), 1) == 0)) {
        gControllerRumbleMotorState[port].state = CONTROLLER_MOTOR_WAIT_PULSE;
        return;
    }
    controllerScheduleRumbleInitialization(&gControllerRumbleMotorState[port]);
}

/// Advance the stop request for a rumble motor, scheduling reinitialization if access fails.
/// \param port Controller port index from zero through three; requires initialized controller communication.
/// \returns Nothing.
void controllerStopRumbleMotor(s32 port) {
    ControllerRumbleMotorState *p;

    if ((controllerGetAccessoryState(port) == CONTROLLER_ACCESSORY_READY) && (__osMotorAccess(controllerGetAccessoryStorage(port), 0) == 0)) {
        p = &gControllerRumbleMotorState[port];
        if (++p->attempts >= 3) {
            p->state = CONTROLLER_MOTOR_IDLE;
        }
    } else {
        controllerScheduleRumbleInitialization(&gControllerRumbleMotorState[port]);
    }
}

/// Request motor stopping when the current rumble pulse has expired.
/// \param port Controller port index from zero through three with an active timed pulse.
/// \returns Nothing.
void controllerUpdateRumblePulseExpiry(s32 port) {
    ControllerRumbleMotorState *p;

    p = &gControllerRumbleMotorState[port];
    if (osGetTime() >= p->deadlineTicks) {
        p->state = CONTROLLER_MOTOR_STOP;
        p->attempts = 0;
    }
}

/// Leave an idle rumble motor control state unchanged.
/// \param unused Unused controller port accepted by the state-handler interface.
/// \returns Nothing.
void controllerRumbleMotorIdle(s32 unused) {
}

/// Leave an idle rumble pattern state unchanged.
/// \param unused Unused controller port accepted by the state-handler interface.
/// \returns Nothing.
void controllerRumblePatternIdle(s32 unused) {
}

/// Cancel a port’s rumble pattern and request that its motor stop.
/// \param port Controller port index from zero through three.
/// \returns Nothing.
void controllerCancelRumblePattern(s32 port) {
    ControllerRumbleMotorState *p;

    gControllerRumblePatternState[port].state = CONTROLLER_PATTERN_IDLE;
    p = &gControllerRumbleMotorState[port];
    p->state = CONTROLLER_MOTOR_STOP;
    p->attempts = 0;
}


/// Advance an active rumble pattern, starting due pulses or requesting completion.
/// \param port Controller port index from zero through three with an active pattern.
/// \returns Nothing.
void controllerUpdateRumblePattern(s32 port) {
    ControllerRumblePatternState *timer;
    ControllerRumbleMotorState *pulse;
    timer = &gControllerRumblePatternState[port];
    if (osGetTime() >= timer->deadlineTicks) {
        timer->state = CONTROLLER_PATTERN_CANCEL;
        return;
    }
    if (osGetTime() < timer->nextPulseTicks)
        return;
    timer->nextPulseTicks += timer->intervalTicks;
    if (osGetTime() > timer->nextPulseTicks) {
        timer->nextPulseTicks = osGetTime();
    }
    pulse = &gControllerRumbleMotorState[port];
    pulse->state = CONTROLLER_MOTOR_START;
    pulse->deadlineTicks = osGetTime() + timer->pulseTicks;
}

/// Cancel a port’s rumble pattern and schedule accessory reinitialization.
/// \param port Controller port index from zero through three.
/// \returns Nothing.
void controllerResetRumblePort(s32 port) {
    gControllerRumblePatternState[port].state = CONTROLLER_PATTERN_IDLE;
    controllerScheduleRumbleInitialization(&gControllerRumbleMotorState[port]);
}

/// Private dispatch table indexed by ControllerPatternLifecycle.
typedef struct {
    void (*handlers[4])(s32); ///< Per-port pattern state handlers.
} ControllerPatternDispatch;
/// Private dispatch table indexed by ControllerMotorLifecycle.
typedef struct {
    void (*handlers[5])(s32); ///< Per-port motor state handlers.
} ControllerMotorDispatch;
/// Handlers for the rumble pattern lifecycle states.
/// \note Original identifier: \c D_800633F0 (0x800633F0).
extern ControllerPatternDispatch gControllerRumblePatternHandlers;
/// Handlers for the rumble motor lifecycle states.
/// \note Original identifier: \c D_80063400 (0x80063400).
extern ControllerMotorDispatch gControllerRumbleMotorHandlers;
/// Advance rumble patterns and motor control for all controller ports.
/// \returns Nothing; requires initialized controller communication and rumble state.
/// \note Portability: paired cursor initializations preserve IDO scheduling.
void controllerUpdateRumble(void) {
    s32 port;
    ControllerPatternDispatch first = gControllerRumblePatternHandlers;
    ControllerMotorDispatch second = gControllerRumbleMotorHandlers;
    ControllerRumblePatternState *p;
    ControllerRumbleMotorState *q;
    // clang-format off
    p = gControllerRumblePatternState; q = gControllerRumbleMotorState;
    // clang-format on
    port = 0;
    do {
        first.handlers[p->state](port);
        second.handlers[q->state](port);
        port++;
        p++;
        q++;
    } while (port != 4);
}

/// Diagnostic format for unsupported queued rumble commands.
/// \note Original identifier: \c D_8009FEFC (0x8009FEFC).
extern char gControllerRumbleUnknownCommandFormat[];
/// Seconds-to-microseconds factor for total rumble request duration.
/// \note Original identifier: \c D_8009FF18 (0x8009FF18).
extern f32 gControllerRumbleTotalMicrosecondsPerSecond;
/// Seconds-to-microseconds factor for rumble pulse intervals.
/// \note Original identifier: \c D_8009FF1C (0x8009FF1C).
extern f32 gControllerRumbleIntervalMicrosecondsPerSecond;
/// Seconds-to-microseconds factor for rumble pulse durations.
/// \note Original identifier: \c D_8009FF20 (0x8009FF20).
extern f32 gControllerRumblePulseMicrosecondsPerSecond;
/// Apply a queued rumble start, stop, or reset request to the selected port.
/// \param request Rumble request with a port from zero through three and command zero to start, one to stop, or two to reset; start requests include total duration, pulse interval, and pulse duration in seconds.
/// \returns Nothing.
void controllerDispatchRumbleRequest(ControllerRumbleRequest *request) {
    ControllerRumblePatternState *timer;
    s32 command;

    timer = &gControllerRumblePatternState[request->port];
    command = request->command;
    switch (command) {
    case CONTROLLER_RUMBLE_START:
        timer->state = CONTROLLER_PATTERN_RUNNING;
        timer->deadlineTicks =
            osGetTime() +
            OS_USEC_TO_CYCLES((u64)(request->durationSeconds * gControllerRumbleTotalMicrosecondsPerSecond));
        timer->nextPulseTicks = osGetTime();
        timer->intervalTicks =
            OS_USEC_TO_CYCLES((u64)(request->intervalSeconds * gControllerRumbleIntervalMicrosecondsPerSecond));
        timer->pulseTicks =
            OS_USEC_TO_CYCLES((u64)(request->pulseSeconds * gControllerRumblePulseMicrosecondsPerSecond));
        break;
    case CONTROLLER_RUMBLE_STOP:
        timer->state = CONTROLLER_PATTERN_CANCEL;
        break;
    case CONTROLLER_RUMBLE_RESET:
        timer->state = CONTROLLER_PATTERN_RESET;
        break;
    default:
        DEBUG_PRINTF((gControllerRumbleUnknownCommandFormat, command));
        break;
    }
}

/// Initialize rumble control state and attempt initialization of pending Rumble Paks.
/// \returns Nothing; requires initialized controller device status and serial communication.
void controllerInitializeRumbleState(void) {
    s32 i;

    for (i = 0; i < 4; i++) {
        gControllerRumbleMotorState[i].state = CONTROLLER_MOTOR_IDLE;
        gControllerRumblePatternState[i].state = CONTROLLER_PATTERN_IDLE;
        if (controllerGetAccessoryState(i) == CONTROLLER_ACCESSORY_PENDING) {
            controllerInitializeRumblePak(i);
        }
    }
}

/// Check whether a controller port has a running rumble pattern.
/// \param port Controller port index from zero through three.
/// \returns One while a pattern is scheduled to run, or zero otherwise.
s32 controllerIsRumblePatternScheduled(s32 port) {
    if (gControllerRumblePatternState[port].state == CONTROLLER_PATTERN_RUNNING) {
        return 1;
    }
    return 0;
}
