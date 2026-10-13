#include "controller_rumble.h"
#include "microphone_requests.h"
#include "text_window.h"
#include "message_bar.h"
#include "actor_assets.h"
#include "story_milestones.h"
#include "progress_storage.h"
#include "progress_fields.h"
#include "camera_overlay.h"
#include "graphics_tasks.h"
#include "scheduler.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "runtime_clock.h"
#include "motion_cache.h"
#include "audio_cue.h"
#include "player_control.h"
#include "text_draw.h"
#include "eeprom_io.h"
#include "text_panel.h"
#include "heap_diagnostics.h"
#include "runtime_program.h"
#include "scene_types.h"
#include "heap_types.h"
#include "player_types.h"
#include "controller_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/* Owned state. .data 0x80063830..0x80063890 holds the four u64 timestamps, the
 * save prompt string tables and (last) runtimeRunProgramEventLoop's static message pointer;
 * .bss 0x800AE4C0..0x800B67B0 is the unit's whole bss (0x82F0), ending with
 * runtimeRunProgramEventLoop's two statics and section alignment. Declared in address order,
 * which is IDO's layout order; `pad` lines are the unreferenced tails of the
 * ROM's symbols. IDO interleaves function statics with file-scope bss by
 * symbol numbering: the bss block comes first and the dead trace arguments
 * below reference their real globals, which numbers runtimeRunProgramEventLoop's statics
 * after D_800B6798 as in the ROM. */
/// Scheduler event queue consumed by the runtime thread.
/// \note Original identifier: \c D_800AE4C0 (0x800AE4C0).
OSMesgQueue gRuntimeEventQueue;
/// Runtime thread registration in the scheduler client list.
/// \note Original identifier: \c D_800AE4D8 (0x800AE4D8).
SchedulerClient gRuntimeSchedulerClient;
/// Borrowed scheduler queue address used to submit graphics tasks.
/// \note Original identifier: \c D_800AE4E0 (0x800AE4E0).
OSMesgQueue *gRuntimeGraphicsCommandQueue;
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
f32 gRuntimeFrameSeconds;
/// Elapsed OS clock ticks for the most recent runtime frame.
/// \note Original identifier: \c D_800AE4E8 (0x800AE4E8).
u64 gRuntimeFrameTicks;
/// Identifier of the currently active runtime program.
/// \note Original identifier: \c D_800AE4F0 (0x800AE4F0).
s32 gRuntimeCurrentProgram;
/// Entrance index used to initialize the player and companion in the current area.
/// \note Original identifier: \c D_800AE4F4 (0x800AE4F4).
s32 gRuntimeCurrentEntrance;
/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
s32 gRuntimeCurrentArea;
/// Entrance index requested for the next area load.
/// \note Original identifier: \c D_800AE4FC (0x800AE4FC).
s32 gRuntimeNextEntrance;
/// Area identifier requested for the next overlay load.
/// \note Original identifier: \c D_800AE500 (0x800AE500).
s32 gRuntimeNextArea;
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
s32 gRuntimeGameMode;
/// Elapsed runtime timestamp at the most recent game-mode change.
/// \note Original identifier: \c D_800AE508 (0x800AE508).
u32 gRuntimeGameModeStartMilliseconds;
/// Whether dismissing the post-save prompt resets the player profile.
/// \note Original identifier: \c D_800AE50C (0x800AE50C).
s32 gRuntimeResetProfileAfterSave;
/// Save request parameters; the first word selects the EEPROM save record.
/// \note Original identifier: \c D_800AE510 (0x800AE510).
u8 gRuntimeSaveParameters[0x8];
/// Persistent scheduler message slots followed by the runtime thread stack.
/// \note Original identifier: \c D_800AE518 (0x800AE518).
u8 gRuntimeThreadStorage[0x8020]; /* thread stack; its end is D_800B6538 */
/// Runtime main thread control block.
/// \note Original identifier: \c D_800B6538 (0x800B6538).
OSThread gRuntimeThread;
/// Elapsed-runtime checkpoint for play time already credited to progress.
/// \note Original identifier: \c D_800B66E8 (0x800B66E8).
u64 gRuntimePlayTimeCheckpointSeconds;
/// Program identifier requested for the next runtime lifecycle.
/// \note Original identifier: \c D_800B66F0 (0x800B66F0).
s32 gRuntimeNextProgram;
/// Number of submitted runtime graphics tasks awaiting completion.
/// \note Original identifier: \c D_800B66F4 (0x800B66F4).
s32 gRuntimePendingGraphicsTasks;
/// Whether the current program has completed its initial update before rendering starts.
/// \note Original identifier: \c D_800B66F8 (0x800B66F8).
s32 gRuntimeRenderPrimed;
/// Retained padding word preceding the eight-byte-aligned frame-duration sample array.
/// \note Original identifier: \c D_800B66FC (0x800B66FC).
s32 gRuntimeFrameTickAlignmentPadding;
/// Ten recent frame-duration samples in OS clock ticks.
/// \note Original identifier: \c D_800B6700 (0x800B6700).
u64 gRuntimeFrameTickSamples[10];
/// Next slot in the rolling frame-duration sample window.
/// \note Original identifier: \c D_800B6750 (0x800B6750).
s32 gRuntimeNextFrameSampleIndex;
/// Number of valid frame-duration samples, capped at ten.
/// \note Original identifier: \c D_800B6754 (0x800B6754).
s32 gRuntimeFrameSampleCount;
/// Total OS clock ticks across the valid frame-duration samples.
/// \note Original identifier: \c D_800B6758 (0x800B6758).
u64 gRuntimeFrameSampleTickTotal;
/// OS clock timestamp used to measure the next frame duration.
/// \note Original identifier: \c D_800B6760 (0x800B6760).
u64 gRuntimePreviousFrameTicks;
/// Elapsed seconds since the shared UI pulse timer was restarted.
/// \note Original identifier: \c D_800B6768 (0x800B6768).
f32 gRuntimeUiPulseSeconds;
/// Whether the runtime event loop should leave after outstanding graphics tasks finish.
/// \note Original identifier: \c D_800B676C (0x800B676C).
s32 gRuntimeProgramChangeRequested;
/// Whether the current game mode still needs its one-time entry work.
/// \note Original identifier: \c D_800B6770 (0x800B6770).
s32 gRuntimeGameModeEntryPending;
/// Retained unused word between game-mode entry state and prompt-panel storage.
/// \note Original identifier: \c D_800B6774 (0x800B6774).
s32 gRuntimePromptPanelReservedPadding;
/// Runtime prompt panel base and retained line count; adjacent words retain the TextPanel layout.
/// \note Typed panel views rely on the target linker layout across these separate symbols.
/// \note Original identifier: \c D_800B6778 (0x800B6778).
s32 gRuntimePromptPanelLineCount;
/// Runtime prompt panel circular replacement index.
/// \note Original identifier: \c D_800B677C (0x800B677C).
s32 gRuntimePromptPanelNextEntry; /* D_800B6778's record tail, one word per line: IDO 8-aligns arrays */
/// Runtime prompt panel packed horizontal and vertical screen offsets.
/// \note Original identifier: \c D_800B6780 (0x800B6780).
s32 gRuntimePromptPanelPosition;
/// Runtime prompt panel per-line color table address.
/// \note Original identifier: \c D_800B6784 (0x800B6784).
s32 gRuntimePromptPanelColors;
/// Runtime prompt panel entry capacity.
/// \note Original identifier: \c D_800B6788 (0x800B6788).
s32 gRuntimePromptPanelCapacity;
/// Runtime prompt panel display-order image table address; zero suppresses panel drawing.
/// \note Original identifier: \c D_800B678C (0x800B678C).
s32 gRuntimePromptPanelImages;
/// Runtime prompt panel circular image-entry table address.
/// \note Original identifier: \c D_800B6790 (0x800B6790).
s32 gRuntimePromptPanelEntries; /* D_800B678C's record tail */
/// Runtime prompt panel borrowed text configuration address.
/// \note Original identifier: \c D_800B6794 (0x800B6794).
s32 gRuntimePromptPanelConfiguration;
/// Controller buttons accepted to dismiss the save-result prompt.
/// \note Original identifier: \c D_800B6798 (0x800B6798).
u16 gRuntimeSaveAcknowledgeButtons; /* save-result acknowledgement button mask */

/// Elapsed runtime milliseconds since timing reset.
/// \note Original identifier: \c D_80063830 (0x80063830).
u64 gRuntimeElapsedMilliseconds = 0;
/// Elapsed whole runtime seconds since timing reset.
/// \note Original identifier: \c D_80063838 (0x80063838).
u64 gRuntimeElapsedSeconds = 0;
/// OS time in microseconds at the most recent millisecond-clock update.
/// \note Original identifier: \c D_80063840 (0x80063840).
u64 gRuntimeMillisecondCheckpointMicroseconds = 0;
/// Elapsed millisecond checkpoint for the whole-second runtime clock.
/// \note Original identifier: \c D_80063848 (0x80063848).
u64 gRuntimeSecondCheckpointMilliseconds = 0;
/// Whether the runtime program event loop is active; reset checkpoints defer shutdown while set.
/// \note Original identifier: \c D_80063850 (0x80063850).
s32 gRuntimeProgramLoopActive = 0;
/* Save prompt pages: three contiguous line tables, each walked by pointer up to
 * the next table's base, so declaration adjacency is the page boundary. */
/// Save prompt line identifying the Game Pak as the destination.
/// \note Original identifier: \c D_800A0560 (0x800A0560).
const char gSavePromptDestinationText[] = "Data will be saved to the Game Pak.";
/// Save prompt line warning against turning the power off.
/// \note Original identifier: \c D_800A0584 (0x800A0584).
const char gSavePromptPowerWarningText[] = "Do not turn the power OFF until";
/// Save prompt line completing the warning to wait for completion.
/// \note Original identifier: \c D_800A05A4 (0x800A05A4).
const char gSavePromptCompletionWarningText[] = "the save is complete.";
/// Blank separator line in the save confirmation prompt.
/// \note Original identifier: \c D_800A05BC (0x800A05BC).
const char gSavePromptBlankText[] = " ";
/// Save prompt line showing the confirmation button symbols.
/// \note Original identifier: \c D_800A05C0 (0x800A05C0).
const char gSavePromptConfirmText[] = "Press @I@J to save.";
/// Save result line confirming successful storage.
/// \note Original identifier: \c D_800A05D4 (0x800A05D4).
const char gSaveSuccessConfirmationText[] = "Your game was saved.";
/// Save result line permitting power-off after completion.
/// \note Original identifier: \c D_800A05EC (0x800A05EC).
const char gSaveSuccessPowerOffText[] = "You can now turn the power OFF.";
/// Blank separator line in the successful-save prompt.
/// \note Original identifier: \c D_800A060C (0x800A060C).
const char gSaveSuccessBlankText[] = " ";
/// Successful-save prompt line showing the continue button symbols.
/// \note Original identifier: \c D_800A0610 (0x800A0610).
const char gSaveSuccessContinueText[] = "Press @I@J to continue.";
/// Save failure heading with text-color controls.
/// \note Original identifier: \c D_800A0628 (0x800A0628).
const char gSaveFailureHeadingText[] = "^cUnable to save data.^^";
/// Save failure line instructing the player to turn the power off.
/// \note Original identifier: \c D_800A0644 (0x800A0644).
const char gSaveFailurePowerOffText[] = "Please turn the power OFF and";
/// Save failure line asking the player to check the Game Pak.
/// \note Original identifier: \c D_800A0664 (0x800A0664).
const char gSaveFailureCheckGamePakText[] = "confirm that the Game Pak is";
/// Save failure line completing the Game Pak insertion instruction.
/// \note Original identifier: \c D_800A0684 (0x800A0684).
const char gSaveFailureInsertionText[] = "inserted correctly.";
/// Save failure prompt line showing the continue button symbols.
/// \note Original identifier: \c D_800A0698 (0x800A0698).
const char gSaveFailureContinueText[] = "Press @U@V to continue.";
/// Ordered lines of the save confirmation prompt; ends at the next prompt table.
/// \note Original identifier: \c D_80063854 (0x80063854).
const char *gSaveConfirmationPromptLines[] = {gSavePromptDestinationText, gSavePromptPowerWarningText,
                                              gSavePromptCompletionWarningText, gSavePromptBlankText,
                                              gSavePromptConfirmText};
/// Ordered lines of the successful-save prompt; ends at the failure prompt table.
/// \note Original identifier: \c D_80063868 (0x80063868).
const char *gSaveSuccessPromptLines[] = {gSaveSuccessConfirmationText, gSaveSuccessPowerOffText, gSaveSuccessBlankText,
                                         gSaveSuccessContinueText};
/// Ordered lines of the failed-save prompt.
/// \note Original identifier: \c D_80063878 (0x80063878).
const char *gSaveFailurePromptLines[] = {gSaveFailureHeadingText, gSaveFailurePowerOffText,
                                         gSaveFailureCheckGamePakText, gSaveFailureInsertionText,
                                         gSaveFailureContinueText};

void actionScriptInitializeHook();        /* extern */
void hudInitialize();        /* extern */
void randomSeedFromSystem();        /* extern */
void runtimeAccumulatePlayTime(void);
void runtimeShutdownAndHalt(void);
void runtimeRequestGameModeChange(s32);
void runtimeCheckResetCheckpoint(const char *, s32);
void runtimeMainThreadEntry(void *);
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
s32 runtimeGetUiPulseAlpha(void);

f32 mathSinDegrees(f32);

void runtimeResetTiming(void);

/// Provide the inactive operating-system and hardware information report.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000A7A0 (0x8000A7A0).
void runtimeOsInfoReportNoOp(void) {
    TRACE("\n---------------- OS info ----------------\n");
    TRACE("OS   Version %4.1f%1s\n", osVersion);
    TRACE("RCP  Version 1.0\n");
    TRACE("RCP  Version 2.0\n");
    TRACE("IO  Version %d\n");
    TRACE("RAC Version %d\n");
    TRACE("RDP Version %d\n");
    TRACE("RSP Version %d\n");
    TRACE("PIF  Version %d\n", osCicId);
    TRACE("DRAM size 0x%x bytes\n", osMemSize);
    TRACE("ROM base address of the game image is 0x%08x\n", osRomBase);
    TRACE("UCODE Version is %s\n");
    TRACE("System Clock Ratio = ", osClockRate);
    TRACE("1:1\n");
    TRACE("3:2\n");
    TRACE("2:1\n");
    TRACE("3:1\n");
    TRACE("illegal value\n");
    TRACE("Video Mode is ");
    TRACE("PAL\n");
    TRACE("NTSC\n");
    TRACE("MPAL\n");
    TRACE("unknown(%d)\n", osTvType);
    TRACE("ROM type is ");
    TRACE("cartridge\n");
    TRACE("bulk media\n");
    TRACE("unknown(%d)\n", osRomType);
    TRACE("Reset type is ");
    TRACE("cold reset\n");
    TRACE("NMI\n");
    TRACE("unknown(%d)\n", osResetType);
    TRACE("EEPROM type is ");
    TRACE("none\n");
    TRACE("4K\n");
    TRACE("16K\n");
    TRACE("unknown(%d)\n", osEepromProbe(&gRuntimeEventQueue));
    TRACE("\n");
}

void controllerStartThread(s32, s32, s32);
void controllerSetRepeatTiming(f32 initialDelaySeconds, f32 repeatIntervalSeconds);
void controllerSetRepeatMask(u32 buttonMask);
void audioInitialize(s32, s32, s32, s32);
void playerResetProfile(void);
void toolboxReset(void);
void eyeCatchInitialize(void);
void videoConfigureMode(OSViMode, s32, s32, s32);
void faultStartHandler(s32, s32);
/// Canonical SDK video-interface mode table indexed by runtime and scheduler mode selection.
/// \note Original identifier: \c D_8009DCB0 (0x8009DCB0).
extern OSViMode osViModeTable[];

/// Current area scenario index shared by actor scripts, scene population, cards and collection titles.
/// \note Original identifier: \c D_800F0B70 (0x800F0B70).
extern s32 gAreaScenarioIndex;
/// Initialize platform services and default runtime state for game execution.
/// \param schedulerAddress Address of a scheduler context that remains valid throughout runtime execution.
/// \returns Nothing on NTSC systems; halts on other television standards.
/// \note Original decompilation identifier: \c func_8000A7A8 (0x8000A7A8).
void runtimeInitializePlatform(s32 schedulerAddress) {
    s32 mode;
    gRuntimeCurrentProgram = 0;
    gRuntimeNextProgram = 0;
    gRuntimeGameMode = 0x12;
    gRuntimeCurrentArea = gRuntimeNextArea = 0x16;
    gRuntimeCurrentEntrance = gRuntimeNextEntrance = 0;
    gAreaScenarioIndex = 1;
    faultStartHandler(110, 110);
    switch (osTvType) {
    case 0:
        mode = 14;
        break;
    case 2:
        mode = 28;
        break;
    default:
    case 1:
        mode = 0;
        break;
    }
    schedulerInitialize((GameScheduler *)schedulerAddress, 100, 100, mode, 1);
    audioInitialize(schedulerAddress, 50, 50, 0x7D00);
    videoConfigureMode(osViModeTable[mode], 320, 240, 16);
    osViSetMode(&osViModeTable[mode]);
    osViBlack(1);
    if (osTvType != 1) {
        for (;;) {
        }
    }
    osViSetSpecialFeatures(0x40);
    osViSetSpecialFeatures(8);
    osViSetSpecialFeatures(2);
    controllerStartThread(schedulerAddress, 4, 8);
    controllerSetRepeatMask(0xF0F);
    controllerSetRepeatTiming(0.3f, 0.05f);
    playerResetProfile();
    toolboxReset();
    _bzero(&gRuntimePromptPanelLineCount, 0x20);
    eyeCatchInitialize();
    *(s32 *)gRuntimeSaveParameters = 1;
}

void renderLoadAreaAssets();                          /* extern */
void objectSystemInitialize();                          /* extern */
void presentationInitializeForCurrentArea();                          /* extern */
void toolboxCreateDisplayObjects();                          /* extern */
void backdropAllocateViewBuffer(s32, u32, u32); /* extern */
void backdropSelectForView(s32, s32, s32);     /* extern */
void mazeInitialize();                          /* extern */
void mazeRunAreaUpdateCallback();                          /* extern */
void areaActivateDestination();                          /* extern */
void playerInitializeForArea();                          /* extern */
void pikachuSpawnForArea();                          /* extern */
void itemInitializeRegistry();                          /* extern */
void systemMenuInitialize();                          /* extern */
/// Whether gameplay exposes microphone interaction and its HUD indicator.
/// \note Original identifier: \c D_800722D0 (0x800722D0).
extern s32 gGameplayMicrophoneAvailable;
/// Whether gameplay permits recognized voice commands to launch messenger effects.
/// \note Original identifier: \c D_800722D4 (0x800722D4).
extern s32 gGameplayVoiceCommandsAllowed;
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Initialize the current scene's gameplay systems and voice interaction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000A9C0 (0x8000A9C0).
void runtimeInitializeSceneGameplay(void) {
    areaActivateDestination();
    if (gRuntimeGameMode != 0x12) {
        runtimeRequestGameModeChange(4);
    }
    presentationInitializeForCurrentArea();
    if (progressHasFieldInCurrentArea(0x40000000) != 0) {
        progressAddValue(0x40000000, 1);
    }
    backdropAllocateViewBuffer(0, 0x118, 0xC8);
    backdropSelectForView(0, ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->backdrop, 1);
    runtimeCheckResetCheckpoint("pre-loadAreaDlist", 1);
    renderLoadAreaAssets();
    heapDebugReportFreeSpace(&gGameHeap, "after loadAreaDlist");
    runtimeCheckResetCheckpoint("aft-loadAreaDlist", 0);
    itemInitializeRegistry();
    heapDebugReportFreeSpace(&gGameHeap, "after initItemSystem");
    objectSystemInitialize();
    gPikachuActor = 0;
    heapDebugReportFreeSpace(&gGameHeap, "after initObjects()");
    runtimeCheckResetCheckpoint("pre-initMaze", 1);
    mazeInitialize();
    heapDebugReportFreeSpace(&gGameHeap, "after initMaze()");
    runtimeCheckResetCheckpoint("aft-initMaze", 1);
    playerInitializeForArea();
    heapDebugReportFreeSpace(&gGameHeap, "after initPlayerParameters()");
    pikachuSpawnForArea();
    heapDebugReportFreeSpace(&gGameHeap, "after initPikachu()");
    systemMenuInitialize();
    runtimeCheckResetCheckpoint("aft-initSystemMenu", 1);
    toolboxCreateDisplayObjects();
    heapDebugReportFreeSpace(&gGameHeap, "after createItemBox()");
    runtimeCheckResetCheckpoint("aft-createItemBox", 1);
    micRequestSessionEnabled(1);
    micRequestTriggerMode(0, 0);
    micRequestTriggerButtons(1, 0x2020);
    micRequestRecognitionOverride(0);
    micRequestRecognitionFollowsTrigger(0);
    gGameplayMicrophoneAvailable = 0;
    gGameplayVoiceCommandsAllowed = 0;
    micRequestDictionary(0);
    textWindowInitialize();
    heapDebugReportFreeSpace(&gGameHeap, "after initMessageMode()");
    runtimeCheckResetCheckpoint("pre-areaInit", 1);
    mazeRunAreaUpdateCallback();
    runtimeCheckResetCheckpoint("aft-areaInit", 1);
}

/// Initialize the selected runtime program and reset its execution state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000AC04 (0x8000AC04).
void runtimeInitializeCurrentProgram(void) {
    void (*initializeProgram)(void);

    gRuntimePendingGraphicsTasks = 0;
    gRuntimeProgramChangeRequested = 0;
    gRuntimeRenderPrimed = 0;
    heapDebugReportFreeSpace(&gGameHeap, "entry_initGame()");
    runtimeResetTiming();
    controllerRequestRumbleReset(0);
    initializeProgram = gRuntimeProgramInitializeEntries[gRuntimeCurrentProgram].callback;
    if (initializeProgram != NULL) {
        initializeProgram();
    }
    micRequestTriggerMode(0, 0);
    hudInitialize();
    randomSeedFromSystem();
    actionScriptInitializeHook();
    heapDebugReportFreeSpace(&gGameHeap, "finish_initGame()");
}

/// Reset elapsed runtime clocks and frame-duration tracking.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000ACB8 (0x8000ACB8).
void runtimeResetTiming(void) {
    gRuntimeNextFrameSampleIndex = 0;
    gRuntimeFrameSampleCount = 0;
    gRuntimeFrameSampleTickTotal = 0;
    gRuntimeFrameSeconds = 0.01666666753590107f;
    gRuntimeElapsedMilliseconds = 0;
    gRuntimeElapsedSeconds = 0;
    gRuntimeMillisecondCheckpointMicroseconds = osGetTime() * 64 / 3000;
    gRuntimeSecondCheckpointMilliseconds = gRuntimeElapsedMilliseconds;
    gRuntimePlayTimeCheckpointSeconds = gRuntimeElapsedSeconds;
    _bzero(gRuntimeFrameTickSamples, 0x50);
    gRuntimePreviousFrameTicks = osGetTime();
}

/// Update the recent frame-duration estimate from a new timing sample.
/// \pre Runtime timing must have been reset before the first sample.
/// \param delta Elapsed system clock ticks for the latest frame.
/// \returns Smoothed frame duration in seconds, limited to 0.001 through 0.1.
/// \note Original decompilation identifier: \c func_8000ADBC (0x8000ADBC).
f32 runtimeSmoothFrameDuration(u64 delta) {
    f32 result;
    f32 minimum;
    gRuntimeFrameSampleTickTotal += delta - gRuntimeFrameTickSamples[gRuntimeNextFrameSampleIndex];
    gRuntimeFrameTickSamples[gRuntimeNextFrameSampleIndex] = delta;
    gRuntimeNextFrameSampleIndex++;
    if (gRuntimeNextFrameSampleIndex >= 10)
        gRuntimeNextFrameSampleIndex = 0;
    if (gRuntimeFrameSampleCount < 10)
        gRuntimeFrameSampleCount++;
    result = (f32)(gRuntimeFrameSampleTickTotal * 64 / 3000);
    minimum = 0.001f;
    result = result / 1000000.0f / (f32)gRuntimeFrameSampleCount;
    if (result < minimum)
        result = minimum;
    else if (0.1f < result)
        result = 0.1f;
    return result;
}

/// Advance runtime clocks, frame timing, and the shared UI pulse timer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000AF08 (0x8000AF08).
void runtimeUpdateTiming(void) {
    u64 nowUs;
    u64 ticks;
    ticks = osGetTime();
    gRuntimeFrameTicks = ticks - gRuntimePreviousFrameTicks;
    gRuntimePreviousFrameTicks = ticks;
    gRuntimeFrameSeconds = runtimeSmoothFrameDuration(gRuntimeFrameTicks);
    gRuntimeUiPulseSeconds += gRuntimeFrameSeconds;
    nowUs = ticks * 64 / 3000;
    if (nowUs - gRuntimeMillisecondCheckpointMicroseconds > 1000) {
        gRuntimeElapsedMilliseconds += (nowUs - gRuntimeMillisecondCheckpointMicroseconds) / 1000;
        gRuntimeMillisecondCheckpointMicroseconds = nowUs;
    }
    if (gRuntimeElapsedMilliseconds - gRuntimeSecondCheckpointMilliseconds > 1000) {
        gRuntimeElapsedSeconds += (gRuntimeElapsedMilliseconds - gRuntimeSecondCheckpointMilliseconds) / 1000;
        gRuntimeSecondCheckpointMilliseconds = gRuntimeElapsedMilliseconds;
    }
}

/// Read elapsed runtime time since the timing clocks were reset.
/// \returns Elapsed milliseconds.
/// \note Original decompilation identifier: \c func_8000B0FC (0x8000B0FC).
u64 runtimeGetElapsedMilliseconds(void) {
    return gRuntimeElapsedMilliseconds;
}

/// Read elapsed runtime time since the timing clocks were reset.
/// \returns Elapsed whole seconds.
/// \note Original decompilation identifier: \c func_8000B110 (0x8000B110).
u64 runtimeGetElapsedSeconds(void) {
    return gRuntimeElapsedSeconds;
}

/// Restart the shared UI pulse timer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B124 (0x8000B124).
void runtimeResetUiPulse(void) {
    gRuntimeUiPulseSeconds = 0.0f;
}

/// Get the current opacity of the shared repeating UI pulse.
/// \pre Pulse age in milliseconds must be representable as a signed 32-bit integer.
/// \returns Opacity from zero through 255.
/// \note Original decompilation identifier: \c func_8000B134 (0x8000B134).
s32 runtimeGetUiPulseAlpha(void) {
    s32 var_v1;
    s32 tmp;

    tmp = (mathSinDegrees((f32)((s32)(gRuntimeUiPulseSeconds * 1000.0f) % 1500) * 0.24f) * 127.0f) + 128.0f;
    var_v1 = tmp;
    if (tmp < 0) {
        var_v1 = 0;
    } else if (tmp >= 0x100) {
        var_v1 = 0xFF;
    }
    return var_v1 & 0xFF;
}


/// Request a runtime program transition and reset transient input and recognition state.
/// \param program Valid identifier in the runtime program table.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B1D8 (0x8000B1D8).
void runtimeRequestProgramChange(s32 program) {
    TRACE("\nchange program to %d\n\n", program);
    gRuntimeNextProgram = program;
    micRequestRecognitionOverride(0);
    micRequestRecognitionFollowsTrigger(0);
    gGameplayMicrophoneAvailable = 0;
    gGameplayVoiceCommandsAllowed = 0;
    gRuntimeProgramChangeRequested = 1;
    gRuntimeRenderPrimed = 0;
    controllerRequestRumbleReset(0);
}

/// Provide an inactive input-service hook for secondary players.
/// \param unused Address of the secondary player input state, unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B234 (0x8000B234).
void runtimeSecondaryPlayerInputNoOp(void *unused) {
}

/// Add newly elapsed play time to the recorded play-time total.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B23C (0x8000B23C).
void runtimeAccumulatePlayTime(void) {
    TRACE("total game play time = %d\n", gRuntimeElapsedSeconds - gRuntimePlayTimeCheckpointSeconds);
    progressAddValue(4, gRuntimeElapsedSeconds - gRuntimePlayTimeCheckpointSeconds);
    gRuntimePlayTimeCheckpointSeconds = gRuntimeElapsedSeconds;
}

/// Add the active actor's total script time to the recorded total without resetting its timer.
/// \pre An active actor with initialized script state is required; its elapsed whole seconds must be representable as a signed integer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B288 (0x8000B288).
void runtimeAccumulateActorScriptTime(void) {
    u8 *actorState = *(u8 **)(gPikachuActor + 0x54);
    progressAddValue(5, (s32) * (f32 *)(*(u8 **)(actorState + 0x1B8) + 0x10));
}

/// Request a game-mode transition and account for elapsed play time.
/// \param mode Valid game-mode identifier to enter.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B2C8 (0x8000B2C8).
void runtimeRequestGameModeChange(s32 mode) {
    TRACE("game mode changed to %d\n", mode);
    TRACE("total system play time = %d\n", gRuntimeElapsedSeconds);
    gRuntimeGameMode = mode;
    gRuntimeGameModeEntryPending = 1;
    gRuntimeGameModeStartMilliseconds = (u32)runtimeGetElapsedMilliseconds();
    gRuntimeRenderPrimed = 0;
    runtimeAccumulatePlayTime();
}

void objectUpdateGroup(s32);                     /* extern */
void objectSweepDeferredHierarchyDestruction();                            /* extern */
void audioStopCategories(s32, f64);
void hudPlayerUpdateNoOp(s32);                /* extern */
void presentationUpdateCardMode();                   /* extern */
void presentationUpdateCollectionResultsMode();                   /* extern */
void wholeMapUpdate();                   /* extern */
void toolboxHandleInput();                   /* extern */
void toolboxUpdateDisplay();                   /* extern */
void titleScreenUpdate();                   /* extern */
void eyeCatchBegin();                   /* extern */
s32 eyeCatchUpdate();                       /* extern */
void areaSetNextDestination(s32, s32);   /* extern */
void playerUpdateInput(s32);                /* extern */
void pikachuUpdateScriptAndTriggers();                   /* extern */
void systemMenuHandleInput();                   /* extern */
void systemMenuUpdateAnimation();                   /* extern */
/// Save-control byte transferred independently from the progress records.
/// \note Original identifier: \c D_8009A6F0 (0x8009A6F0).
extern u8 gEepromSaveControl;
/// Selected eye-catch group index, or minus one when no interlude is selected.
/// \note Original identifier: \c D_8009B580 (0x8009B580).
extern s32 gEyeCatchSelectedGroup;
/// Remaining duration of the primary camera secondary overlay transition.
/// \note Original identifier: \c D_800ABDCC (0x800ABDCC).
extern f32 gPrimarySecondaryOverlayRemainingSeconds;
/// Processed button and stick samples for all controller ports; scalar declarations view the primary held-button word.
/// \note Original identifier: \c D_800AD540 (0x800AD540).
extern u32 gControllerInputState[];
/// Second controller processed sample; also the start of the secondary-player input traversal.
/// \note Original identifier: \c D_800AD558 (0x800AD558).
extern ControllerState gControllerSecondPortInputState;
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;
/// Pending completed-visit card and progress update, consumed by area-zero card selection.
/// \note Original identifier: \c D_800F0B74 (0x800F0B74).
extern s32 gCompletedVisitCardPending;

/// Advance the current game mode, its transitions and save prompts, and player input servicing.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B30C (0x8000B30C).
void runtimeUpdateGameMode(void) {
    const char **savePrompt;
    const char **savedPrompt;
    s32 i;
    s32 status;
    s32 player;
    u8 *pad;

    objectSweepDeferredHierarchyDestruction();
    switch (gRuntimeGameMode) {
    case 0:
        player = 0;
        if (gEyeCatchSelectedGroup != -1) {
            eyeCatchBegin();
            runtimeRequestGameModeChange(0xD);
        } else {
            do {
                if (player <= 0) {
                    playerUpdateInput(player);
                }
                player += 1;
            } while (player != 3);
            if ((gRuntimeGameMode == 0) || (gRuntimeGameMode == 3)) {
                pikachuUpdateScriptAndTriggers();
                objectUpdateGroup(0);
            }
        }
        break;
    case 18:
        titleScreenUpdate();
        break;
    case 4:
        presentationUpdateCardMode();
        break;
    case 5:
        presentationUpdateCollectionResultsMode();
        break;
    case 11:
        if (gRuntimeGameModeEntryPending != 0) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.8f);
            runtimeAccumulateActorScriptTime();
            gRuntimeGameModeEntryPending = 0;
        } else if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            runtimeRequestProgramChange(0);
        }
        objectUpdateGroup(0);
        break;
    case 12:
        if (gRuntimeGameModeEntryPending != 0) {
            gCompletedVisitCardPending = 1;
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.8f);
            runtimeAccumulateActorScriptTime();
            gRuntimeGameModeEntryPending = 0;
        } else if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            runtimeRequestGameModeChange(5);
        }
        objectUpdateGroup(0);
        break;
    case 6:
        if (gRuntimeGameModeEntryPending != 0) {
            audioStopCategories(3, 0.10000000149011612);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.8f);
            runtimeAccumulateActorScriptTime();
            controllerRequestRumbleReset(0);
            gRuntimeGameModeEntryPending = 0;
        } else if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            audioStopCategories(3, 0.0);
            runtimeRequestGameModeChange(8);
        }
        objectUpdateGroup(0);
        break;
    case 13:
        if (eyeCatchUpdate() != 0) {
            runtimeRequestGameModeChange(0);
        }
        break;
    case 7:
        if (gRuntimeGameModeEntryPending != 0) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0x1E, 0.8f);
            runtimeAccumulateActorScriptTime();
            controllerRequestRumbleReset(0);
            gRuntimeGameModeEntryPending = 0;
        } else if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            runtimeRequestGameModeChange(0xE);
        }
        objectUpdateGroup(0);
        break;
    case 14:
        if (gRuntimeGameModeEntryPending != 0) {
            textPanelInitialize((TextPanel *)&gRuntimePromptPanelLineCount, 5);
            textPanelSetTextConfiguration((TextPanel *)&gRuntimePromptPanelLineCount, 0);
            textPanelSetPosition((TextPanel *)&gRuntimePromptPanelLineCount, 0, 0x64);
            savePrompt = gSaveConfirmationPromptLines;
            do {
                textPanelAppend((TextPanel *)&gRuntimePromptPanelLineCount, (void *)*savePrompt);
                savePrompt++;
            } while (savePrompt != gSaveSuccessPromptLines);
            runtimeResetUiPulse();
            gRuntimeGameModeEntryPending = 0;
        } else if (gControllerInputState[1] & 0x8000) {
            textPanelReleaseEntries((TextPanel *)&gRuntimePromptPanelLineCount);
            gRuntimePromptPanelImages = 0;
            runtimeRequestGameModeChange(0x10);
        }
        objectUpdateGroup(0);
        break;
    case 16:
        if (gRuntimeGameModeEntryPending != 0) {
            switch (*(s32 *)gRuntimeSaveParameters) {
            case 1:
                gEepromSaveControl |= 1;
                break;
            case 2:
                gEepromSaveControl |= 2;
                break;
            default:
                TRACE("Illegal saveFile:%d\n", *(s32 *)gRuntimeSaveParameters);
                break;
            }
            progressSaveControlDataToEeprom();
            gRuntimeGameModeEntryPending = 0;
        } else if (eepromIsRequestOrTransferBusy() != 0) {
            runtimeRequestGameModeChange(0xF);
        }
        objectUpdateGroup(0);
        break;
    case 15:
        if (gRuntimeGameModeEntryPending != 0) {
            playerPlayTrackedSoundEffect((u8 *)&gPlayerState, 0x86);
            progressSaveRecordToEeprom(*(s32 *)gRuntimeSaveParameters);
            gRuntimeGameModeEntryPending = 0;
            textPanelInitialize((TextPanel *)&gRuntimePromptPanelLineCount, 5);
            textPanelSetTextConfiguration((TextPanel *)&gRuntimePromptPanelLineCount, 0);
            textPanelSetPosition((TextPanel *)&gRuntimePromptPanelLineCount, 0, 0x64);
        } else {
            status = eepromGetQueuedTransferState();
            switch (status) {
            case 2:
                break;
            case 0:
                savedPrompt = gSaveSuccessPromptLines;
                do {
                    textPanelAppend((TextPanel *)&gRuntimePromptPanelLineCount, (void *)*savedPrompt);
                    savedPrompt++;
                } while (savedPrompt != gSaveFailurePromptLines);
                runtimeResetUiPulse();
                gRuntimeSaveAcknowledgeButtons = 0x8000;
                runtimeRequestGameModeChange(0x11);
                break;
            case 5:
                for (i = 0; i < 5; i++) {
                    textPanelAppend((TextPanel *)&gRuntimePromptPanelLineCount, (void *)gSaveFailurePromptLines[i]);
                }
                runtimeResetUiPulse();
                gRuntimeSaveAcknowledgeButtons = 0x1000;
                runtimeRequestGameModeChange(0x11);
                break;
            default:
                TRACE("Illegal saveState:%d\n", status);
                break;
            }
        }
        objectUpdateGroup(0);
        break;
    case 17:
        if (gRuntimeSaveAcknowledgeButtons & gControllerInputState[1]) {
            audioStopCategories(3, 0.10000000149011612);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.0f);
            textPanelReleaseEntries((TextPanel *)&gRuntimePromptPanelLineCount);
            gRuntimePromptPanelImages = 0;
            if (gRuntimeResetProfileAfterSave != 0) {
                areaSetNextDestination(0x16, 0);
                playerResetProfile();
                progressSetChapterAndUnlockMilestones(0);
                runtimeRequestGameModeChange(0x12);
                runtimeRequestProgramChange(0);
            } else {
                areaSetNextDestination(0, 0);
                runtimeRequestProgramChange(0);
            }
        }
        objectUpdateGroup(0);
        break;
    case 1:
        toolboxHandleInput();
        toolboxUpdateDisplay();
        break;
    case 2:
        systemMenuHandleInput();
        systemMenuUpdateAnimation();
        break;
    case 3:
        textWindowUpdate();
        break;
    case 8:
        wholeMapUpdate();
        break;
    default:
        TRACE("Unknown game mode(%d).\n", gRuntimeGameMode);
        break;
    }
    pad = (u8 *)&gControllerSecondPortInputState;
    player = 1;
    do {
        hudPlayerUpdateNoOp(player);
        runtimeSecondaryPlayerInputNoOp(pad);
        player += 1;
        pad += 0x18;
    } while (player != 3);
}

void audioShutdown(void);
s32 rspTryResetProgramCounter(void);
/// Stop runtime activity, wait for outstanding work, and halt for system reset.
/// \returns Does not return.
/// \note Original decompilation identifier: \c func_8000BA1C (0x8000BA1C).
void runtimeShutdownAndHalt(void) {
    s16 *message = NULL;
    osViSetYScale(1.0f);
    controllerRequestRumbleReset(0);
    audioShutdown();
    if (gRuntimePendingGraphicsTasks != 0) {
        do {
            osRecvMesg(&gRuntimeEventQueue, (OSMesg *)&message, OS_MESG_BLOCK);
            switch (*message) {
            case 1:
            case 3:
            case 4:
                break;
            case 2:
                gRuntimePendingGraphicsTasks--;
                break;
            }
        } while (gRuntimePendingGraphicsTasks != 0);
    }
    for (;;) {
        if (rspTryResetProgramCounter() == 0)
            break;
        osRecvMesg(&gRuntimeEventQueue, (OSMesg *)&message, OS_MESG_BLOCK);
    }
    for (;;) {
    }
}

/// First color framebuffer storage used for drawing and presentation.
/// \note Original identifier: \c D_803B5000 (0x803B5000).
extern u8 gColorFramebuffer0[]; /* first color framebuffer */
/// Second color framebuffer storage used for drawing and presentation.
/// \note Original identifier: \c D_803DA800 (0x803DA800).
extern u8 gColorFramebuffer1[]; /* color framebuffer */

/// Honor a pending system reset when runtime execution permits shutdown.
/// \param label Diagnostic checkpoint label; unused in this build.
/// \param unused Unused diagnostic argument retained by the interface.
/// \returns Nothing; halts if a pending reset can be honored.
/// \note Original decompilation identifier: \c func_8000BB48 (0x8000BB48).
void runtimeCheckResetCheckpoint(const char *label, s32 unused) {
    TRACE("%16s: %p %p %x\n", "zbuf", gColorFramebuffer0);
    TRACE("%16s: %p %p %x\n", "cfb", gColorFramebuffer1);
    if ((gRuntimeProgramLoopActive == 0) && (schedulerIsResetPending() != 0)) {
        runtimeShutdownAndHalt();
    }
}

void controllerUpdateInputState(void);
#include "voice_commands.h"
void renderBeginFrame(void);
void renderSubmitFrameBorder(void);
void audioUpdateGameplayMusicDucking(void);
/// Service runtime events and frames until a requested program transition is ready.
/// \returns Nothing when the current program can be left; halts on system reset.
/// \note Original decompilation identifier: \c func_8000BB8C (0x8000BB8C).
void runtimeRunProgramEventLoop(void) {
    void (*callback)(void);
    static u32 startupFrames;
    static s32 resumed;
    static s16 *message = NULL;
    startupFrames = 1;
    for (;;) {
        osRecvMesg(&gRuntimeEventQueue, (OSMesg *)&message, OS_MESG_BLOCK);
        switch (*message) {
        case 1:
            if (gRuntimePendingGraphicsTasks == 0) {
                if (gRuntimeProgramChangeRequested == 0) {
                    runtimeUpdateTiming();
                    controllerUpdateInputState();
                    voiceCommandUpdate();
                    if (gRuntimeRenderPrimed == 0)
                        resumed = 0;
                    else {
                        renderBeginFrame();
                        resumed = 1;
                    }
                    romCacheReleaseUses(&gMotionRomCache);
                    gRuntimeRenderPrimed = 1;
                    callback = gRuntimeProgramUpdateEntries[gRuntimeCurrentProgram].callback;
                    if (callback != NULL)
                        callback();
                    audioUpdateGameplayMusicDucking();
                    if (resumed != 0) {
                        renderSubmitFrameBorder();
                        gRuntimePendingGraphicsTasks++;
                    }
                } else
                    return;
            }
            continue;
        case 2:
            gRuntimePendingGraphicsTasks--;
            if (startupFrames != 0) {
                if (startupFrames < 4U)
                    startupFrames++;
                else {
                    osViBlack(0);
                    startupFrames = 0;
                }
            }
            if (gRuntimeProgramChangeRequested != 0 && gRuntimePendingGraphicsTasks == 0)
                return;
            continue;
        case 3:
            runtimeShutdownAndHalt();
            continue;
        case 4:
            continue;
        }
        continue;
    }
}

extern u8 ovl_ROM_START[];
extern u8 ovl_ROM_END[];
extern u8 ovl_VRAM[];
extern u8 ovl_TEXT_START[];
extern u8 ovl_TEXT_END[];
extern u8 ovl_DATA_START[];
extern u8 ovl_RODATA_END[];
extern u8 ovl_BSS_START[];
extern u8 ovl_BSS_END[];
void renderInitializeResources(void);
void objectPrepareDistributedKindSearches(u16, s32);
void runtimeRunProgramEventLoop(void);
void textureCacheInitialize(void);
void textureCacheDestroy(void);
void actionInitializeTransitionMap(void);
void overlayLoadFromRom(void *romStart, void *romEnd, void *destination, void *textStart, void *textEnd,
                        void *dataStart, void *dataEnd, void *bssStart, void *bssEnd);
void hudProgramInitializeNoOp(void);
void heapInitializeGlobal(s32);
void *areaLoadNextOverlay(void);
void messengerResetTrackedObject(void);
extern u8 D_B140C0[];

/// Run the main runtime thread through successive program lifecycles.
/// \param scheduler Scheduler context that remains valid for this thread's lifetime, used for initialization and event delivery.
/// \returns Does not return.
///
/// \note Portability: the infinite loop and placement of the disabled debug trace preserve the original
/// IDO epilogue and register allocation.
/// \note Original decompilation identifier: \c func_8000BD70 (0x8000BD70).
void runtimeMainThreadEntry(void *scheduler) {
    runtimeInitializePlatform((s32)scheduler);
    osCreateMesgQueue(&gRuntimeEventQueue, (OSMesg *)gRuntimeThreadStorage, 8);
    schedulerAddClient(scheduler, &gRuntimeSchedulerClient, &gRuntimeEventQueue);
    gRuntimeGraphicsCommandQueue = schedulerGetGraphicsCommandQueue(scheduler);
    for (;;) {
        if (gRuntimeCurrentProgram == 0) {
            runtimeCheckResetCheckpoint("pre-overlay", 0);
            overlayLoadFromRom(ovl_ROM_START, ovl_ROM_END, ovl_VRAM, ovl_TEXT_START, ovl_TEXT_END, ovl_DATA_START,
                               ovl_RODATA_END, ovl_BSS_START, ovl_BSS_END);
            runtimeCheckResetCheckpoint("pre-heapInit", 1);
            heapInitializeGlobal((s32)areaLoadNextOverlay());
            runtimeCheckResetCheckpoint("aft-heapInit", 0);
            textDrawInitialize(0x140, 0xF0, 0xDA);
            textDrawSetPalette(gDefaultTextPalette);
            TRACE("unknown program number(%d).\n", gRuntimeCurrentProgram);
        }
        audioSelectCueBank((s32)&D_B140C0);
        heapDebugReportFreeSpace(&gGameHeap, "after songbuffinit()");
        renderInitializeResources();
        heapDebugReportFreeSpace(&gGameHeap, "after gfxInit()");
        runtimeCheckResetCheckpoint("aft-gfxInit", 1);
        messageBarInitialize();
        actorAssetCacheInitialize();
        motionCacheInitialize();
        textureCacheInitialize();
        actionInitializeTransitionMap();
        messengerResetTrackedObject();
        runtimeCheckResetCheckpoint("aft-initMessengerSystem", 1);
        graphicsTaskInitialize(gRuntimeGraphicsCommandQueue, gColorFramebuffer0, gColorFramebuffer1, 0xC, 0x1000);
        runtimeInitializeCurrentProgram();
        objectPrepareDistributedKindSearches(0, 0xA);
        objectPrepareDistributedKindSearches(0xB3, 0xA);
        if (gRuntimeCurrentProgram == 0) {
            hudProgramInitializeNoOp();
        }
        runtimeResetTiming();
        gRuntimeProgramLoopActive = 1;
        runtimeRunProgramEventLoop();
        gRuntimeProgramLoopActive = 0;
        textureCacheDestroy();
        motionCacheDestroy();
        actorAssetReadQueueDestroy();
        messageBarResetAndReleaseImages();
        audioStopCategories(1, 0.0);
        osViSwapBuffer(gColorFramebuffer1);
        osViBlack(1);
        gRuntimeCurrentProgram = gRuntimeNextProgram;
    }
    TRACE("MAINLOOP OUT!\n");
}

/// Create and start the main runtime thread.
/// \param scheduler Scheduler context that must remain valid for the lifetime of the started thread.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C040 (0x8000C040).
void runtimeStartMainThread(void *scheduler) {
    osCreateThread(&gRuntimeThread, 0x1E, runtimeMainThreadEntry, scheduler,
                   gRuntimeThreadStorage + sizeof(gRuntimeThreadStorage), 0xA);
    osStartThread(&gRuntimeThread);
}

/// Draw the active runtime prompt panel with a pulsing final line.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C094 (0x8000C094).
void runtimeDrawPromptPanel(void) {
    if (gRuntimePromptPanelImages != 0) {
        textPanelSetLineColor((TextPanel *)&gRuntimePromptPanelLineCount, gRuntimePromptPanelLineCount - 1, 0xFF, 0xFF, 0xFF,
                              runtimeGetUiPulseAlpha());
        gDPPipeSync(gRenderDisplayListCursor++);
        gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
        textPanelDraw((TextPanel *)&gRuntimePromptPanelLineCount, &gRenderDisplayListCursor, 2);
    }
}
