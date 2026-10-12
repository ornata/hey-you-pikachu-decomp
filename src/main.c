#include "camera_pose.h"
#include "camera_types.h"
#include "color_transition.h"
#include "camera_visibility.h"
#include "graphics_tasks.h"
#include "title_screen.h"
#include "backdrop_draw.h"
#include "system_menu.h"
#include "heap_free.h"
#include "heap_tail.h"
/* harvested: matched m2c drafts; pending style pass */
/**
 *  ─────────────────────────────────────────────────────────────────────────
 *
 *        /\  /\
 *       /  \/  \        HEY YOU, PIKACHU! (USA)  —  src/main.c
 *      |  o  o  |       boot thread + idle thread
 *       \  __  /
 *      __/|  |\__       ROM:  0x1050 .. 0x3FD0
 *     /   |__|   \      VRAM: 0x80000450 .. 0x800033D0
 *     \__/\  /\__/      compiler: IDO 7.1  -O2 -mips2 -32 -G0
 *         |==|
 *         \__/
 *
 *  Thread startup, in boot order:
 *
 *      entry (asm/entry.s)          clears BSS, jumps here with sp set
 *        └── main()                 creates + starts the idle thread
 *              └── IdleMain()       boots the scheduler, then parks
 *                                   itself at priority 0 forever
 *
 *  MATCHING BUILD NOTE
 *  ───────────────────
 *  Every function here must compile to the exact bytes in the retail
 *  ROM. Code style that looks odd (dead stores, unreachable code,
 *  pointless temporaries) is usually load-bearing: it is what the 1998
 *  compiler needed to emit the original instructions. Do not "clean
 *  up" a matched function.
 *
 *  UB / PORTABILITY LEDGER (see docs/UB.md for the project-wide list)
 *  ──────────────────────────────────────────────────────────────────
 *  Annotations in this file use the `UB:` and `NONPORTABLE:` comment
 *  markers. If you retarget this code at another compiler or platform,
 *  every marked site may legally change behavior.
 *  ─────────────────────────────────────────────────────────────────────────
 */
#include <ultra64.h>
#include "common.h"
#include "render_assets.h"
#include "rom_dma.h"
#include "asset_loader.h"
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
void romDmaInitializeHook(void);
void romDmaCreatePiManager(void);
void runtimeStartMainThread(void *stack);

#define IDLE_THREAD_ID 10
#define IDLE_THREAD_PRI 10












typedef struct PadSlot {
    u8 data[0x68];
} PadSlot;

/// Controller ports enabled by device initialization.
/// Original: D_800ACE80 (0x800ACE80).
extern u8 gControllerEnabledPortMask;
/// Enabled controller ports with a responding device.
/// Original: D_800ACE81 (0x800ACE81).
extern u8 gControllerConnectedPortMask;
/// Persistent cached device status for each controller port.
/// Original: D_800ACE88 (0x800ACE88).
extern OSContStatus gControllerPortStatus[4];
/// Persistent accessory context storage for each controller port.
/// Original: D_800ACE98 (0x800ACE98).
extern PadSlot gControllerAccessoryStorage[4];
/// Shared serial I/O completion queue owned by the controller subsystem.
/// Original: D_800AD038 (0x800AD038).
extern OSMesgQueue gSerialCompletionQueue;
/// Backing message slots for the serial I/O completion queue.
/// Original: D_800AD050 (0x800AD050).
extern OSMesg gSerialCompletionMessages[10];
/// Accessory lifecycle state for each controller port.
/// Original: D_800AD078 (0x800AD078).
extern s32 gControllerAccessoryState[4];
/// Controller thread queue for scheduler events and device commands.
/// Original: D_800AD088 (0x800AD088).
extern OSMesgQueue gControllerCommandQueue;
/// Backing message slots for the controller thread command queue.
/// Original: D_800AD0A0 (0x800AD0A0).
extern OSMesg gControllerCommandMessages[16];
/// Per-port rumble motor state, pulse deadline and initialization retry state.
/// \note Original identifier: \c D_800AD0E0 (0x800AD0E0).
extern u8 gControllerRumbleMotorState[0x60];
/// Per-port rumble pattern state, end time, pulse interval and pulse duration.
/// \note Original identifier: \c D_800AD140 (0x800AD140).
extern u8 gControllerRumblePatternState[0xA0];
/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern u8 gRenderCameras[];
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
void colorBlendToward(u8 *, u8 *, f32);
void cameraFadeOverlayToColor(s32, s32, s32, s32, s32, f32);
void colorStoreClampedRgba(s32, s32, s32, s32, u8 *);
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
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
void matrix33Transpose(void *);
void matrix34TransformPointVector(void *, f32 *, f32 *);

/// Scale converting an overlay alpha byte to a blend weight.
/// \note Original identifier: \c D_8009FC90 (0x8009FC90).
extern f32 gOverlayAlphaWeightScale;
void matrix34TranslateLocal(s32, s32, s32, s32);
/* 3A058 copies the twelve words of an affine 3-by-4 matrix. */
typedef struct {
    s32 w[12];
} MatrixBlock00800;
void matrix34Identity(MatrixBlock00800 *);
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
void matrix33ApplyRotationXYZDegrees(s32, f32, f32, f32);
void controllerThreadMain(void *);
/// Thread record for controller polling, voice requests and rumble servicing.
/// \note Original identifier: \c D_800AC4C0 (0x800AC4C0).
extern OSThread gControllerThread;
/// Controller scheduler-client registration; its address also marks the controller-thread stack top.
/// \note Original identifier: \c D_800ACE70 (0x800ACE70).
OSMesgQueue *controllerGetCommandQueue(void);
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
/// Initialize the operating system and start the boot idle thread.
/// \returns Nothing; execution continues in the newly started system threads.
///
/// \note UB: linked at the END of the idle stack region. libultra stacks grow DOWN, so the "stack
/// base" handed to osCreateThread is one-past-the-last u64 of storage. Taking the address is fine;
/// any dereference of gIdleThreadStackTop[0] would be out of bounds.
///
/// \note NONPORTABLE: `void main(void)` is not a hosted-C signature; there is no C runtime here,
/// entry.s calls it like any other function.
void main(void) {
    osInitialize();
    osCreateThread(&gIdleThread, IDLE_THREAD_ID, IdleMain, NULL, gIdleThreadStackTop, IDLE_THREAD_PRI);
    osStartThread(&gIdleThread);
}

/// Start the application services and provide the system idle thread.
/// \param arg Unused thread startup argument.
/// \returns Never returns.
///
/// \note NONPORTABLE: The original IDO build retains an unreachable epilogue
/// and homes the unused startup argument; other compilers may omit both.
void IdleMain(void *arg) {
    romDmaInitializeHook();
    romDmaCreatePiManager();
    runtimeStartMainThread(gMainSchedulerContext);
    osSetThreadPri(NULL, 0);
    for (;;) {
    }
}

/// Provide the runtime hook that requires no action.
/// \returns Nothing.
void runtimeNoOpHook(void) {
}

/// Provide the optional frame-rendering hook that requires no action.
/// \returns Nothing.
void renderOptionalFrameHook(void) {
}

/// Provide the audio effect-release interface for a build that requires no release action.
/// \param fx Unused effect reference accepted by the audio interface.
/// \returns Nothing.
///
/// \note NONPORTABLE: The original leaf homes its unused argument at 0(sp),
/// relying on the caller-provided argument area in the MIPS o32 ABI.
void n_alSynFreeFX(void *fx) {
}

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
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
void heapDebugNoOp();
void cameraResetOverlayColor(s32, s32, s32, s32, s32);
void cameraFadePrimarySecondaryOverlay(s32, s32, s32, s32, f32);
/// Load shared rendering assets and initialize camera and display-list resources.
/// \returns Nothing; requires an initialized local heap and must run before rendering resources are used.
void renderInitializeResources(void) {
    s32 size = gSharedRenderAssetsRomEnd - gSharedRenderAssetsRomStart;
    u8 *camera;
    s32 *buffer;
    gSharedRenderAssets = heapAllocLocal(&gGameHeap, size, 0xFA);
    heapDebugNoOp(&gGameHeap, gSharedRenderAssets);
    romDmaReadBlocking((s32)gSharedRenderAssetsRomStart, gSharedRenderAssets, size);
    gRenderCameraIndex = 0;
    /* Keep initialization with the loop for IDO address-load scheduling. */
    // clang-format off
    camera = gRenderCameras; do {
        // clang-format on
        _bzero(camera, 0xE0);
        *(CameraDefaults *)camera = gRenderCameraDefaults;
        *(s32 *)(camera + 0xA8) = 20;
        *(s32 *)(camera + 0xAC) = 20;
        *(s32 *)(camera + 0xB0) = 299;
        *(s32 *)(camera + 0xB4) = 219;
        camera += 0xE0;
    } while (camera < (u8 *)&gRenderDisplayListCursor);
    cameraResetOverlayColor(0, 0, 0, 0, 0);
    cameraFadeOverlayToColor(0, 0, 0, 0, 0, 0.0f);
    cameraFadePrimarySecondaryOverlay(0, 0, 0, 255, 0.0f);
    gCameraElapsedTintNeedsOrigin = 1;
    gCameraElapsedTintEnabled = 0;
    gCameraElapsedTintInitialSeconds = 0.0f;
    /* Same scheduling requirement for the display-list buffer pair. */
    // clang-format off
    buffer = gRenderCommandBuffers; do {
        // clang-format on
        *buffer = (s32)heapAllocLocal(&gGameHeap, 0xC000, 0x125);
        buffer++;
    } while (buffer != (s32 *)gRenderTaskViewports);
    gAreaRenderAssets = 0;
}

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
/// Load the current area’s rendering assets for use by its scene.
/// \returns Nothing; requires a loaded area configuration and no existing area asset allocation to replace.
void renderLoadAreaAssets(void) {
    s32 outputSize;
    s32 scratchSize;
    u8 *temporary;
    outputSize = *(u32 *)(gCurrentAreaConfiguration + 0x54) - *(u32 *)(gCurrentAreaConfiguration + 0x50);
    gAreaRenderAssets = heapAllocLocal(&gGameHeap, outputSize, 0x140);
    heapDebugNoOp(&gGameHeap, gAreaRenderAssets);
    scratchSize = (s32)(*(u32 *)(gCurrentAreaConfiguration + 0x60) - *(u32 *)(gCurrentAreaConfiguration + 0x50)) * 8 +
                  (s32)(*(u32 *)(gCurrentAreaConfiguration + 0x58) - *(u32 *)(gCurrentAreaConfiguration + 0x50));
    temporary = heapAllocTailLocal(&gGameHeap, scratchSize, 0x14C);
    assetLoadAndPatchFromRom(*(u32 *)(gCurrentAreaConfiguration + 0x48), (u8 *)gAreaRenderAssets, temporary,
                             *(u32 *)(gCurrentAreaConfiguration + 0x58) - *(u32 *)(gCurrentAreaConfiguration + 0x50),
                             *(u32 *)(gCurrentAreaConfiguration + 0x54) - *(u32 *)(gCurrentAreaConfiguration + 0x50),
                             *(u32 *)(gCurrentAreaConfiguration + 0x5C) - *(u32 *)(gCurrentAreaConfiguration + 0x50),
                             *(u32 *)(gCurrentAreaConfiguration + 0x60) - *(u32 *)(gCurrentAreaConfiguration + 0x50));
    heapFreeLocal(&gGameHeap, temporary);
}


/// Release the current area’s rendering asset allocation.
/// \returns Nothing; requires a previously loaded area asset allocation.
void renderUnloadAreaAssets(void) {
    heapFreeLocal(&gGameHeap, (void *)gAreaRenderAssets);
    gAreaRenderAssets = 0;
}

/// Build the camera-relative world transform used to position interface models.
/// \param output Address of writable storage for a three-by-four floating-point transform; uses the active camera position and current player viewing angles.
/// \returns Nothing.
void cameraBuildInterfaceTransform(s32 output) {
    u8 *sp1C;

    sp1C = gRenderCameras + gRenderCameraIndex * 0xE0;
    matrix34Identity((MatrixBlock00800 *)output);
    matrix34TranslateLocal(output, *(s32 *)(sp1C + 0), *(s32 *)(sp1C + 4), *(s32 *)(sp1C + 8));
    matrix33ApplyRotationXYZDegrees(output, -(gPlayerPitch + gPlayerPitchOffset),
                                    gPlayerViewShake + gPlayerYaw + 180.0f, 0.0f);
}

/// Whether the next display-list setup must emit the one-time initial render state.
/// \note Original identifier: \c D_800633E4 (0x800633E4).
extern s32 gRenderInitialStatePending;
extern Gfx D_637D8[], D_636B0[], D_63798[];
/// Prepare the current display list to draw with shared and current-area rendering assets.
/// \returns Nothing; requires initialized rendering resources and a writable current display list.
void renderPrepareDisplayList(void) {
    gSPSegment(gRenderDisplayListCursor++, 0, 0);
    gSPSegment(gRenderDisplayListCursor++, 1, osVirtualToPhysical(gSharedRenderAssets));
    gSPSegment(gRenderDisplayListCursor++, 4, osVirtualToPhysical((void *)gAreaRenderAssets));
    gSPDisplayList(gRenderDisplayListCursor++, D_637D8);
    {
        s32 *flag = &gRenderInitialStatePending;

        if (*flag != 0) {
            gSPDisplayList(gRenderDisplayListCursor++, D_636B0);
            *flag = 0;
        }
    }
    gSPDisplayList(gRenderDisplayListCursor++, D_63798);
}

/// Upper near-zero cosine bound selecting the camera pitch fallback slope.
/// \note Original identifier: \c D_8009FC88 (0x8009FC88).
extern f32 gCameraPitchCosineUpperTolerance;
/// Lower near-zero cosine bound selecting the camera pitch fallback slope.
/// \note Original identifier: \c D_8009FC8C (0x8009FC8C).
extern f32 gCameraPitchCosineLowerTolerance;
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
/// Set a camera’s world position and viewing orientation.
/// \param index Valid camera slot to configure.
/// \param x World-space eye X coordinate.
/// \param y World-space eye height.
/// \param z World-space eye Z coordinate.
/// \param pitch Viewing pitch in degrees; positive angles look downward.
/// \param yaw Viewing yaw in degrees, measured from positive Z toward positive X.
/// \param roll Camera up-vector angle in degrees, measured from positive Y toward positive X.
/// \returns Nothing.
void cameraSetPositionAndAngles(s32 index, f32 x, f32 y, f32 z, f32 pitch, f32 yaw, f32 roll) {
    f32 *camera = (f32 *)(gRenderCameras + index * 0xE0);
    f32 denominator;
    f32 slope;
    /* IDO schedules these three position stores together. */
    // clang-format off
    camera[0] = x; camera[1] = y; camera[2] = z;
    // clang-format on
    denominator = mathCosDegrees(pitch);
    if (denominator < gCameraPitchCosineUpperTolerance && gCameraPitchCosineLowerTolerance < denominator) {
        slope = 100.0f;
    } else {
        slope = mathSinDegrees(pitch) / denominator;
    }
    camera[3] = mathSinDegrees(yaw) + x;
    camera[4] = y - slope;
    camera[5] = mathCosDegrees(yaw) + z;
    camera[6] = mathSinDegrees(roll);
    camera[7] = mathCosDegrees(roll);
    camera[8] = 0.0f;
}

/// Aim a camera from a world-space eye position toward a target with world-up orientation.
/// \param cameraIndex Valid camera slot to configure.
/// \param eye Three-component world-space eye position in X, Y, Z order.
/// \param target Three-component world-space look-at position in X, Y, Z order.
/// \returns Nothing.
void cameraSetEyeAndTarget(s32 cameraIndex, f32 *eye, f32 *target) {
    u8 *cameraState;

    cameraState = gRenderCameras + cameraIndex * 0xE0;
    ((MatrixCamera *)cameraState)->eye[0] = eye[0];
    ((MatrixCamera *)cameraState)->eye[1] = eye[1];
    ((MatrixCamera *)cameraState)->eye[2] = eye[2];
    ((MatrixCamera *)cameraState)->target[0] = target[0];
    ((MatrixCamera *)cameraState)->target[1] = target[1];
    ((MatrixCamera *)cameraState)->target[2] = target[2];
    ((MatrixCamera *)cameraState)->up[2] = 0.0f;
    ((MatrixCamera *)cameraState)->up[0] = 0.0f;
    ((MatrixCamera *)cameraState)->up[1] = 1.0f;
}

/// Framebuffer and command-buffer index for the current frame.
/// \note Original identifier: \c D_800F298C (0x800F298C).
extern s32 gGraphicsFramebufferIndex;
/// First color framebuffer storage used for drawing and presentation.
/// \note Original identifier: \c D_803B5000 (0x803B5000).
extern u8 gColorFramebuffer0[];
/// Fill a rectangle in the current framebuffer with a solid color.
/// \param left Left pixel coordinate; empty or reversed rectangles are ignored.
/// \param top Top pixel coordinate.
/// \param right Inclusive right pixel coordinate, limited to 319.
/// \param bottom Inclusive bottom pixel coordinate, limited to 239.
/// \param r Red intensity from zero through 255.
/// \param g Green intensity from zero through 255.
/// \param b Blue intensity from zero through 255.
/// \param a One-bit alpha value, zero or one.
/// \returns Nothing.
void renderFillFramebufferRectangle(u32 left, u32 top, u32 right, u32 bottom, u8 r, u8 g, u8 b, u8 a) {
    if (right >= 320)
        right = 319;
    if (bottom >= 240)
        bottom = 239;
    if (left < right && top < bottom) {
        gDPPipeSync(gRenderDisplayListCursor++);
        gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_FILL);
        gDPSetColorImage(gRenderDisplayListCursor++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320,
                         osVirtualToPhysical(gColorFramebuffer0 + gGraphicsFramebufferIndex * 0x25800));
        gDPSetFillColor(gRenderDisplayListCursor++, (GPACK_RGBA5551(r, g, b, a) << 16) | GPACK_RGBA5551(r, g, b, a));
        gDPFillRectangle(gRenderDisplayListCursor++, left, top, right, bottom);
    }
}


/// Draw the configured backdrop or clear the framebuffer to black.
/// \param useBackdrop Nonzero draws the configured backdrop; zero fills the screen with opaque black.
/// \returns Nothing.
void renderDrawBackground(s32 useBackdrop) {
    if (useBackdrop != 0) {
        backdropDraw(&gRenderDisplayListCursor);
        return;
    }
    renderFillFramebufferRectangle(0, 0, 0x13F, 0xEF, 0, 0, 0, 1);
}

/// Physical depth-buffer address used in graphics commands.
/// \note Original identifier: \c D_38F800 (0x38F800).
extern u8 gDepthBufferPhysicalAddress[];
/// Clear a rectangular region of the depth buffer for subsequent drawing.
/// \param left Left pixel coordinate of a valid depth-buffer rectangle.
/// \param top Top pixel coordinate.
/// \param right Inclusive right pixel coordinate.
/// \param bottom Inclusive bottom pixel coordinate.
/// \returns Nothing.
void renderClearDepthRectangle(s32 left, s32 top, s32 right, s32 bottom) {
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetDepthImage(gRenderDisplayListCursor++, gDepthBufferPhysicalAddress);
    gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_FILL);
    gDPSetColorImage(gRenderDisplayListCursor++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, gDepthBufferPhysicalAddress);
    gDPSetFillColor(gRenderDisplayListCursor++, 0xFFFCFFFC);
    gDPFillRectangle(gRenderDisplayListCursor++, left, top, right, bottom);
}

/// Blend an RGBA color toward a target color.
/// \param dest Four-byte RGBA color to update.
/// \param source Four-byte target RGBA color.
/// \param weight Fraction of the original destination color to retain, limited to zero through one; zero selects the target.
/// \returns Nothing.
void colorBlendToward(u8 *dest, u8 *source, f32 weight) {
    s32 i;
    s32 value;
    f32 inverse;
    if (weight <= 0.0f) {
        for (i = 0; i < 4; i++)
            dest[i] = source[i];
        return;
    }
    if (weight < 0.0f)
        weight = 0.0f;
    else if (weight > 1.0f)
        weight = 1.0f;
    inverse = 1.0f - weight;
    for (i = 0; i < 4; i++) {
        value = (u32)*dest * weight + (u32)*source * inverse;
        if (value < 0)
            value = 0;
        else if (value > 255)
            value = 255;
        *dest = value;
        dest++;
        source++;
    }
}

/// Advance a timed RGBA color transition by the current frame duration.
/// \param color Four writable RGBA components to update.
/// \param target Four target RGBA components, each from zero to 255.
/// \param remaining Remaining transition time in seconds, updated toward zero.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800010C0 (0x800010C0).
void colorAdvanceTransition(u8 *color, u8 *target, f32 *remaining) {
    f32 var_f12;

    *remaining -= gRuntimeFrameSeconds;
    if (*remaining < 0.0f) {
        *remaining = 0.0f;
    }
    if (*remaining <= gRuntimeFrameSeconds) {
        var_f12 = 0.0f;
    } else {
        var_f12 = 1.0f - (gRuntimeFrameSeconds / *remaining);
    }
    colorBlendToward(color, target, var_f12);
}

/// Advance camera overlay transitions and refresh the blended overlay color for the current frame.
/// \param cameraIndex Valid camera slot whose overlay colors are updated.
/// \param advanceBlend Nonzero also advances the additional blend transition; zero retains its current contribution.
/// \returns Nothing.
void cameraUpdateOverlayColors(s32 cameraIndex, s32 advanceBlend) {
    u8 *cameraState;

    cameraState = gRenderCameras + cameraIndex * 0xE0;
    colorAdvanceTransition(cameraState + 0xD8, cameraState + 0xD4, (f32 *)(cameraState + 0xDC));
    colorAdvanceTransition(cameraState + 0xC0, cameraState + 0xBC, (f32 *)(cameraState + 0xC4));
    if (advanceBlend != 0) {
        colorAdvanceTransition(cameraState + 0xCC, cameraState + 0xC8, (f32 *)(cameraState + 0xD0));
    }
    cameraState[0xB8] = cameraState[0xC0];
    cameraState[0xB9] = cameraState[0xC1];
    cameraState[0xBA] = cameraState[0xC2];
    cameraState[0xBB] = cameraState[0xC3];
    colorBlendToward(cameraState + 0xB8, cameraState + 0xCC,
                     (f32)(0xFF - cameraState[0xCF]) * gOverlayAlphaWeightScale);
}

/// Segmented shared display list drawing the camera overlay rectangle.
/// \note Original identifier: \c D_100C960 (0x100C960).
extern Gfx gCameraOverlayRectangleDisplayList[];
/// Draw the camera’s blended color overlay when its opacity is nonzero.
/// \param index Valid camera slot whose overlay is drawn in the current drawing context.
/// \returns Nothing.
void cameraDrawOverlay(s32 index) {
    u8 *camera = gRenderCameras + index * 0xE0;
    if (camera[0xBB] != 0) {
        gDPPipeSync(gRenderDisplayListCursor++);
        gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
        gDPSetRenderMode(gRenderDisplayListCursor++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
        gDPSetCombineMode(gRenderDisplayListCursor++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, camera[0xB8], camera[0xB9], camera[0xBA], camera[0xBB]);
        gSPDisplayList(gRenderDisplayListCursor++, gCameraOverlayRectangleDisplayList);
    }
}

/// Draw the camera’s independent secondary color overlay when its opacity is nonzero.
/// \param index Valid camera slot whose secondary overlay is drawn in the current drawing context.
/// \returns Nothing.
void cameraDrawSecondaryOverlay(s32 index) {
    u8 *camera = gRenderCameras + index * 0xE0;
    if (camera[0xDB] != 0) {
        gDPPipeSync(gRenderDisplayListCursor++);
        gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
        gDPSetRenderMode(gRenderDisplayListCursor++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
        gDPSetCombineMode(gRenderDisplayListCursor++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, camera[0xD8], camera[0xD9], camera[0xDA], camera[0xDB]);
        gSPDisplayList(gRenderDisplayListCursor++, gCameraOverlayRectangleDisplayList);
    }
}

/// Whether the primary-actor elapsed-script camera tint is enabled.
/// \note Original identifier: \c D_800AC4B4 (0x800AC4B4).
extern s32 gCameraElapsedTintEnabled;

/// Enable the camera tint that grows with the primary actor’s elapsed script time.
/// \param initialElapsed Initial elapsed time in seconds used when the effect first starts; negative values are ignored.
/// \returns Nothing.
void cameraEnableElapsedTimeTint(f32 initialElapsed) {
    if (initialElapsed >= 0.0f) {
        gCameraElapsedTintInitialSeconds = initialElapsed;
        gCameraElapsedTintEnabled = 1;
    }
}

/// Store an RGBA color with each channel limited to the byte range.
/// \param red Requested red intensity, limited to zero through 255.
/// \param green Requested green intensity, limited to zero through 255.
/// \param blue Requested blue intensity, limited to zero through 255.
/// \param alpha Requested opacity, limited to zero through 255.
/// \param color Output buffer holding four bytes in RGBA order.
/// \returns Nothing.
void colorStoreClampedRgba(s32 red, s32 green, s32 blue, s32 alpha, u8 *color) {
    if (red < 0) {
        red = 0;
    } else if ((u32)red >= 0x100U) {
        red = 0xFF;
    }
    if (green < 0) {
        green = 0;
    } else if ((u32)green >= 0x100U) {
        green = 0xFF;
    }
    if (blue < 0) {
        blue = 0;
    } else if ((u32)blue >= 0x100U) {
        blue = 0xFF;
    }
    if (alpha < 0) {
        alpha = 0;
    } else if ((u32)alpha >= 0x100U) {
        alpha = 0xFF;
    }
    color[0] = red;
    color[1] = green;
    color[2] = blue;
    color[3] = alpha;
}

/// Upper bound of values stored as zero by the float tolerance helper.
/// \note Original identifier: \c D_8009FC94 (0x8009FC94).
extern f32 gFloatStoreZeroUpperTolerance;
/// Lower bound of values stored as zero by the float tolerance helper.
/// \note Original identifier: \c D_8009FC98 (0x8009FC98).
extern f32 gFloatStoreZeroLowerTolerance;

/// Store a floating-point value while treating sufficiently small magnitudes as zero.
/// \param value Value to store; magnitudes below the configured near-zero tolerance become zero.
/// \param output Destination for the resulting value.
/// \returns Nothing.
void floatStoreWithZeroTolerance(f32 value, f32 *output) {
    if ((value < gFloatStoreZeroUpperTolerance) && (gFloatStoreZeroLowerTolerance < value)) {
        *output = 0.0f;
        return;
    }
    *output = value;
}

/// Set the target color and duration for a camera overlay transition.
/// \param cameraIndex Valid camera slot to configure.
/// \param red Target red intensity, limited to zero through 255.
/// \param green Target green intensity, limited to zero through 255.
/// \param blue Target blue intensity, limited to zero through 255.
/// \param alpha Target opacity, limited to zero through 255.
/// \param duration Transition time in seconds; near-zero values request immediate completion on update.
/// \returns Nothing.
void cameraFadeOverlayToColor(s32 cameraIndex, s32 red, s32 green, s32 blue, s32 alpha, f32 duration) {
    u8 *sp20;

    sp20 = gRenderCameras + cameraIndex * 0xE0;
    floatStoreWithZeroTolerance(duration, (f32 *)(sp20 + 0xC4));
    colorStoreClampedRgba(red, green, blue, alpha, sp20 + 0xBC);
}

/// Set the target color and duration of camera slot zero’s additional overlay blend.
/// \param red Target red intensity, limited to zero through 255.
/// \param green Target green intensity, limited to zero through 255.
/// \param blue Target blue intensity, limited to zero through 255.
/// \param alpha Target opacity, limited to zero through 255.
/// \param duration Transition time in seconds; near-zero values request completion on the next applicable camera update.
/// \returns Nothing.
void cameraFadePrimaryOverlayBlend(s32 red, s32 green, s32 blue, s32 alpha, f32 duration) {
    colorStoreClampedRgba(red, green, blue, alpha, gPrimaryOverlayBlendTarget);
    floatStoreWithZeroTolerance(duration, &gPrimaryOverlayBlendRemainingSeconds);
}

/// Reset a camera overlay to a specified color and clear its additional blend contribution.
/// \param cameraIndex Valid camera slot to configure.
/// \param red Red intensity, limited to zero through 255.
/// \param green Green intensity, limited to zero through 255.
/// \param blue Blue intensity, limited to zero through 255.
/// \param alpha Opacity, limited to zero through 255.
/// \returns Nothing.
void cameraResetOverlayColor(s32 cameraIndex, s32 red, s32 green, s32 blue, s32 alpha) {
    u8 *sp20;

    sp20 = gRenderCameras + cameraIndex * 0xE0;
    colorStoreClampedRgba(red, green, blue, alpha, sp20 + 0xB8);
    colorStoreClampedRgba(red, green, blue, alpha, sp20 + 0xC0);
    colorStoreClampedRgba(0, 0, 0, 0, sp20 + 0xCC);
}

/// Set the target color and duration of camera slot zero’s independent secondary overlay.
/// \param red Target red intensity, limited to zero through 255.
/// \param green Target green intensity, limited to zero through 255.
/// \param blue Target blue intensity, limited to zero through 255.
/// \param alpha Target opacity, limited to zero through 255.
/// \param duration Transition time in seconds; near-zero values request completion on the next applicable camera update.
/// \returns Nothing.
void cameraFadePrimarySecondaryOverlay(s32 red, s32 green, s32 blue, s32 alpha, f32 duration) {
    colorStoreClampedRgba(red, green, blue, alpha, gPrimarySecondaryOverlayTarget);
    floatStoreWithZeroTolerance(duration, &gPrimarySecondaryOverlayRemainingSeconds);
}

/// Draw an RGBA overlay across the current view and write its depth.
/// \param r Red intensity from zero through 255.
/// \param g Green intensity from zero through 255.
/// \param b Blue intensity from zero through 255.
/// \param a Opacity from zero through 255.
/// \returns Nothing.
void renderDrawColorOverlayWithDepth(u8 r, u8 g, u8 b, u8 a) {
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
    gDPSetDepthSource(gRenderDisplayListCursor++, G_ZS_PRIM);
    gDPSetPrimDepth(gRenderDisplayListCursor++, 0xFFFF, 0);
    gDPSetRenderMode(gRenderDisplayListCursor++, G_RM_AA_XLU_SURF | Z_UPD, G_RM_AA_XLU_SURF2 | Z_UPD);
    gDPSetCombineMode(gRenderDisplayListCursor++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, r, g, b, a);
    gSPDisplayList(gRenderDisplayListCursor++, gCameraOverlayRectangleDisplayList);
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetDepthSource(gRenderDisplayListCursor++, G_ZS_PIXEL);
}

f32 viewportSetPixelRectangle(Vp *, s32, s32, s32, s32, s32, s32, f32);
void matrixFixedTo34(Mtx, f32 *);
/// Additional aspect scale applied when constructing the camera viewport projection.
/// \note Original identifier: \c D_8009FC9C (0x8009FC9C).
extern f32 gCameraProjectionAspectScale;
/// Prepare a camera’s viewport, view and projection matrices, and world-to-view transforms.
/// \param index Valid camera slot to prepare; requires a loaded area configuration.
/// \param viewport Writable buffer suitable for one aligned viewport structure.
/// \param buffer Writable aligned buffer for two fixed-point matrices, retained by the camera until replaced.
/// \returns The perspective normalization value for rendering with the prepared projection.
s32 cameraBuildViewProjection(s32 index, u8 *viewport, u8 *buffer) {
    s32 row;
    u16 normalize;
    f32 scale;
    Mtx *view;
    f32 distance;
    MatrixCamera *camera;
    MatrixCamera *cameras = (MatrixCamera *)gRenderCameras;
    camera = (MatrixCamera *)((u8 *)cameras + index * sizeof(*cameras));
    guPerspective((Mtx *)buffer, &normalize, 45.0f,
                  viewportSetPixelRectangle((Vp *)viewport, camera->viewport[0], camera->viewport[1],
                                            camera->viewport[2], camera->viewport[3], 320, 240,
                                            gCameraProjectionAspectScale),
                  0.5f, 1000.0f, 1.0f);
    view = (Mtx *)buffer + 1;
    guLookAt(view, camera->eye[0], camera->eye[1], camera->eye[2], camera->target[0], camera->target[1],
             camera->target[2], camera->up[0], camera->up[1], camera->up[2]);
    guMtxCatL(view, (Mtx *)buffer, (Mtx *)buffer);
    matrixFixedTo34(*view, camera->view.values);
    distance = *(f32 *)(gCurrentAreaConfiguration + 0x2C) / mathSinDegrees(36.0f);
    camera->distance = distance;
    scale = mathCosDegrees(36.0f) * distance;
    camera->scaled = camera->view;
    for (row = 0; row < 4; row++) {
        cameras[index].scaled.values[row * 3] *= scale;
        cameras[index].scaled.values[row * 3 + 1] *= scale;
    }
    camera->matrices = (Mtx *)buffer;
    return normalize;
}

/// Prepare and apply a camera’s viewport, projection, and area rendering settings to the current drawing context.
/// \param cameraIndex Valid camera slot to render using the current frame’s buffers and loaded area configuration.
/// \returns Nothing.
void cameraApplyRenderingState(s32 cameraIndex) {
    u8 *sp1C;
    u8 *sp18;
    s32 perspectiveNormalization;

    sp18 = gRenderTaskViewports + gGraphicsNextTaskIndex * 0x10;
    sp1C = gRenderTaskMatrices + (gGraphicsNextTaskIndex << 7);
    perspectiveNormalization = cameraBuildViewProjection(cameraIndex, sp18, sp1C);
    gSPPerspNormalize(gRenderDisplayListCursor++, perspectiveNormalization);
    gSPViewport(gRenderDisplayListCursor++, sp18);
    gSPMatrix(gRenderDisplayListCursor++, sp1C + 0x80000000, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    renderApplyAreaFog(cameraIndex, &gRenderDisplayListCursor);
}

/// Apply the current area’s fog and texture filtering settings to the drawing context.
/// \param unused Unused camera index accepted by the rendering interface.
/// \returns Nothing; requires a loaded area configuration with distinct near and far fog distances.
void renderApplyAreaFog(s32 unused) {
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetTextureFilter(gRenderDisplayListCursor++, G_TF_BILERP);
    gDPSetFogColor(gRenderDisplayListCursor++, gCurrentAreaConfiguration[0x38], gCurrentAreaConfiguration[0x39],
                   gCurrentAreaConfiguration[0x3A], gCurrentAreaConfiguration[0x3B]);
    /* Pack fog offset before multiplier to preserve the original division order. */
    gMoveWd(gRenderDisplayListCursor++, G_MW_FOG, G_MWO_FOG,
            _SHIFTL(((500 - *(s32 *)(gCurrentAreaConfiguration + 0x30)) * 256 /
                     (*(s32 *)(gCurrentAreaConfiguration + 0x34) - *(s32 *)(gCurrentAreaConfiguration + 0x30))),
                    0, 16) |
                _SHIFTL((128000 /
                         (*(s32 *)(gCurrentAreaConfiguration + 0x34) - *(s32 *)(gCurrentAreaConfiguration + 0x30))),
                        16, 16));
}

f32 actorGetPrimaryScriptElapsedTime(); /* extern */
/// Primary-actor script-time origin used to calculate elapsed camera tint.
/// \note Original identifier: \c D_800633E8 (0x800633E8).
extern f32 gCameraElapsedTintOriginSeconds;
/// Whether the elapsed-script tint needs its initial script-time origin.
/// \note Original identifier: \c D_800AC4B0 (0x800AC4B0).
extern s32 gCameraElapsedTintNeedsOrigin;

/// Update the camera’s reddish tint according to elapsed primary-actor script time when the effect is enabled.
/// \param cameraIndex Valid camera slot to tint; requires an active primary actor when the effect is enabled.
/// \returns Nothing.
void cameraUpdateElapsedTimeTint(s32 cameraIndex) {
    u32 elapsedTintAlpha;
    u32 clampedTintAlpha;

    if (gCameraElapsedTintEnabled == 0) {
        gCameraElapsedTintNeedsOrigin = 1;
        gCameraElapsedTintInitialSeconds = 0.0f;
        return;
    }
    if (gCameraElapsedTintNeedsOrigin != 0) {
        gCameraElapsedTintOriginSeconds = actorGetPrimaryScriptElapsedTime() - gCameraElapsedTintInitialSeconds;
        gCameraElapsedTintNeedsOrigin = 0;
        return;
    }
    elapsedTintAlpha = (u32)(2.0f * (actorGetPrimaryScriptElapsedTime() - gCameraElapsedTintOriginSeconds));
    clampedTintAlpha = elapsedTintAlpha;
    if (elapsedTintAlpha >= 0x97U) {
        clampedTintAlpha = 0x96;
    }
    cameraFadeOverlayToColor(cameraIndex, 0x6E, 0x28, 0x2D, clampedTintAlpha, 0.0f);
}

extern Gfx D_100C7E0[];
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;
/// Scheduler event queue consumed by the runtime thread.
/// \note Original identifier: \c D_800AE4C0 (0x800AE4C0).
extern OSMesgQueue gRuntimeEventQueue;
/// Shared ROM cache owning loaded texture images, palettes and pending reads.
/// \note Original identifier: \c D_800B67B0 (0x800B67B0).
extern u8 gTextureRomCache[];
/// Start of the current graphics command-list submission.
/// \note Original identifier: \c D_800ABDD4 (0x800ABDD4).
extern Gfx *gRenderDisplayListStart;
void objectDrawGroup(s32);
void hudDraw(Gfx **);
s32 messageBarUpdateAndDraw(void);
void hudDrawAreaLabel(Gfx **);
void presentationDrawCardScreen(Gfx **);
void presentationDrawCollectionScreen(Gfx **);
void eyeCatchDraw(Gfx **);
void runtimeDrawPromptPanel(void);
void toolboxDrawFrame(s32);
void textWindowDrawFrame(void);
void wholeMapDraw(Gfx **);
void fontCacheReleaseUses(void);
void romCacheReleaseUses(u8 *);
/// Draw the current gameplay or menu mode and submit its graphics task.
/// \returns Nothing; requires a prepared current frame and initialized resources for the active mode.
void renderDrawCurrentMode(void) {
    gSPDisplayList(gRenderDisplayListCursor++, D_100C7E0);
    switch (gRuntimeGameMode) {
    case 0:
        objectDrawGroup(0);
        cameraUpdateElapsedTimeTint(gRenderCameraIndex);
        cameraUpdateOverlayColors(gRenderCameraIndex, 1);
        cameraDrawOverlay(gRenderCameraIndex);
        hudDraw(&gRenderDisplayListCursor);
        messageBarUpdateAndDraw();
        cameraDrawSecondaryOverlay(gRenderCameraIndex);
        hudDrawAreaLabel(&gRenderDisplayListCursor);
        break;
    case 18:
        titleScreenDrawFrame(&gRenderDisplayListCursor);
        break;
    case 4:
        presentationDrawCardScreen(&gRenderDisplayListCursor);
        break;
    case 5:
        presentationDrawCollectionScreen(&gRenderDisplayListCursor);
        break;
    case 6:
    case 11:
    case 12:
        objectDrawGroup(0);
        cameraUpdateOverlayColors(gRenderCameraIndex, 0);
        cameraDrawOverlay(gRenderCameraIndex);
        hudDraw(&gRenderDisplayListCursor);
        cameraDrawSecondaryOverlay(gRenderCameraIndex);
        break;
    case 13:
        eyeCatchDraw(&gRenderDisplayListCursor);
        cameraUpdateOverlayColors(gRenderCameraIndex, 0);
        cameraDrawSecondaryOverlay(gRenderCameraIndex);
        break;
    case 7:
    case 14:
    case 17:
        objectDrawGroup(0);
        cameraUpdateOverlayColors(gRenderCameraIndex, 0);
        cameraDrawOverlay(gRenderCameraIndex);
        cameraDrawSecondaryOverlay(gRenderCameraIndex);
        runtimeDrawPromptPanel();
        break;
    case 15:
    case 16:
        objectDrawGroup(0);
        cameraUpdateOverlayColors(gRenderCameraIndex, 0);
        cameraDrawOverlay(gRenderCameraIndex);
        cameraDrawSecondaryOverlay(gRenderCameraIndex);
        break;
    case 1:
        toolboxDrawFrame((s32)&gRenderDisplayListCursor);
        messageBarUpdateAndDraw();
        break;
    case 2:
        systemMenuDrawFrame(&gRenderDisplayListCursor);
        break;
    case 3:
        textWindowDrawFrame();
        break;
    case 8:
        wholeMapDraw(&gRenderDisplayListCursor);
        break;
    }
    fontCacheReleaseUses();
    romCacheReleaseUses(gTextureRomCache);
    graphicsTaskSubmit(0, 0, &gRenderDisplayListStart, &gRenderDisplayListCursor, &gRuntimeEventQueue);
}

/// Identifier of the currently active runtime program.
/// \note Original identifier: \c D_800AE4F0 (0x800AE4F0).
/// Number of startup frames whose full framebuffer has been cleared, capped at two.
/// \note Original identifier: \c D_800633EC (0x800633EC).
extern s32 gRenderInitialClearCount, gRuntimeCurrentProgram;
void renderPrepareDisplayList(void);

/// Begin a frame with the current background and camera, then invoke the active runtime drawing callback.
/// \returns Nothing; requires initialized rendering resources and a valid active runtime mode.
void renderBeginFrame(void) {
    s32 clearFrames;
    s32 *counter = &gRenderInitialClearCount;
    gRenderDisplayListCursor = (Gfx *)gRenderCommandBuffers[gGraphicsFramebufferIndex];
    gRenderDisplayListStart = gRenderDisplayListCursor;
    gRenderCameraIndex = 0;
    renderPrepareDisplayList();
    clearFrames = *counter;
    if (clearFrames < 2) {
        clearFrames++;
        gDPPipeSync(gRenderDisplayListCursor++);
        gDPSetScissor(gRenderDisplayListCursor++, G_SC_NON_INTERLACE, 0, 0, 319, 239);
        *counter = clearFrames;
        renderClearDepthRectangle(0, 0, 319, 239);
        renderDrawBackground(0);
        gDPPipeSync(gRenderDisplayListCursor++);
        gDPSetScissor(gRenderDisplayListCursor++, G_SC_NON_INTERLACE,
                      *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xA8),
                      *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xAC),
                      *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xB0),
                      *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xB4));
    } else {
        gDPPipeSync(gRenderDisplayListCursor++);
        gDPSetScissor(gRenderDisplayListCursor++, G_SC_NON_INTERLACE,
                      *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xA8),
                      *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xAC),
                      *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xB0),
                      *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xB4));
        renderClearDepthRectangle(*(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xA8),
                                  *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xAC),
                                  *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xB0),
                                  *(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xB4));
        renderDrawBackground(*(s32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0xA4));
    }
    cameraApplyRenderingState(gRenderCameraIndex);
    if (gRuntimeProgramDrawEntries[gRuntimeCurrentProgram].callback != NULL) {
        (gRuntimeProgramDrawEntries[gRuntimeCurrentProgram].callback)();
    }
}

/// Whether final frame submission invokes the optional rendering hook.
/// \note Original identifier: \c D_80088F18 (0x80088F18).
extern s32 gRenderOptionalFrameHookEnabled;
/// Scheduler event queue consumed by the runtime thread.
/// \note Original identifier: \c D_800AE4C0 (0x800AE4C0).
extern OSMesgQueue gRuntimeEventQueue;
/// Draw the frame’s right and bottom border and submit the finishing graphics task.
/// \returns Nothing; requires an active display list and initialized rendering resources.
void renderSubmitFrameBorder(void) {
    renderPrepareDisplayList();
    if (gRenderOptionalFrameHookEnabled != 0) {
        cameraApplyRenderingState(gRenderCameraIndex);
        renderOptionalFrameHook();
    }
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
    gDPSetCombineLERP(gRenderDisplayListCursor++, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    gDPSetRenderMode(gRenderDisplayListCursor++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetScissor(gRenderDisplayListCursor++, G_SC_NON_INTERLACE, 0, 0, 319, 239);
    gDPFillRectangle(gRenderDisplayListCursor++, 299, 20, 300, 220);
    gDPFillRectangle(gRenderDisplayListCursor++, 20, 219, 300, 220);
    graphicsTaskSubmit(1, 64, &gRenderDisplayListStart, &gRenderDisplayListCursor, &gRuntimeEventQueue);
}


/// Test whether a camera-space point lies within a configured viewing region.
/// \param r Viewing region with inclusive forward-depth limits, horizontal and vertical scale factors, and lateral margin.
/// \param point Three-component camera-space point in X, Y, Z order; forward depth is negative Z.
/// \returns One when the point satisfies the depth limits and lies strictly inside the lateral limits, or zero otherwise.
s32 cameraTestViewPoint(CameraViewRegion r, f32 *point) {
    f32 forwardLimit;
    f32 scaledY;
    f32 scaledX;

    forwardLimit = -point[2];
    if ((r.maximumDepth < forwardLimit) || (forwardLimit < r.minimumDepth)) {
        return 0;
    }
    scaledX = point[0] * r.scaleX;
    scaledY = point[1] * r.scaleY;
    forwardLimit = forwardLimit + r.margin;
    if ((scaledX < forwardLimit) && (-scaledX < forwardLimit) && (scaledY < forwardLimit) &&
        (-scaledY < forwardLimit)) {
        return 1;
    }
    return 0;
}

/// Convert a point from the active camera’s view space to world space.
/// \param point Three-component camera-space XYZ position, replaced with its world-space position; requires prepared active-camera transforms.
/// \returns Nothing.
void cameraViewPointToWorld(f32 *point) {
    CameraMatrix matrix;
    f32 *in = point;
    f32 *out = point;

    matrix = *(CameraMatrix *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 0x28);
    /* Build the view transform in its own block, like the transform
     * snapshots in actor_model_motion.c. */
    do {
        matrix33Transpose(&matrix);
        matrix.values[9] = *(f32 *)(gRenderCameras + gRenderCameraIndex * 0xE0);
        matrix.values[10] = *(f32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 4);
        matrix.values[11] = *(f32 *)(gRenderCameras + gRenderCameraIndex * 0xE0 + 8);
    } while (0);
    matrix34TransformPointVector(&matrix, in, out);
}

/* Start-up handshake for the controllerThreadMain thread: the thread sets ready
 * once it has consumed arg1 (controllerInitializeDevices) and arg2 (micSessionInitialize). */
typedef struct MainThreadArgs {
    /* 0x0 */ s32 ready;
    /* 0x4 */ s32 arg1;
    /* 0x8 */ s32 arg2;
} MainThreadArgs;

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

typedef struct {
    s32 state;
    u64 stopTime;
    u64 nextTime;
    u64 interval;
    u64 duration;
} ControllerTimer;
typedef struct {
    s32 state;
    u64 stopTime;
    s32 value;
} ControllerPulse;

typedef struct {
    void (*handlers[4])(s32);
} PortHandlers4;
typedef struct {
    void (*handlers[5])(s32);
} PortHandlers5;
/// Handlers for the rumble pattern lifecycle states.
/// \note Original identifier: \c D_800633F0 (0x800633F0).
extern PortHandlers4 gControllerRumblePatternHandlers;
/// Handlers for the rumble motor lifecycle states.
/// \note Original identifier: \c D_80063400 (0x80063400).
extern PortHandlers5 gControllerRumbleMotorHandlers;

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
