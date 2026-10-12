# Undefined Behavior & Portability Ledger

```
      !  ---------------------------------------------------  !
      !  THIS CODEBASE TARGETS EXACTLY ONE COMPILER: IDO 5.3  !
      !  Matched C is a *witness* for the ROM's machine code, !
      !  not portable ISO C. Retarget at your own risk.       !
      !  ---------------------------------------------------  !
```

A matching decompilation inverts the usual contract of C. Normally you
write standard C and any conforming compiler may run it; here the
machine code is fixed (the retail ROM) and the C exists to make one
specific compiler — SGI IDO 5.3, `-O2 -mips2 -32 -G0` — emit those
exact bytes. Where the original programmers (or the SDK, or the
compiler's own idioms) relied on undefined, unspecified, or
implementation-defined behavior, we must keep that reliance to match.

## Annotation convention

Markers appear in source at the site of the hazard:

| marker          | meaning                                                      |
| --------------- | ------------------------------------------------------------ |
| `UB:`           | undefined behavior per ISO C; a different compiler/optimizer may do anything, including deleting the code |
| `NONPORTABLE:`  | implementation-defined or ABI/hardware-specific behavior; well-defined on IDO + MIPS o32 + N64, different elsewhere |

ASan/UBSan findings, if the code is ever compiled for a modern host,
should be recorded here and at the site with the sanitizer name.

## Project-wide hazards (apply everywhere, not re-annotated per site)

1. **Hardcoded address constants.** BSS, stacks, hardware registers
   (`0x800A9000`, `0xA4600010`, ...) are produced by casting integers
   to pointers. Implementation-defined (C90 6.3.4), and on any hosted
   platform those dereferences are wild pointers. Under ASan every one
   of these is an invalid access; on N64 they are the memory map.

2. **Signed integer overflow.** IDO compiles with 32-bit two's
   complement wraparound and the game logic assumes it. ISO C calls
   signed overflow undefined; clang/gcc at `-O2` will optimize on the
   assumption it cannot happen (e.g. delete `x + 1 < x` checks).

3. **Type punning through unions and pointer casts.** The SDK
   (libultra headers in `include/`) routinely reads float bits as
   integers, treats message queues as untyped word stores, and aliases
   structs. Violates C99+ strict aliasing; IDO has no strict-aliasing
   optimizer so it is safe *there*. A modern compiler needs
   `-fno-strict-aliasing` at minimum.

4. **Intentional infinite loops** (`for (;;);` in idle threads,
   `while (TRUE);` hangs). Defined in C90, and *also* defined in C11:
   6.8.5p6 only lets an implementation assume termination when the
   controlling expression is not a constant expression, and `for (;;)`
   / `while (1)` are exempt. What is nonportable is the codegen: IDO
   emits the loop plus a dead function epilogue after it, and those
   unreachable bytes are in the retail ROM. See `IdleMain` in
   `src/main.c`.

5. **No C runtime.** `main` returns `void`, is called from hand-written
   assembly (`asm/entry.s`) with BSS cleared manually and a stack
   pointer picked from a linker symbol. Freestanding, not hosted.

6. **Stores below the stack pointer.** IDO "homes" unused incoming
   arguments to the caller's argument-save area even in leaf functions
   with no frame (`sw a0, 0(sp)` — see `n_alSynFreeFX`). Legal only
   because the o32 ABI reserves that area and nothing on the N64
   asynchronously clobbers it (interrupt handlers switch stacks).

7. **Uninitialized reads that match.** Some functions only match when a
   variable is declared but not initialized on every path, because that
   is what the original code did. These are genuine latent bugs in the
   shipped game, preserved intentionally; each gets a site `UB:` marker.

## Per-site ledger

| file        | symbol          | marker       | note |
| ----------- | --------------- | ------------ | ---- |
| src/main.c  | `main`          | NONPORTABLE  | `void main(void)`, freestanding entry |
| src/main.c  | `D_800A9DE8`    | UB           | one-past-end "stack base" pointer; never dereference |
| src/main.c  | `IdleMain`      | NONPORTABLE  | `for(;;);` idle loop (defined in C11: constant controlling expression, 6.8.5p6) followed by an unreachable epilogue IDO still emits |
| src/main.c  | `IdleMain`      | NONPORTABLE  | dead argument homing store emitted by IDO |
| src/main.c  | `n_alSynFreeFX` | NONPORTABLE  | store below sp in frameless leaf |

Grow this table as functions are decompiled; `grep -rn 'UB:\|NONPORTABLE:' src/`
must agree with it.

SDK implementation ledgers were removed with the SDK C sources. SDK sections
are now generated locally; see [Local SDK inputs](LOCAL_SDK.md).

## game wave5B

| file             | symbol             | marker       | note |
| ---------------- | ------------------ | ------------ | ---- |
| src/game/heap.c | `heapDebugNoOp37FBC` / `heapDebugReportBlock` | UB | K&R one-parameter empty stub called with two arguments (ROM passes `a0, a1`); only the K&R form keeps the stub's single `sw a0` home |
| src/game/toolbox.c | `progressAddValue` | NONPORTABLE | calls the game function at 0x80032C1C, which symbol_addrs names `progressGetValue`; this tree gives it the libultra prototype `s32 (u32)` |
| src/game/text_window.c, 28030.c, 2F760.c | volatile-less hardware-ish globals read after `(u64)` masks | NONPORTABLE | `x & 0xFFFFFFFFFFFFFFFFu` and `(u64)(cond)` are register-allocation witnesses for IDO's 64-bit pair handling, value-neutral |

## game wave6A

| file             | symbol             | marker       | note |
| ---------------- | ------------------ | ------------ | ---- |
| src/game/actor_movement.c | `actorApproachTargetWithMediumScaleOscillation` / `actorApproachTargetWithSlowScaleOscillation` | UB | the "neither condition holds" path ends with a call to a `void` function and falls off the end of the `s32` function; the ROM returns whatever `$v0` the callee left, and the caller reads it |
| src/game/doors_object_responses.c | `objectResponsesClearWithCallback` | NONPORTABLE | the callback arrives as an `s32` and is cast to a function pointer for the indirect call (32-bit pointers) |
| src/game/actor_movement.c, 8200.c, 1CD10.c | `actorAdvanceWithGroundClearance`, `actorBackstepTowardTarget`, `objectAllocateHierarchyMatrixBuffers`, `objectResponsesClearWithCallback` | NONPORTABLE | actor pointers are `s32` parameters and are offset and dereferenced as addresses, as the callers pass them (32-bit pointers) |
| src/game/sequence_control.c | `sequenceEngineGetVoicePan` | UB | declared `s32 sequenceEngineGetVoicePan();` (K&R) and called with an `s32` where the prototyped definition takes `SndVoiceTab *`; the callers cannot be re-typed without changing the ROM |
| src/main.c | `controllerStartThread` | UB | `args.ready` is polled without `volatile` while another thread writes it; IDO reloads it because the loop body is a call |
| src/game/unk_*.c | `s32 pad[n]` locals | NONPORTABLE | unused locals kept only so the frame size and slot offsets match the ROM (marked "unused, sizes the frame") |

## game wave7

| file             | symbol             | marker       | note |
| ---------------- | ------------------ | ------------ | ---- |
| src/game/text_window.c | `textWindowDrawBackground` | UB | K&R definition `(gdl, alpha)`; callers pass two or three arguments |
| src/game/microphone_session.c | `eepromIsRequestOrTransferBusy` | NONPORTABLE | returns `int`, not `s32` (`long`): IDO 5.3 inserts an int-to-long conversion for an `\|\|` result returned as `long`, which moves the value through `$v1` |
| src/game/title_screen.c | `titleScreenClearScreen` | NONPORTABLE | now declared `(Gfx **)` while `titleScreenDrawFrame` still passes its `s32` display-list cursor (32-bit pointers) |
| src/game/object_hierarchy.c | `objectMakeHierarchyRoot` | NONPORTABLE | returns its argument; callers in particle_effects/flipbook_messenger declare it `void` (the return value lands in `$v0` and is ignored) |
| src/game/audio_playback.c | `audioFadeOutSequence` | NONPORTABLE | two initialisers and the loop head share one source line under `clang-format off`; IDO 5.3 schedules by source line |

## ovl wave8

| file             | symbol             | marker       | note |
| ---------------- | ------------------ | ------------ | ---- |
| src/ovl/area_terrain_maze.c | `terrainGetMaterialSound` | UB | `D_80123A48[][8]` indexed with a column masked to 0..15: columns 8..15 read the next row (out-of-bounds subarray); flat-array spellings change IDO's register order |
| src/ovl/area_terrain_maze.c | `mazePrepareObjectHierarchy` | NONPORTABLE | the node arrives as an `s32` and is passed on as a pointer (32-bit pointers) |
| src/ovl/player_companion.c | `playerUpdateViewCamera` | UB | K&R definition `(arg0)`; `playerSetWorldPosition` calls it with a second, pass-through argument |
| src/ovl/script_vm.c | `scriptValueGetInteger` | UB | K&R definition `(arg0)`; `scriptVmReadIntegerOperand` calls it with a second argument (`a1` = its VM pointer) |
| src/ovl/actor_script_queries.c | `actorReleaseHeldItem` | NONPORTABLE | `s32 pad[2]` unused local sizes the frame |
| src/ovl/actor_script_player_world.c | `actorScriptSetPikachuSecondaryMeter` | NONPORTABLE | the clamp and the store share one source line under `clang-format off`; IDO 5.3 schedules by source line |
| src/ovl/actor_script_player_world.c | `actorScriptPushElapsedTime` | NONPORTABLE | the VM handle pointer is passed as an `s32` (32-bit pointers) |

## game wave8

| file             | symbol             | marker       | note |
| ---------------- | ------------------ | ------------ | ---- |
| src/game/archive_compression.c | `func_80049D64` | NONPORTABLE | the free and the re-alloc share one source line under `clang-format off`; IDO 5.3 schedules by source line |
| src/game/trigonometry.c | `matrix33KeepScaleOnly` | NONPORTABLE | two unused `f32` locals (`pad`, `pad2`) only place the spill homes where the ROM has them |
| src/game/scheduler.c | `schedulerAudioThread` | NONPORTABLE | the yielded task is held as an `s32` and offset into an `OSTask *` (32-bit pointers); `for (;;)` leaves IDO's dead epilogue in place |
| src/game/texture_cache.c | `textureGetDescriptor` | NONPORTABLE | empty `if (idx < 0 \|\| idx >= count) {}`: a compiled-out bounds check whose test the ROM still evaluates |
| src/game/save_progress.c, voice_recognition.c | `directionAnimationStartMotion`, `voiceRecognitionTakeResult` | NONPORTABLE | unused `pad` locals (marked "unused, sizes the frame") only size the frame |
| src/game/backdrop.c | `backdropAdvanceScroll` | NONPORTABLE | the same word is reached as `D_800ABCF0 + 0xA0` and as `D_800ABD90`; C sees two objects, so the aliasing is invisible to the compiler (as in the ROM) |
| src/game/keyed_pool.c | `keyedPoolFindEntryWithValue` | NONPORTABLE | the table, key and out pointer arrive as `s32` (the unit's prototype) and are cast to pointers (32-bit pointers) |
| src/game/particle_effects.c | `effectEmitterCreateWithCallback` | NONPORTABLE | `objectResolveAvailableHandle` is declared `s32` in this unit and its result is used as an object pointer (32-bit pointers) |

## game wave9A

| file             | symbol             | marker       | note |
| ---------------- | ------------------ | ------------ | ---- |
| src/game/object_hierarchy.c | `romByteReaderRead` | UB | K&R definition with two parameters; `midiReaderOpen` calls it with a stray third argument (the ROM sets `a2` for that call) |
| src/game/object_hierarchy.c | `matrix34MakeViewFromEulerDegrees` (declaration) | UB | K&R declaration of a three-parameter function (vector_math); `objectEvaluatePointRelativeYaw` passes four arguments, `objectFindNearestTargetInView` three |
| src/game/object_hierarchy.c | `objectEvaluatePointRelativeYaw` | NONPORTABLE | the existing second argument carries a pointer as `s32`, cast to `f32 *` for `matrix34GetTransformedPointYawDegrees` |
| src/game/texture_cache.c | `textureCacheResolve` | NONPORTABLE | an unused `pad` local only sizes the frame |
| src/libm/sins.c | `sins` | NONPORTABLE | the SDK's static `sintable` lives in a data blob; the ROM symbol `sins_data_0000` is named `extern` instead of defining the table |


## game wave9B

| file             | symbol             | marker       | note |
| ---------------- | ------------------ | ------------ | ---- |
| src/game/save_progress.c | `directionAnimationAdvancePose` | UB | K&R definition with a second parameter (the record index); `directionAnimationUpdateRetargetingMotion` and `directionAnimationUpdateTimedMotion` call it with one argument, so `arg1` is the caller's leftover `a1` |
| src/game/graphics_text.c | `videoConfigureMode` | NONPORTABLE | `comRegs.width` and `comRegs.xScale` of `D_800F29B0` are written through their own symbols `D_800F29B8`/`D_800F29D0`; C sees separate objects, so the aliasing is invisible to the compiler (as in the ROM) |
| src/game/save_progress.c | `progressReadFieldValue` | UB | K&R definition `(idx, type)`; `progressGetValue` calls it with one argument and the dead `type` parameter is reused as a local (the ROM keeps it in `a1`); the bitfield mask `(1 << (bits + 31)) << 1` shifts by 31 or more, exactly as the ROM does |
| src/game/scheduler.c | `schedulerRetraceThread` | NONPORTABLE | `for (;;)` message loop; IDO keeps its dead epilogue (seven nops and the restores) after the loop |
| src/game/title_screen.c | `titleScreenDrawLogo`, `titleScreenDrawStartupPanel`, `titleScreenDrawStartPrompt`, `titleScreenDrawStartupSilhouette` | NONPORTABLE | take `Gfx **`, but `titleScreenDrawFrame` passes the display-list cursor as `s32` (32-bit pointers) |

## ovl wave9

| file | symbol | marker | note |
| ---- | ------ | ------ | ---- |
| src/ovl/script_vm.c | `scriptVmReadVariableReference` | UB | frame values below -1 return an uninitialized pointer, matching the ROM |
| src/ovl/actor_script_collection_profile.c | `actorScriptPushLegacyVoiceResult` | UB | reads the first halfword of an uninitialized 0x20-byte stack buffer when mode is nonnegative |
| src/ovl/actor_script_queries.c | `actorFindNearestHandleFromPosition`, `objectFindNearestHandleInFacingCone`, `objectFindNearestHandleFromPosition`, `actorFindNearestHandleInFacingCone` | NONPORTABLE | object handles arrive as s32 and their fields are passed as pointers (32-bit pointers) |
| src/ovl/actor_target_interaction.c | `actorPostScriptMessage` | NONPORTABLE | the object handle arrives as s32 and is cast to a pointer |
| src/ovl/actor_script_player_world.c | `actorScriptReplaceItemKind` | NONPORTABLE | object handles and VM addresses are represented as s32 (32-bit pointers) |
| src/ovl/actor_script_messages_items.c | `actorScriptCreateAttachedEffect` | NONPORTABLE | an unused word between the VM handle and popped kind preserves the observed stack homes; the second pop has its own named local |

## effect callback dispatch

| file | symbol | marker | note |
| ---- | ------ | ------ | ---- |
| src/game/particle_effects.c | `effectEmitterCreateForStyle` | NONPORTABLE | callback addresses are passed to `effectEmitterCreateWithCallback` as s32, preserving its 32-bit handle representation |

## Keyframe time advancement

| file | symbol | marker | note |
| ---- | ------ | ------ | ---- |
| src/game/animation_tracks.c | `animationTrackAdvanceKey` | NONPORTABLE / UB | `idx` carries a writable pointer in an `s32`; the key pointer slot uses the existing `f32 **` interface but is accessed as `Key16 **` (incompatible pointer types under strict aliasing) |

## Eye-catch rendering

| file | symbol | marker | note |
| ---- | ------ | ------ | ---- |
| src/game/eye_catch.c | `eyeCatchDrawFadingImage` | NONPORTABLE | the existing caller passes its display-list cursor through an s32; the cast assumes 32-bit pointers |
| src/game/eye_catch.c | `eyeCatchChooseGroup` | NONPORTABLE | two unused words preserve the original 0x28-byte frame and the selected-column spill at sp+0x18; no source-level meaning is assigned to the reserved bytes |

## Grid deformation

| file | symbol | marker | note |
| ---- | ------ | ------ | ---- |
| src/game/ground_shadow.c | `groundShadowUpdateVertices` | UB | count == 1 enters the vertex loop and divides integer texture coordinates by count - 1; the original input precondition is preserved |


## Node pool initialization

`src/game/node_pool.c` — `nodePoolInitialize` forms the first node's predecessor
one element before the allocated array, then clears that link after initializing
all nodes. The transient pointer is not dereferenced, but forming it is outside
ISO C array-pointer bounds. The original also requires a positive node count;
the observed caller supplies0x761. Preserved for the exact IDO instruction stream.


## Maze link initialization

| file | symbol | marker | note |
| ---- | ------ | ------ | ---- |
| src/ovl/area_terrain_maze.c | `mazeConfigureEventLinks` | NONPORTABLE | IDO schedules the initial configuration load differently unless the offset initialization and loop label share a source line; protected by a narrow clang-format exclusion |

`src/game/keyed_pool.c` — `keyedPoolInitialize` likewise forms the first node's
predecessor one element before the allocation while building a circular list,
then replaces it with the final node before splitting the chain at its midpoint.
That transient pointer is not dereferenced. The initializer requires a positive
count; zero would also make the final node decrement invalid. Original behavior
is preserved for the exact IDO match.


## Linked-model oscillation

`src/ovl/item_physics.c` — `pinataUpdateWriggle` depends on IDO source-line scheduling:
the loop-index initialization and do statement share one source line so the
damping load precedes initialization. A narrow formatting guard preserves it.


## Particle display-list initialization

`src/game/particle_effects.c` — `particleClusterCreateDisplayList` requires the texture-enable and
move-word macros on one source line to preserve IDO instruction scheduling.
A narrow formatting guard documents and preserves this dependency. Matrix
addresses are encoded segment-7 addresses, as required by the original RSP list.


## Randomized particle displacement

`src/game/particle_effects.c` — `wispEffectUpdateDriftingMotion` uses integer factors 2 and 6 in
double-precision random displacement expressions. IDO preserves multiplication
for these spellings; a double literal 2.0 instead becomes an addition and does
not match. Scale increments retain the promoted value of the float 0.01f.


## Splay-tree detach/link scheduling

`src/game/search_tree.c` — `searchTreeLocateRoot` pairs each node detachment and
following tail-link assignment on one source line. IDO otherwise reverses
three independent address/index calculations. Narrow formatting guards
preserve the exact instruction order without changing tree operations.


## Grid display-list setup

`src/game/ground_shadow.c` — `groundShadowBuildDisplayList` keeps texture enable and fog
move-word commands on one source line to retain IDO scheduling. A narrow
formatting guard preserves the dependency.


## SDK fixed-matrix representation

`src/game/trigonometry.c` — `matrixFixedTo34` accepts the aligned SDK `Mtx`
value used by its callers, then views its integer and fractional halves as
16-bit arrays. This relies on N64 big-endian storage and IDO's aliasing
behavior; a strict-aliasing portable implementation needs explicit byte
extraction or copying. The corrected aggregate contract preserves both
caller and converter instructions.


## Horizontal movement-vector scheduling

`src/game/actor_movement.c` — `actorApproachTargetWithinSceneBounds` keeps the three horizontal-vector
assignments on one source line. IDO otherwise reverses the zero and Z
stores. A narrow formatting guard preserves the exact store order.


## Overlay loader address representation

`src/game/overlay_loader.c` — `func_800135C0` uses linker section bounds as
byte pointers and rounds the aligned BSS end through a 32-bit integer,
matching the N64 address space.

The startup caller in `src/game/runtime.c` still declares ROM bounds as
pointers and discards the result; the recovered definition takes two u32
ROM offsets and returns a pointer. Correcting this caller currently changes
saved-register allocation. All nine arguments are one word on N64. This
ABI-dependent declaration mismatch remains a contract cleanup item.


## Audio bank relocation contracts

`src/game/audio_bank_loader.c` — `audioSoundRelocate` and `audioWaveTableRelocate` now pass
the bank relocation delta and sample ROM base explicitly. The former
K&R/pass-through calls relied on inherited a1/a2 register values; typed
three-argument calls preserve the original instructions. Numeric pointer
fields still represent the N64 32-bit address space.
