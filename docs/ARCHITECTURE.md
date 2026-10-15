# Architecture overview

This is an overview of how the game runs as recovered in this tree.

Names such as `runtimeMainThreadEntry` are recovered descriptions, not original identifiers;
`docs/VOICE_AND_SCRIPT_VM.md` has the voice and script details.

Statements below were read from the source in this checkout. Anything marked *(inferred)*
is a reading of the control flow that has not been confirmed by running the game.

## Example screen context

```text
boot (entry.s)
  main                       src/main.c
    osInitialize, start idle thread
  IdleMain                   src/main.c
    start ROM DMA / PI manager
    runtimeStartMainThread   src/game/runtime.c
    drop to priority 0 and spin (the idle thread)

main runtime thread          runtimeMainThreadEntry
  runtimeInitializePlatform
  register as a client of the scheduler -> receives retrace and frame events
  loop forever, one pass per "program lifecycle":
    load resident overlay   (program 0 only)
    load the selected scene overlay, start the heap after it
    init audio bank, rendering, caches, graphics tasks, current program
    runtimeRunProgramEventLoop      <- the per-frame loop
    tear down caches and queues, swap program, repeat
```

## Threads and message flow

| Thread | Entry | Role |
|---|---|---|
| Idle | `IdleMain` | Starts services, then spins at priority 0. |
| Main runtime | `runtimeMainThreadEntry` | The game: initialization, frame updates, drawing. |
| Scheduler: retrace | `schedulerRetraceThread` | Turns vertical retraces into events for registered clients. |
| Scheduler: graphics | `schedulerGraphicsThread` | Runs submitted RSP/RDP tasks, presents frames. |
| Scheduler: audio | `schedulerAudioThread` | Runs audio tasks. |
| Controller | `controllerThreadMain` | Controller polling, microphone (VRU) requests, EEPROM, rumble. |

The scheduler (`src/game/scheduler.c`) fans events out to clients with
`schedulerAddClient` and `schedulerNotifyClients`. The main thread receives four message
kinds on its event queue:

- `GAME_MESSAGE_RETRACE`: run one frame (below), unless graphics tasks are still pending.
- `GAME_MESSAGE_FRAME_COMPLETE`: a graphics task finished; decrement the pending count.
  Four startup frames pass before `osViBlack(0)` unblanks the display.
- `GAME_MESSAGE_PRE_NMI`: reset requested; shut down and halt.
- `GAME_MESSAGE_GRAPHICS_COMPLETE`: ignored by this loop.

## One frame

`runtimeRunProgramEventLoop` handles a retrace message by running, in order:

1. `runtimeUpdateTiming`: advance clocks and the smoothed frame duration.
2. `controllerUpdateInputState`: sample input.
3. `voiceCommandUpdate`: consume any microphone recognition result.
4. `renderBeginFrame` (once rendering is primed): set the background and camera.
5. `romCacheReleaseUses` on the motion cache.
6. The current program's update callback (`gRuntimeProgramUpdateEntries`), which advances
   the active game mode (`runtimeUpdateGameMode`).
7. `audioUpdateGameplayMusicDucking`.
8. `renderSubmitFrameBorder`: finish the display list and submit the graphics task.

A program change request makes the loop return, which ends the lifecycle.

## Programs, game modes and areas

- **Program.** A top-level run state selected by `gRuntimeCurrentProgram`. The retained
  callback table (`include/runtime_program.h`) only has program zero populated. Its
  initialize/update/draw entries are strided slots in one table.
- **Game mode.** The state machine inside a program (`include/runtime_mode.h`): gameplay,
  toolbox, system menu, text window, area card, collection results, world map, save
  prompt/confirmation/record/control/acknowledgment, eye catch, fade-to-load states and
  the title. `runtimeRequestGameModeChange` transitions and accounts play time.
- **Area (scene).** One of 23 areas, each a separate overlay in ROM. Entering an area
  re-runs the lifecycle: the resident overlay is reloaded, then
  `areaLoadNextOverlay` loads the destination scene overlay with `overlayLoadFromRom`
  (zero text/data, invalidate caches, DMA, zero BSS), and the heap starts after it.
  *(inferred: a fade-to-area-load game mode requests the program reload that triggers
  this.)*

## Memory and overlays

| Region | Contents |
|---|---|
| Boot segment (ROM `0x1000` to `0xA9800)` | Game C and SDK code resident from startup. |
| Resident overlay (ROM `0xB3C70` to `0xDCF60`) | Shared gameplay code in `src/ovl/`: script VM, actor queries, items, companion, system menu. |
| Scene overlays (ROM `0xDCF60` to `0x4B4990`) | One per area in `src/scenes/`, loaded over the same RAM region on area change. |
| Shared render assets (ROM `0x4B4990` to `0x505090`) | Loaded at `renderInitializeResources`. |

Scene overlays are replaced, never kept alongside each other: references into the previous
scene must be dead before the next load (documented on `areaLoadNextOverlay`).

## Area setup

`runtimeInitializeSceneGameplay` initializes a scene's gameplay in this order:

1. activate the destination area
2. area card
3. presentation
4. backdrop buffers
5. area render assets
6. item registry
7. object system
8. maze/terrain
9. player parameters
10. Pikachu actor
11. system menu
12. toolbox display objects
13. microphone and default dictionary
14. message window
15. **scene initializer** (`areaRunSceneInitializer`)

The scene initializer calls into the scene overlay. Scene files such as
`src/scenes/bedroom.c` implement it: they create actors (`actorCreateForKind`), doors and
items according to saved progress (`progressGetValue`).

## Subsystems and where to find them

| Subsystem | Files |
|---|---|
| Runtime loop, timing, modes | `src/game/runtime.c`, `src/game/scheduler.c` |
| Boot, rendering, cameras | `src/main.c` (render*, camera* helpers), `src/game/texture_cache.c`, `src/game/backdrop.c`, `src/game/particle_effects.c` |
| Controllers, rumble, EEPROM | `src/main_controller.c`, `src/game/controller_*.c`, `src/game/eeprom_transfer.c` |
| Voice (microphone/VRU) | `src/game/voice_device.c`, `voice_recognition.c`, `microphone_*.c`, `voice_commands_text_panel.c` |
| Actor scripts and actions | `src/ovl/script_vm.c`, `src/ovl/actor_script_*.c`, `src/game/action_script*.c` |
| Objects, actors, collision | `src/game/object_hierarchy.c`, `actor_*.c`, `collision_grid.c`, `node_pool.c`, `keyed_pool.c`, `search_tree.c` |
| Items, toolbox, menus | `src/ovl/item_*.c`, `src/game/toolbox.c`, `src/ovl/system_menu.c`, `src/game/text_window.c` |
| Audio and music | `src/game/audio_*.c`, `sequence_*.c`, `midi_sequence.c`, `sound_effect_player.c`, `ambient_audio.c` |
| Assets and ROM access | `src/game/rom_dma.c`, `rom_cache.c`, `asset_loader.c`, `archive_compression.c`, `motion_cache.c` |
| Save and progress | `src/game/save_progress.c`, `player_profile.c` |
| Heap and pools | `src/game/heap.c`, `node_pool.c`, `keyed_pool.c` |
| SDK (libultra) | generated locally under `build/asm/` from your ROM; not in the tree |
