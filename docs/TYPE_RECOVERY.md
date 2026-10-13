# Type and name recovery

This pass recovers supported interfaces and record fields without changing the
retail ROM. Remaining placeholders are deferred explicitly; their presence does
not imply that a complete layout or semantic name has been established.

## Inventory

The baseline is main commit `acee42740ab9e971b1180d2ddfa03df76ed73135`.
Counts exclude comments, strings and character literals and cover game C under
`src/`. Occurrences and affected files are separate measures.

| Category | Before occurrences / files | After occurrences / files |
| --- | --- | --- |
| `M2C_FIELD` | 2,575 / 75 | 0 / 0 |
| `M2C_UNK*` | 1,012 / 71 | 0 / 0 |
| Address-named globals | 6,650 / 102 | 7 / 3 |
| `temp_*` | 2,480 / 58 | 0 / 0 |
| Raw offset candidates | 4,040 / 86 | 2,047 / 76 |

Address-global counts are references, not declarations: three address-named
globals remain in three files, with seven references. Their unresolved roles
are recorded in the recovery ledger. Counts reflect the current source;
regenerate the report after further recovery. Temporary-name uniqueness is
spelling uniqueness, not distinct local variables. The raw-offset heuristic
includes scalar arithmetic; it is a review list, not a count of incorrect
pointer accesses.

Run `make source-quality`, `make source-quality JSON=1`, or
`python3 tools/source_quality.py --json` for current counts and every occurrence's
location, function and source context.

## Recovered interfaces and views

- Node pools now use their entry/list fields throughout, and the object-manager
  consumer uses their shared prototypes. Keyed pools use one record layout in
  place of five overlapping views; ordered links and lookup chains remain distinct.
- DMA queues and cartridge caches expose their owned storage, counters, completion
  queue, heap and keyed index. Motion, texture and actor-configuration callers use
  the recovered declarations.
- The allocator interface records the established heap, byte-size and flag types.
  Callers adopt it where object comparison permits; legacy extra-argument calls
  remain explicit exceptions.
- The runtime-object prefix describes transforms, hierarchy links, object kind,
  flags, handle, matrix buffers, world position, root and kind-specific state.
  Actor script consumers additionally use the established VM address and mailbox.
- Item and player prefixes describe supported motion/rendering and control fields.
  These are separate records. Scene consumers share the recovered configuration
  dimensions and object scale. Text image dimensions and orbit initialization use
  existing types instead of offset macros.
- Function declarations were reconciled against established definitions where the
  types could be expressed and verified. Local names were recovered only where
  their definitions and uses established a stable role.

Headers document purpose, field roles and interface contracts with Doxygen.
Compile-time size and offset checks validate recovered layouts for the target ABI.
A type named `Prefix` is a view, not an allocation size: do not allocate, copy or
clear complete runtime objects with its `sizeof`. Unknown storage remains explicit.
Integer and pointer views of object references preserve the existing 32-bit ABI.

## Evidence and matching constraints

Recovery used allocation/initialization, readers/writers and caller definitions,
checked against the resident Ghidra analysis and ROM assembly. Representative
Ghidra queries included controller polling (`0x80003A40`), node-list append
(`0x8001BFB8`), keyed-entry acquisition (`0x800418A0`), DMA initialization
(`0x80037BEC`), cache initialization (`0x8003F700`), object creation
(`0x80008160`), item-state allocation (`0x8011386C`) and companion control
(`0x80112294`). These addresses identify recovered behavior, not original names.

Every retained source-unit edit was compared against the preserved object
baseline. Declaration corrections, field substitutions and local renames were
reverted when they changed allocated bytes or failed compilation. Function-level
trials retained matching changes around the exceptions instead of discarding all
improvements to an affected subsystem.

The exceptions are maintained in `TYPE_RECOVERY_NOTES.json`. In particular, pool
destruction and some allocator callers pass unused extra arguments in the ROM.
Their compatibility declarations preserve that call convention. Removing arguments
or changing return/address representations needs its own assembly investigation.
Known player and item-motion exceptions need register-allocation/expression-reuse
analysis; compiler-decision tools belong in the separate ido-diagnostics repository.

No compiler settings, verification tolerances, gameplay behavior, SDK names or
external dependency sources were changed. The object comparator still treats
relocation addends strictly; interior-address aliases must not be silently merged.

## Field-resolution continuation

The continuation baseline is commit `0eb2081`, with its matching build preserved
outside the checkout. The first seven source-unit changes remove 21 field macros:
heap block links/tag/size, SDK voice-handle queue/channel, sound-player volume,
text-panel capacity, ambient-source count/location, direction-animation timing/mode,
and collection text-image dimensions. Each unit matches its preserved object.

The direction-animation view combines two existing 0x28-byte poses and two
0x10-byte angular-motion records with the mode, selected pose and duration fields.
Target size and offset checks guard that local layout. Ghidra evidence was checked
at `0x800385D0`, `0x80040E64`, `0x80033FF4` and `0x80032384`.
The next batch removes another 281 field placeholders across MIDI playback and
sequence control/engine state, EEPROM requests, audio banks, eye-catch and title
rendering, backdrop descriptors, texture pairs and word maps, search trees, linked
and particle effects, fishing populations, lake spawn positions and object
hierarchies. Every source unit is compared with the same immutable baseline.

New shared headers describe MIDI/control state, search-tree nodes and fishing
population choices. Private views remain local to their units. The object-manager
allocation is confirmed at 0x68 bytes, including its entry pointer and embedded
0xC-byte tree node. Buffered ROM-reader storage and ribbon-controller state have
target size/offset checks. Existing duplicate eye-catch, word-map and tree-node
views are consolidated. A corrected terrain-query prototype preserves floating
point arguments and removes integer bit-pattern field reads in its hierarchy caller.

Additional Ghidra evidence includes sequence controls (`0x800059E4`), engine voice
links (`0x80005C1C`), registration (`0x800060A8`), EEPROM requests (`0x8000451C`),
bank relocation (`0x80020470`), backdrop selection (`0x80033A70`), linked-effect
motion (`0x8001CF3C`), ribbon initialization (`0x80017D18`), search-tree removal
(`0x8004131C`) and buffered-reader initialization (`0x800076E0`).

The subsequent continuation resolves the remaining field placeholders; see the final sweep below.

## Continuing recovery

`TYPE_RECOVERY.json` inventories every residual occurrence by file and token, with
`line:column` locations. Category policies give the reason for retaining each class
and the next evidence needed; file-specific exceptions take precedence. Included
project headers identify available shared views. This is an inventory survey of all
game C, not a claim that every residual expression has received a complete semantic
analysis. Raw offsets remain candidates and may be intentional byte arithmetic.

Before editing, save a matching build outside `build/`. Keep one source-unit
mutation active, use Ghidra to resolve ambiguity, and preserve access width,
alignment, signedness, ownership and expression order. Do not assign speculative
field meanings or replace unknown types mechanically with `void *`.

After source locations or names change, update any relevant exception notes and
regenerate the ledger:

```sh
python3 tools/source_quality.py --ledger > docs/TYPE_RECOVERY.json
python3 tools/source_quality.py --check-ledger
```

The stale-ledger check is part of `make verify`. Acceptance also requires a clean
build ending in `OK: ROM matches`, full object comparison to the saved baseline,
direct ROM comparison, name checks without errors, tests and publication hygiene.
Regenerate m2c context when headers change. Do not replace the baseline to hide a
failed comparison. See `AGENT_WORKFLOW.md` for the exact verification commands.

## Actor and VM continuation

The actor-state view now covers the observed prefix through 0x238 of its 0x408
allocation. Producers and consumers establish pose, action requests, motion,
targeting, sound handles, item ownership, collision extents, and messenger state.
Offset-named storage remains where semantics are uncertain. The final handle
also makes the prefix extent compatible with its eight-byte-aligned timestamp.

Model motion, playback, movement, target interaction, and action dispatch use
these fields. Script contexts now expose their message settings, action timer,
handler selector and initialization flag; creation reads a documented resource
prefix with an unresolved word at 0xC. Messenger state uses its existing record.
Ghidra initializer and motion analysis at 0x80014FE4 and 0x80016DE0 corroborates
the actor layout; VM creation at 0x80115114 corroborates the resource prefix.

The two blend reads in actionUpdatePlayback retain checked member-offset
addressing because direct member reads reverse a floating add's operands under
IDO. This exception is recorded in the ledger notes and remains object-identical.

## Player, scene, scheduler and item continuation

Player initialization and movement establish the 0x100 record stride, controller
filtering, collision motion, footstep state, held-item handle, hierarchy handles,
behavior callbacks, camera-transition poses, profile points and friendship state.
The controller producer and player consumers share one processed sample type.
Three player angle reads retain checked member offsets for IDO operand ordering;
the camera and interpolation interfaces now use their established float arguments.
Ghidra at 0x80111F40 and 0x8010F370 corroborates initialization and movement.

Scene queries expose the entrance-pose, scenario-program and path tables, plus
region highlight and cave anchor/landmark records. The 0x20 entrance record holds
separate player and companion poses. The path-object prefix remains partial: its
points pointer is known, while its preceding words and full allocation are not.

Scheduler registrations and graphics submissions now share checked records.
Matrix and vector placeholders became typed array indexing. All placeholders in
player control, item registry, item physics and item special behaviors are gone.
The item registry's state at 0x54 is an embedded tree node: the former pointer at
0x58 is its greater-key subtree link. Registry initialization and traversal prove
this relationship. Shared item-kind configuration retains unresolved offset-named
fields while exposing the friction coefficient and callback contracts.
Two item velocity reads and one radius read retain checked member offsets for
IDO scheduling and floating operand order. These exceptions match the baseline.

## Final field sweep

All game C now contains zero `M2C_FIELD` accesses. The final sweep recovers maze
nodes and terrain grids, scene cell resources and placements, menu navigation and
entries, marker state, toolbox slots, actor creation arguments and gameplay meters.
The actor-state allocation establishes the 0x408-byte extent; offset-named words
remain where relative-part controller policy or action response semantics are not
fully established. Profile bytes with unknown roles likewise retain offset names.
The effect creator prototype now carries world coordinates as floats consistently
with its definition, without changing the compiled calls.

The keyed-pool layouts live in `keyed_pool_types.h`, allowing door clients to reuse
them while retaining their documented legacy extra-argument ABI declarations.
Menu backdrop access is an explicitly bounded interior view: the scroll word at
view offset 0xA0 precedes the backdrop-enable word at 0xA4. The renderer reads this
flag to select backdrop drawing; the view does not establish a complete camera
allocation. Maze allocation/model setup and backdrop scroll were cross-checked
through Ghidra at `0x8010D4F0`, `0x8010D658` and `0x80033118`.

Four terrain functions retain checked member-offset reads because direct fields
change IDO operand ordering or scheduling. Child-array byte cursors and the
three-symbol maze reset storage retain target-specific representations, recorded
in the exception ledger. Raw-offset candidates increase slightly because these
proven member-offset expressions are counted by the deliberately broad heuristic.
Zero field macros does not imply that all field meanings, unknown types, address
names, local names or buffer arithmetic have been resolved. The ledger preserves
those outstanding recovery targets and the evidence required to advance them.

Final acceptance used a clean build, `make verify BASE=main
BASELINE=/tmp/pikachu-field-resolution-baseline/build`, direct `cmp`, publication
hygiene and progress reporting. All 483 objects match, all 31 tests pass, and the
name checker has no errors (five permitted port-binding warnings). The clean build
prints `OK: ROM matches`. Both ROM files have SHA1
`da1a1e47a86720f9d54fb2d2d247480041bda824`. The comparator and compiler settings
are unchanged; integration into main remains separate.

## Unknown-type and local-name continuation

This continuation starts from main `2e09177`, with a separate immutable matching
build at `/tmp/pikachu-types-locals-baseline/build`. Its starting counts are 358
unknown-type tokens in 57 files and 1,726 temporary-name occurrences in 53 files.
The first batch reduces them to 234 in 34 files and 912 in 35 files respectively.
Field-access macros remain at zero. Every source-unit mutation is compared before
the next unit is changed; the original strict comparator is unchanged.

Recovered local roles include vector components and normalization lengths, heap
block sizes and selection, actor creation resources, script VM addresses, item
transformation results, target objects, and VM call/frame operands. Names preserve
expression order, statements and control flow. Unknown return/argument types are
reconciled against definitions and callers; compatibility declarations explicitly
retain unused original ABI arguments where a canonical prototype rejects them.
Unused external declarations are removed only when the identifier has no code
references. The actor-creation reserved stack word remains because removing it
changes the compiled frame.

`text_panel_types.h` shares the existing panel layout, its circular next-entry
index, line/color storage and borrowed configuration with the title renderer.
Global VM storage is an array of 500 packed five-byte values, established by the
0x9C4-byte reset and indexed value accesses. Profile/session buffers retain byte
views when only their clearing size is established. Ghidra confirms the synth
player-removal void return and linked-player argument (`0x8004DF60`); heap and VM
recovery uses existing allocation, operand decoding and dispatcher evidence.

The goal remains zero unknown-type tokens and zero temporary-name occurrences.
The current ledger records the remaining locations; this checkpoint does not
assert complete recovery of their roles.

This checkpoint passes the clean-ROM gate and direct byte comparison, strict
comparison of all 483 objects against the continuation baseline, all 31 tests,
publication hygiene and the name checker (no errors; permitted warnings remain).
The compiler settings and verification tools are unchanged. Work remains on
`codex/recover-types-and-locals`; main is unchanged.

The next batch reduces unknown-type tokens to 214 in 29 files and temporary-name
occurrences to 584 in 30 files. It recovers hierarchy traversal, flipbook buffer
selection and outlines, scrolling state, messenger state, motion duration, actor
part lookup and model-resource locals. `word_map_types.h` shares the existing
12-byte map and 8-byte entry layouts, with size checks and ownership comments.
Ghidra at 0x80048CE0 confirms the initializer writes the three map words and
allocates eight bytes per entry. Original unused call arguments remain explicit
in the matching-exception ledger. Object and ROM verification use the same
immutable continuation baseline.

## Completion of unknown-type and temporary-name recovery

The final continuation reduces the previous checkpoint's 214 unknown-type tokens
and 584 temporary-name occurrences to zero across game C under `src/`. An
independent search also finds no uses in project headers; `m2c_macros.h` retains
only the decompiler compatibility typedef definitions for future draft imports.
The reporter still excludes comments and literals and has not changed its scope.

The final sweep recovers camera and maze locals, player and companion control,
item registration and physics, presentation and toolbox state, effect neighbors,
audio relocation and release timing, action commands, controller state, font
cache and text interfaces, and scene positions and packed script data. Unused
microphone locals and a player transition slot can be removed without changing
objects. No expressions or control flow were combined to rename temporaries.

Four focused headers share existing layouts: `item_spawn.h` (scene placements),
`integer_animation_types.h` (integer keys and playback cursors),
`texture_draw_types.h` (screen rectangle lists), and `sequence_driver_types.h`
(sequence driver and voice pool). Each has a Doxygen file summary, ownership and
field contracts, and target-compatible size/offset checks. Existing `RomCache`,
`PlayerControlPrefix`, `ControllerState`, `KeyedPool`, and heap types replace
unknown declarations in their consumers. Packed actor-script streams remain
explicit byte arrays; their conversion to a higher-level script format is not
claimed by this recovery.

Zero token counts do not establish every field's semantic meaning or complete
all type recovery. Partial views, offset-named fields, address-named globals and
raw buffer access remain explicit. The ledger records their locations and the
next evidence needed. It also retains original ABI arguments, split-symbol
storage and the eye-catch caller's unused integer-return compatibility exception;
changing that declaration to the canonical void return changes IDO output.

Final acceptance uses a clean build and `make verify BASE=main
BASELINE=/tmp/pikachu-types-locals-baseline/build`, direct ROM comparison,
progress reporting and publication hygiene. The immutable baseline, strict
relocation-addend comparator, SDK sources and compiler settings are unchanged.
The work remains on `codex/recover-types-and-locals`; integration into main is
separate.

The final clean build prints `OK: ROM matches`; direct `cmp` succeeds. All 483
objects match the preserved continuation baseline, all 31 tests pass, the ledger
matches current source, and the name checker reports no errors (five permitted
port-binding warnings). Both ROMs have SHA1
`da1a1e47a86720f9d54fb2d2d247480041bda824`.

## Shared-layout and contract audit

The continuation starts from main `d5ea983`, with an immutable matching build at
`/tmp/pikachu-contracts-baseline/build`. The first batch shares the action
sequencer's 0x10-byte command and 0x20-byte state records through
`action_script_types.h`. The actor layout now exposes command time, borrowed
command cursor, sound playback handle, and last/pending request identifiers at
0x164 through 0x174 instead of opaque padding. Target size and offset checks
protect both representations.

Evidence comes from actionScriptStart, actionScriptUpdate, the start-action and
play-sound command handlers, and actionScriptHasLastActionEnded. Ghidra queries
at 0x80012034 and 0x80012114 confirm initialization and completion checks.
Command resources remain borrowed; script stop and action completion remain
separate contracts. The command opcode operands remain explicit scalar words
because their interpretation varies by command.

This is the first accepted audit batch, not completion of shared-layout recovery.
Remaining work includes callback consistency, integer-address compatibility
interfaces, private duplicate views, ownership contracts, and unresolved field
semantics across the other subsystems.

The next batch reconciles all four timed-action handlers with one shared
`ActionScriptCommandHandler` signature. The ROM handler table at 0x8006EE50
contains 0x80011CB0, 0x80011E28, 0x80011D78 and 0x80011DD0, corroborating the
four definitions. Their actor-state, sequencer and command accesses now use
established fields. The remaining integer-address sound interface is explicit
at the call boundary. No expressions, handler ordering or command semantics
change, and strict comparison still accepts all 483 objects.

The scalar-stack continuation introduces `script_vm_stack.h` for the existing
integer-address ABI of numeric and borrowed-string stack operations. Profile
and demo/item commands reuse these declarations, while the VM implementation
uses the established packed ScriptValue record for all five wrappers. String
addresses are converted explicitly at the API boundary. The profile unit's
redundant VM pointer casts are removed; its horizontal-movement command's unused
second pop argument can also be removed with no object change.

The source definitions at 0x8011601C through 0x801160DC establish the scalar
signatures. Ghidra's 0x801160B4 analysis confirms the single context argument but
misidentifies the return as void; the C definition and callers establish the
integer return. Demo maze-block effects now reuse RuntimeObjectPrefix and
SceneConfigurationPrefix for object kind, radius, position and scene scale.
Each of the three source units passed strict object comparison independently.

Message/item commands, action-wait handlers, player/world commands and action
commands now share the scalar stack contracts too. Their original unused extra
stack-operation arguments can be removed without changing objects. Pointer views
needed for context fields and integer addresses needed by the stack ABI have
explicit conversions. The action-command operand reader continues to use its
established byte-pointer interface. A duplicate player-types include is removed.
Each source unit was independently compared against the immutable baseline
before the next unit changed. Remaining unrelated object, player and toolbox
interface mismatches remain in the compiler-diagnostic inventory for later audit.

The object-lookup continuation consolidates every local declaration of
objectFindLiveByHandle and objectResolveAvailableHandle into `object_lookup.h`.
All 26 affected source units were built and compared sequentially. The core
lookup filters object flags 0x30; the gameplay resolver additionally rejects
zero handles and kind-0x86 objects whose item state is four. Ghidra at 0x80007E8C
and 0x80118ACC corroborates these separate policies. Borrowed results remain
valid only until object destruction; lookup does not confer ownership.

The core lookup now uses the checked RuntimeObjectPrefix flags field. Three
units with integer-return declarations adopt the canonical pointer result; demo
and profile consumers retain pointer locals and convert explicitly where legacy
item helpers take integer addresses. Other consumers' integer-address locals
remain visible diagnostics for the next interface pass. No local declarations
of either lookup remain, and no lookup or filtering behavior changes.

The toolbox continuation shares its eight-byte kind/handle slots and 0x100-byte
display/storage state in `toolbox_types.h`, with size and offset checks for rows,
placeholder handles and bonus/purchase entries. Collection presentation reuses
these types, including the bonus-handle read formerly expressed as base+0xF4.
Separate linker aliases remain separate declarations; their relocation identities
and addends are preserved. Slot storage tracks handles without extending object
lifetime, and replacement/destruction remains controlled by toolbox routines.

Toolbox also reuses the existing texture rectangle and ItemKind records instead
of private duplicates. Explicit signed-byte casts preserve the old classification
reads. Its slot writer accepts ToolboxSlot pointers, and frame drawing retains
object pointers until integer-address drawing calls. Ghidra at 0x8002F088 confirms
kind/handle writes and removal from active item search. Both source units, and
the subsequent typed slot-writer change, independently match all 483 objects.

The player-control continuation gives mode update and leave callbacks explicit
shared signatures. All mode table entries now agree on borrowed player and input
pointers, including eight handlers formerly using integer address parameters.
The input sample is temporary call storage, corroborated by Ghidra at 0x80112344.
PlayerModeState size and both callback offsets are checked. Existing angle-read
matching exceptions remain intact.

`player_control.h` shares view-angle, companion selection and pitch-permission
contracts across their producer and all declaring consumers. The primary-player
query at 0x80117B64 formerly declared void despite its script caller consuming an
integer. Linked assembly preserves the getter result in v0 through return, and
an explicit integer return matches every object. This corrects the source
contract without changing the ROM. Ghidra's inferred void return is weaker than
the caller, getter definition and linked instruction evidence here. Each source
unit passed the strict immutable-baseline comparator before the next changed.

The particle-effects continuation recovers the private 0x18-byte timed emitter
state and its void emission callback. Initialization, creation, target following,
lifetime expiry and cooldown consumers establish all six fields; Ghidra at
0x8001968C and 0x800196D8 corroborates allocation and callback invocation. The
callback remains callable for the emitter lifetime, and target references remain
handles resolved per update. Callback addresses no longer travel through integer
parameters or fields. The record stays private because no external consumer
requires its layout.

Effect allocations retain pointer locals and use the established HpHdr heap
record. The deallocator return now agrees with its void definition; unprototyped
calls still preserve documented unused trailing arguments. This is an explicit
remaining ABI exception, not a claim that deallocation interfaces are finished.
The cluster animation evaluator receives a pointer rather than an integer. Every
step matches the immutable 483-object baseline.

The integer-animation continuation shares fully prototyped ValueTrack/ValueKey
initialization and evaluation through `integer_animation.h`. The producer uses
checked fields for key, retained value and time. Ghidra at 0x8000A5CC corroborates
initialization and initial evaluation. Every consumer now uses the same contract;
embedded tracks and retained integer resource addresses have explicit typed views
at the boundary, with byte-offset expressions preserved before conversion.

The actor-part texture-animation caller's unused third argument is removable
without changing its object, so its old unprototyped compatibility declaration
and portability exception are removed. All consumer source units were compared
individually with the immutable baseline. An early incorrect cast placement was
rejected by verification and corrected before acceptance; no comparison rules or
baseline files changed. The track owns no key storage: borrowed keys must remain
valid for playback, and evaluation reports active, held/absent and one-shot values.

The runtime continuation shares the dispatcher-established 16-byte callback-slot
stride through `runtime_program.h`. Initialization, update and drawing keep their
separate interior symbols and relocation addends. The view deliberately skips
adjacent storage and does not establish a complete program allocation. Resident
bytes at 0x80063820 contain runtimeUpdateGameMode, renderDrawCurrentMode,
runtimeInitializeSceneGameplay and a retained zero word; source dispatchers
establish the stride and no-argument void callback contract.

`heap_diagnostics.h` reconciles heapDebugReportFreeSpace with HpHdr pointers and
borrowed diagnostic strings across all declaring consumers. Ghidra at 0x80038690
confirms the free-space call; the label is unused in retail code, so source
provides the label contract absent from inferred binary arguments. The scene
consumer's scalar heap view now uses the actual heap type. All seven source units
were built and compared independently before the next changed.

The text-panel continuation shares initialization, release, append, configuration,
color and rendering contracts through `text_panel.h`. All three consumers use
TextPanel pointers; flipbook buffering reuses the checked 0x20-byte TextPanel
layout instead of a duplicate record. The runtime's split symbols remain intact,
with its target-specific cross-symbol view documented. Release has a void result
and retains the separate color allocation. Ghidra at 0x80012730 corroborates the
entry allocation, pointer tables and capacity. An unsigned allocation-size local
preserves the established allocator contract and original instruction selection.
Each producer and consumer matched all 483 baseline objects individually.

The flipbook layout continuation consolidates the drawing and initialization
views into one private checked 0x74-byte model. Its two embedded TextPanel banks
replace a second duplicate panel definition; drawing uses the existing TextImage
record instead of a duplicate image layout. Initialization allocation partitions,
draw bank selection and Ghidra at 0x80025170 establish the drawing allocation
and paired-panel destruction. The unused word at 0x68 remains explicitly
unresolved. The destructor takes a model pointer instead of OSTimer, and redundant
local forward declarations following definitions are removed. All 483 objects
remain identical to the immutable baseline.

The actor-target continuation shares lookup, tracking, messenger-contact mode,
turning and pivot contracts through `actor_target.h` across eight source units.
Target results are borrowed live objects, and turning requires the actor to be
the current object update because movement is accumulated through global state.
Ghidra at 0x80118C80 corroborates destination, orientation and pending movement
use. The turning producer reuses checked ActorScriptState destination/orientation
fields and RuntimeObjectPrefix worldPosition, rather than local position. A trial
using local position was rejected by object comparison and corrected; the
immutable baseline and comparator were unchanged. Slow, medium and fast scale
oscillation wrappers now accept actor pointers, matching their action consumers.
Every source unit was built and compared independently before the next changed.

The EEPROM continuation unifies the queued producer and controller-thread
dispatcher through the checked 0x20-byte EepromRequest record in `eeprom_io.h`.
Ghidra at 0x80003938 confirms command, acknowledgment, result, page, buffer and
size offsets. Request APIs carry RAM pointers rather than integer addresses,
and all five producer/consumer units share fully prototyped transfer and query
interfaces. Read buffers remain borrowed until completion; write payloads are
staged before acknowledgment. Request acknowledgment is not transfer completion.
The microphone configuration message retains a distinct private record because
its command-dependent payload has different semantics despite a similar extent.
The unused local EepromArgs view is removed. Every unit matched all 483 objects
independently before the next source mutation.

The sound-effect continuation consolidates five duplicate player/slot views into
one private SoundEffectPlayer and SoundEffectVoice layout. Ghidra at 0x8002138C
corroborates the 0x28-byte player and 0x50-byte slot allocations; compile-time
size and offset checks cover the shared interpretation. Playback, volume, pan,
lifetime and destruction consumers reuse the same voice record. The trailing
slot word at 0x4C stays explicitly unresolved. `sound_effect_player.h` exposes
an opaque player and documented playback/registration contracts, while audio
initialization retains an actual synthesizer pointer and sound-effect player
pointer. Borrowed definitions and sample data remain live throughout playback;
unregistering does not release voices or audio-heap storage. Both source units
matched all 483 immutable baseline objects independently.

The corrected-asset continuation shares the checked seven-word AssetDescriptor
record and pointer-based load/free contracts through `asset_loader.h`. Ghidra at
0x8002D160 confirms normalized range and correction arguments. Resident bytes
at 0x8009B390 and 0x8009B588 corroborate ROM start/end, segmented range bias,
encoded/decoded sizes and zero correction counts for title and eye-catch assets.
Backdrop consumers use the same descriptor fields. The unused controller-local
asset loader declaration is removed; actor decompression uses the shared API.
The allocating loader requests staging for encoded bytes while the DMA interface
also includes correction records. Capacity for nonzero counts is not established
and is explicitly documented, without changing gameplay or allocation behavior.
All seven source units matched the immutable 483-object baseline independently.

The ROM-I/O continuation types queued read destinations and advancing RAM
cursors as byte pointers, with unsigned cartridge addresses. Ghidra at
0x80037D40 corroborates the queue's request slots and completion processing.
Both cache and actor asset consumers use the same queue interface, retaining
explicit conversions where existing storage still holds target-width addresses.
The PI manager's 200 message slots and blocking read's completion slot now use
OSMesg; the reserved PI backing extent is 0x328 bytes, sufficient for 200 slots.
Fault notification storage likewise uses OSMesg and its entry point uses the
SDK's void-pointer callback parameter. The ROM-I/O deallocator calls explicitly
convert the heap view at the existing byte-oriented boundary. All four source
units independently matched the immutable 483-object baseline.

The text-drawing continuation shares the indexed palette selector and the
sixteen-entry RGBA5551 default palette through `text_draw.h`. The producer retains
a borrowed const-u16 palette pointer rather than an integer; all ten consumers
use the same contract without integer-address casts. Ghidra at 0x8003D580 confirms
the retained drawing-state value, and the TLUT consumer establishes entry width
and count. Palette storage must survive execution of the submitted graphics
commands. The separate built-in graphics-text palette keeps its own symbol.
Tracked player playback start/stop contracts join `player_control.h` across the
producer and all declaring consumers. Runtime's unused secondary input hook now
accepts its borrowed address as a pointer. Every source mutation matched all
483 baseline objects before moving on.

The audio-cue continuation shares state query, bank selection, cue playback,
transition and rate contracts through `audio_cue.h` across all ten consumers.
HUD now agrees with the playback-handle return type, and script rate calls use
the established double-precision prototype. Bank selection and lookup share
reader state and require serialized use. Runtime explicitly converts the ROM
linker symbol at the cartridge-address boundary.

The private RomWindow layout has checked 0x424-byte size, queue and message
offsets. Ghidra at 0x80023388 confirms initialization and the aligned buffer view;
initialization now uses the named fields. Sixteen-byte-aligned reader storage
keeps the full 1024-byte transfer inside the embedded data area. The signed
address comparisons and 0x80000000 empty sentinel remain unchanged. Every source
unit matched all 483 baseline objects before the next mutation.

The sequence-channel continuation consolidates pan, effect, instrument and
modulation views into one private checked EngineChannel record. Producer resets,
controller dispatch and pitch/volume consumers establish sustain, bank selection,
expression, registered-parameter selection, portamento and modulation fields.
The byte at 0xA and three trailing bytes remain explicitly unresolved. Ghidra at
0x80005E14 corroborates fourteen-bit pan at channel offset 8 and the 0x1C stride.
Size and pan/effect/vibrato offset checks guard the interpretation.

The generic SeqChan byte-array view is removed: its accesses crossed the nominal
record boundary to compensate for the sequence header. Consumers now address
the actual channel array, with named fields for controller and note operations.
Pan evaluation and refresh share an EngineSequence pointer rather than mixing
integer and record arguments. The full 483-object baseline remains identical
after each accepted source mutation; no scheduling or arithmetic was changed.

The sequence-voice continuation shares one checked 0x50-byte SequenceVoice
record across engine playback and audio-supplied pool storage. Active/free links,
borrowed instrument and sound pointers, native voice, note/channel/velocity,
sustain deferral, modulation, pitch and envelope state replace the opaque pool
and two private partial records. Unknown words/bytes remain offset-named. Ghidra
at 0x80005C1C corroborates active-list unlinking and free-pool return; note setup,
note-off, sustain and lifetime consumers establish the remaining roles. Native
voice, channel and pitch offsets have target-compatible compile-time checks.

`sequence_driver.h` reconciles registration and output controls across producer
and audio initialization. The synthesizer is a borrowed N_ALGlobals pointer,
and the registered callback now directly satisfies the SDK void-pointer
ALVoiceHandler contract. Most voice offset accesses use the shared fields. The
pitch-ratio expression retains its prior byte views: a typed-access trial changed
IDO scheduling only in that function, and restoring its matching representation
restored all 483 baseline objects. This exception is documented in its Doxygen
comment. No arithmetic, compiler settings or comparison rules changed.

The script-query continuation shares category, exposed kind, item kind, creature
kind and object type queries through `object_script_query.h`. Category constants
retain numeric type groups where semantics are unresolved. Ghidra at 0x80117840
corroborates the category mapping. The unused actor context has a pointer
contract, and the producer and script consumers agree on signed item-kind and
unsigned creature/type result widths without changing caller objects.

`object_audio.h` reconciles the spatial-playback producer and all seven callers.
`item_audio.h` uses the established ItemStatePrefix for tracked sound state; item
registry and physics pass real object/state pointers, preserving original sound
selection and playback ownership. The camera-space nonzero-position requirement
is retained at the interface. All eleven changed source units were built and
compared independently with the immutable 483-object baseline.

### Controller status and VM address boundaries

Controller queries now pass the first `OSContStatus` in the existing four-port
array, matching the SDK interface and the initialization/reset callers. The
creation-argument script query retains its VM address as the established `s32`
N64 ABI value throughout stack operations. The interpreter comparison dispatch
explicitly exposes its context as the byte view expected by the private operator
helper, consistently with the other operator dispatches. These changes preserve
all allocated object bytes, layouts, symbols and relocation addends.

### Heap tail allocation contract

`heap_tail.h` shares the tail allocator's `HpHdr *`, unsigned byte count,
16-bit allocation label and owned payload result across its definition and all
six declaring consumers. Ghidra at `0x800381D4` confirms the label store width
and payload return. The asset loader now declares the shared heap as `HpHdr`
rather than a byte array. Each source-unit change matches the preserved object
baseline. The separate header avoids imposing the regular allocator prototype
on consumers that still retain documented extra-argument matching constraints.
Heap release declarations and their extra-argument call sites remain under audit.

### Heap release ownership and ordinary callers

`heap_free.h` provides the canonical `heapFreeLocal(HpHdr *, void *)` contract:
storage must belong to the supplied heap, NULL is accepted, and released storage
must no longer be used. Ghidra at `0x80038548` confirms two consumed arguments,
the null case and the heap allocation-cursor update. The producer now uses the
shared heap record for that cursor; helper boundaries retain explicit byte views.
All ordinary callers include the shared interface. Asset scratch allocations and
text-image cleanup retain pointer types rather than converting to integer addresses.
The response heap is now an `HpHdr`, with pointer-typed owned backing storage;
its existing size, alignment and BSS placement match the preserved baseline.

Five source units still require legacy release declarations: keyed and node pools,
actor model motion, particle effects and the word map. Their existing unused
third arguments remain documented matching exceptions rather than being folded
into the two-argument public contract. The unused controller declaration was removed.
Every changed source unit passes strict comparison against all 483 baseline objects.

### System-menu links and callback contracts

The private `MenuState`, `MenuList` and `MenuItem` records now retain typed
menu links, history, selected items and callback signatures. List callbacks
receive their menu, selection callbacks receive the signed item value, and
detail callbacks take no arguments. Allocation and linked-list operations
use `MenuItem *` throughout. Menu construction, setting synchronization and
drawing establish the confirmed-row index, first visible index and optional
confirmation-sound suppression flag. Size checks are supplemented with offset
checks for selection, timing, item links and submenu links. Ghidra at
`0x8011B7F8` confirms the item-chain and successor offsets.

`system_menu.h` shares the `Gfx **` drawing cursor contract with its caller;
the unused controller declaration is removed. Private records stay local.
Unsigned byte text accumulation remains a recorded matching constraint: both
signed-character interface trials changed allocated code and were rejected.
All retained changes match the preserved 483-object baseline.

### Scene, title and microphone-status drawing cursors

`backdrop_draw.h`, `title_screen.h` and `hud_draw.h` provide canonical `Gfx **`
drawing cursor contracts across definitions and every active declaring caller.
Unused declarations were removed. The title drawing dispatcher and its menu
renderer now retain pointer cursors throughout, including their shared caller
in `main.c`. Ghidra at `0x80033B78` confirms the fixed/scrolling backdrop dispatch;
the C drawing implementations establish the cursor mutation contract.

The four title-menu label tables are signed 32-bit arrays rather than byte arrays.
ROM data at `0x8009B464` contains `[0, 1, -1]`, followed at `0x8009B470` by
`[2, 3, 6, -1]`, at `0x8009B480` by `[4, 5, 6, -1]`, and at `0x8009B490`
by `[7, 6, -1]`. Their declarations retain those lengths and read-only ownership.
The renderer borrows the arrays and their negative terminators. All retained
source-unit changes pass strict comparison against the 483-object baseline.

### Script item references and toolbox replacement

`item_object.h` shares item destruction and material animation/frame contracts
across every definition and declaring caller. Script handlers retain resolved
objects as `u8 *` and the established VM address as `s32`, separating live RAM
pointers from script integer values without changing control flow. Replacement
of an item kind no longer converts the pointer to an integer for destruction.

`toolbox_items.h` shares bonus and purchase replacement contracts. These receive
a live initialized item pointer, retain its handle, and request destruction of
the previous item. Ghidra at `0x8002F2F0` confirms the previous-handle lookup,
destruction request and replacement slot operation. Existing validity and
distinctness preconditions remain explicit. Every changed source unit passes
strict comparison against the preserved 483-object baseline.

### Heap measurements and player friendship progress

`heap_diagnostics.h` now shares free-space measurement using the canonical
`HpHdr *` and unsigned largest-block output. Validation failure can leave a
partial maximum, which the contract documents. Ghidra at `0x800385D0` confirms
the unsigned block-size comparison and initialized output. The menu uses a
matching unsigned result, converting only for its retained signed display format.

`player_friendship.h` shares milestone lookup, award and display-title interfaces.
Awarding uses `PlayerControlPrefix *`; title lookup borrows a const player record
and returns a borrowed persistent string. Ghidra at `0x801125AC` confirms the
level byte at `0xF6`, milestone word at `0xF8`, and no repeated award. Both menu
and script consumers share the declarations. All affected source units match
the preserved 483-object baseline.

### Scheduler task and completion-message views

Audio and graphics workers now receive submissions and completion events through
`OSMesg` storage and the shared queue fields. Active, suspended and waiting tasks
use `SchedulerTask *`; SDK descriptors, command buffers and completion replies
use that record's established members. Ghidra at `0x80040200` confirms the
graphics command, reply, RSP and RDP queues and the task descriptor offset.
Queue offsets now have target compile-time checks alongside task size and offsets.

All three scheduler threads use the SDK `void *` entry signature, removing
function-pointer casts at registration. Typed task/context views preserve the
original worker sequencing and borrowed-resource lifetimes. Client notification
traversal now uses `SchedulerClient *`; framebuffer waiting uses typed scheduler
and task parameters. The existing public client registration and queue-access
interfaces still need their separate producer/consumer audit. All retained
changes pass comparison against the preserved 483-object baseline.

### Shared scheduler initialization and event registrations

`scheduler.h` shares scheduler initialization, typed audio/graphics submission
queues, client registration/removal and reset queries across definitions and
all declaring callers. Registrations use `SchedulerClient *` and retain their
queues until removal; pending notifications remain queued after removal.
Ghidra at `0x8003FF8C` confirms the client link at offset zero and the scheduler
list at `0x668`. Framebuffer waiting now uses a real temporary client record.

Runtime event storage is an `OSMesgQueue`, its registration a `SchedulerClient`,
and its borrowed graphics submission queue an `OSMesgQueue *`, preserving BSS
layout and symbols. Controller registration uses the same shared record while
retaining its stack-top symbol identity. Audio worker storage retains a queue
pointer at its established offset. Legacy runtime/audio entry points retain
explicit integer-address conversions at their existing boundaries; the graphics
initializer interface still needs its producer/consumer audit. Every affected
source unit passes strict comparison against the preserved 483-object baseline.

### Graphics task submission and microcode views

`graphics_tasks.h` shares initialization and submission using scheduler queues,
borrowed framebuffer pointers and the shared `SchedulerTask` pool. Task-pool,
queue, output-buffer, microcode-stack and yield-buffer globals now retain pointer
types. Pool indexing uses the checked task size, and FIFO end addressing uses
the output buffer's 64-bit word type. Completion queues remain borrowed through
task completion; command storage and framebuffers have explicit lifetimes.
Ghidra at `0x8003C500` confirms pool and working-buffer allocation sizes.

The private microcode descriptor and its data-pointer interior view have size
checks. ROM data at `0x8009D41C` confirms two sixteen-byte descriptors with data
pointers at offset four, FIFO flags of one and retained data-size metadata.
The interior symbol at `0x8009D420` remains separate to preserve strict relocation
addends. Its unused trailing bytes are explicitly stride storage spanning the
next descriptor, rather than asserted unknown semantic fields. All declaring
callers now share the graphics API; the unused controller declaration is removed.
Every retained source-unit change matches the preserved 483-object baseline.

### Keyed-pool entry and predecessor contracts

Keyed lookup, removal and value-query APIs now share `KeyedPool *`,
`KeyedPoolEntry *` and writable predecessor-pointer results. Entry lookup with
a value returns a borrowed entry pointer; stored values remain generic signed
32-bit values, with explicit pointer conversions in object/cache consumers.
Ghidra at `0x80041D98` confirms the removed value, chain link and relocation
behavior. Existing documentation retains entry invalidation on pool mutation.

The cache and object-handle registry use the shared header. Node and keyed pool
allocation use the canonical heap pointer. Collision removal and response
insertion retain their documented unused extra arguments, while their pool
and traversal-value types agree with the implementation. The response registry's
reserved trailing storage remains unchanged and recorded for further evidence.
All affected units match the preserved 483-object baseline, including strict
relocation addends and the legacy destruction-call argument setup.

### Cache storage and motion archive contracts

ROM cache entries now expose owned byte buffers and typed allocation/removal
interfaces. The cache heap backing region is an owned pointer; its size and
index/read offsets remain checked. Motion cache readers and their runtime and
playback consumers share a documented header and the same cache-pointer type.
Ghidra at 0x80017228 confirms the archive-relative base adjustment. That result
remains a target-width integer because it is an address bias, potentially before
the loaded allocation, rather than a pointer to an allocated object.

Motion cartridge addresses use an explicit numeric linker-address boundary.
The analogous texture expression changes IDO instructions and relocations; its
original representation is retained with a matching exception in the ledger.
Every retained unit matches the immutable object baseline.

### Gameplay heap and small owned effect records

Attached effects, caves, scene regions and linked effects share the canonical
game-heap declaration through game_heap.h. Allocation callers pass HpHdr pointers.
Linked-effect initialization stores a typed allocation in the object's pointer
view and checks its private 0x58-byte motion-state size.

Scene regions use a private checked 0x10-byte record for world X/Z half-extents,
player-only highlight state and opaque scene association. Ghidra at 0x8001F318
confirms the companion-first occupancy precedence and byte flag. The scene-data
value remains opaque; no unsupported pointer ownership is inferred. Padding is
retained. These changes preserve all allocated bytes and strict relocations.

### Audio callback and terrain diagnostic cell contracts

The sound-effect lifetime handler uses the SDK ALVoiceHandler contract directly:
ALMicroTime return and void-pointer client argument, with an ALPlayer view at the
client-data boundary. Ghidra at 0x8002116C confirms the registered player's
client-data access and 5750-microsecond interval. No callback cast is required.

Terrain diagnostic cells have a checked private 0x18-byte state: enable flag,
four corner matrix indices and RGB diagnostic color. The creator, visibility
controller, terrain updater and draw routine share that record. Ghidra at
0x8002B380 confirms the corner-index stores. Root state remains a separate
four-byte grid dimension, and both allocations use the object pointer view.
The item initializer likewise stores its allocated byte state as a pointer.
All retained changes match the immutable object baseline.

### Actor part lookup and position query contracts

Actor-part lookup, default-part lookup and world-position queries share
actor_parts.h with borrowed pointer results and explicit output-array contracts.
Ghidra at 0x800164AC confirms preferred-part lookup, fallback and three coordinate
loads. The world-position query requires a valid default part when the preferred
part is missing. Its definition and action-script callers now use actor pointers.

All ordinary part-lookup callers include the shared header. The messenger
default-part call retains its integer compatibility declaration because the
canonical pointer prototype changes ROM bytes. Ground-shadow default-part lookup
retains an unused second argument from the ROM. Both exceptions are recorded
rather than hidden by casts or changed compiler flags. All accepted units match
the preserved object baseline.

### Shared affine matrices and hierarchy queue boundaries

Hierarchy traversal, actor motion and matrix math now use one checked Matrix34
layout: 0x30 bytes with translation at 0x24. The traversal global is the full
affine matrix rather than a three-component vector. Saved traversal entries use
the same type; Ghidra at 0x80007F34 confirms the 0x30-byte stack copy stride.
The matrix identity declaration agrees with its producer, and scale-only calls
pass the linear-basis storage explicitly. All changed units match the baseline.

The private buffered cartridge reader passes its typed completion queue and
message fields to libultra. Their 0x418 and 0x430 offsets are now checked, in
addition to the existing total-size and buffer-offset checks. Ghidra at
0x800076E0 confirms these SDK boundary addresses. Optional object-state release
uses the pointer union view, preserving its existing ownership and invalidation
contract.

### Search-tree root and comparator contracts

Search-tree initialization and independent-root search share search_tree.h.
Comparators receive a TreeNode and target-width key directly; object and item
comparators explicitly interpret that key as an object address. Registration
no longer casts function pointers to integer addresses. Ghidra at 0x80040F20
confirms the comparator field at 0xC and initialization extent.

The root-search primitive uses a writable TreeNode pointer and TreeCompare
callback throughout its callers. Independent-root lookup returns a borrowed
TreeNode pointer. Existing paired detach/link statements and traversal behavior
remain unchanged, and all affected units match the strict baseline. Generic
stored keys remain signed target-width values rather than invented object types.

### Nearest-target context and title message tables

The object search-tree global uses SearchTree rather than an unbounded byte
array, so its initialization agrees with the canonical interface. Existing
legacy insert/remove boundaries retain explicit byte views. Nearest-target
search owns a complete Matrix34 at 0x18, replacing a vector plus padding.
Its 0x54-byte context and cone at 0x48 are checked. Ghidra at 0x80009ED4 confirms
the identity/view-matrix destination and following search fields.

Four title-screen message tables are borrowed text-pointer arrays with fixed
entry counts (7, 5, 7 and 4), agreeing with the existing text-panel input contract.
Ghidra bytes at 0x8009B408 through 0x8009B463 confirm the pointer table extents;
no text encoding or ownership beyond the panel's established contract is inferred.
All changes match the immutable object baseline.

### Panel-owned text image tables

TextPanel display-order and circular-entry tables now contain TextImage pointers,
agreeing with the builders and destruction interface. Panel initialization,
drawing, replacement and display-order copying share the typed tables; the
legacy cursor renderer retains an explicit encoded-byte boundary. Ghidra at
0x80012D20 confirms entry replacement, destruction and display-order copying.

Voice-command dictionary names retain an unsigned encoded-byte view with an
explicit conversion from the dictionary character-string interface. A char
global declaration changes generated byte loads, so the retained representation
is documented at the declaration. All changed units and header dependents match
the immutable object baseline.

### Animated texture state and allocator return contracts

Actor animated textures use a private checked 0x4C-byte AnimatedTextureState:
up to three texture IDs, playback rates and established ValueTrack records,
followed by two display-list views and a per-buffer command capacity.
The display-list offset is checked at 0x40. Initialization, evaluation, binding
and release share this record. Ghidra at 0x80016658 confirms allocation size,
zero initialization and object-state pointer storage.

The local allocator compatibility declaration now uses its established void
pointer result. It remains unprototyped solely for documented unused trailing
arguments in other callers. Animated-state allocation uses a typed pointer,
and the release call retains its original unused third argument. All changes
preserve allocated object bytes, layout and strict relocation identities.

### Current-area maze root ownership

The current maze root is a shared MazeNode pointer declared in area_maze.h,
owned by the area hierarchy and cleared on destruction. Its producer and actor
movement consumer agree on the type. Ghidra at 0x8010D540 confirms the root
allocation, bounds and depth stores. An unused integer declaration in companion
code and a redundant producer declaration were removed.

Actor movement retains explicit byte views of the established x/w/z/d bounds:
direct members change IDO output, so that exception is documented in its Doxygen
contract and ledger. Scene-cell initialization uses the existing byte-address
callback boundary explicitly. Hierarchy-root creation now has the producer's
pointer return type. All affected units match the preserved object baseline.

### Maze block resource interfaces

Maze block placement and replacement share area_maze.h with all five scene
callers. Template arguments are borrowed byte-addressed hierarchy resources;
replacement terrain is an established MazeGrid pointer stored in MazeNode.
Ghidra at 0x8010DC88 confirms the terrain assignment at 0x44 and template
creation/registration. The template is consumed during creation; terrain must
remain live while the region uses it.

Scene resource blobs retain explicit MazeGrid views rather than inventing
complete packed resource declarations. The former integer template parameters
and inconsistent local scene prototypes were removed. Each source unit was
rebuilt and compared independently against the preserved baseline.

### Word-map ownership and lookup interfaces

Motion and actor registries share WordMap initialization, insertion and
destruction declarations. Destruction takes the established map record directly,
releases its owned slot allocation and clears its fields. Ghidra at 0x80048D44
confirms those ownership operations. The original unused deallocator argument
remains explicit and documented.

The canonical two-argument lookup declaration lives in word_map_lookup.h and
is shared by its producer and motion caller. Actor part-index lookup retains
its already recorded four-argument compatibility declaration because the ROM
supplies two unused trailing integers. The focused lookup header lets that
caller share ownership APIs without conflicting declarations. No compiler
flags or call argument setup were changed; all affected units match the baseline.

### Custom-font descriptor and ink metrics

Custom fonts now expose checked FontImage and FontGlyph layouts in a focused
header. FontImage is 0x10 bytes with its metrics pointer at 0xC; each glyph is
eight bytes. Glyph fields at 4 and 5 are signed left/right ink boundaries,
replacing misleading width/height names. Ghidra at 0x8003F554 confirms the
signed right-boundary read, glyph stride and fixed-cell fallback.

Left/right queries share typed FontImage pointer contracts in font_image.h.
Existing integer address boundaries in unrelated font builders remain explicit.
Glyph bytes 6 and 7 retain offset names: no consumer has established their role;
recovering resource generation or an additional reader is the next evidence
needed. Their original storage is preserved. All changes match the strict
object baseline and preserve the original unsupported-character constraints.

### Encoded byte-string copy contract

All stringCopy callers share string_bytes.h with the producer: writable byte
destination, borrowed const byte source and the original destination pointer as
the return value. Ghidra at 0x80049A78 confirms terminator-inclusive copying and
the destination return. The contract requires sufficient destination capacity
and nonoverlapping storage.

Profile, message-bar, companion-help and presentation callers use the same
declaration. The presentation caller's former void return and integer source
declaration were removed; its packed address source has an explicit byte view.
The profile's ASCII literal likewise crosses the encoded-byte boundary
explicitly. Every affected unit preserves allocated bytes and strict relocations.

### Messenger flight state contracts

Messenger projectiles use a checked private 0x40-byte MessengerFlightState.
Its established ValueTrack is at 0x10, velocity at 0x1C and copied ScriptMessage
at 0x28. Fields name the behavior mode, remaining transition time, homing orbit
phase and linked recipient handle. Initialization, creation, frame updates,
position advancement and destruction share the record.

Position advancement accepts the same object byte-pointer contract as its
caller and uses the object's local position and flight velocity fields.
Ghidra at 0x80025FAC confirms the object-state reference and three velocity
components multiplied by frame duration. Existing homing phase and countdown
consumers establish their field roles. The pending message remains a value
copy, not borrowed payload storage. All changes preserve strict object matching.

### Text-image creation ownership interface

All textImageCreateFromText declarations now share text_image_create.h:
a writable TextImage output slot, encoded text and an optional borrowed
FontImage. Empty text produces a null image; generated images own their
descriptor and pixels until textImageDestroy. TextImage and its target-width
queue handle now have size checks and field documentation.

Ghidra at 0x8003DDC0 confirms custom-versus-system font dispatch. Ten caller
units were rebuilt and compared separately. Existing integer-address slots
and encoded string representations cross the interface through explicit pointer
views rather than inconsistent function prototypes. Those representations
remain recovery targets; this interface change does not claim complete recovery
of every caller's image table. All retained units match the strict baseline.

### Door text and owned rendering allocations

Door access descriptions use character storage at the sprintf boundary and
an explicit encoded-byte return view. Passage access is read through the
established PassageState field. Ghidra at 0x8001C35C confirms the state selector,
shared output buffer and borrowed return lifetime.

Shared and area rendering allocations are u8 pointers declared in
render_assets.h, with their original linker addresses and ownership lifetimes.
Loading uses the canonical ROM DMA declaration and a RAM destination pointer;
segment setup consumes that same allocation. Unused integer redeclarations in
controller code were removed. All changed units match the preserved object
baseline; command-buffer address representations are not claimed recovered.

### Text cursor image interface

text_draw.h now declares textDrawImageAtCursor with the shared TextImage
descriptor. Ghidra at 0x8003D5EC confirms width and height at offsets 4 and 8;
the producer uses those fields directly. All seven caller units share the
prototype, with explicit descriptor views at legacy byte/address slots. Pixel
storage is borrowed until the emitted drawing commands finish. Each unit
preserved the strict object baseline before the shared declaration was added.
Legacy image table representations remain separate recovery targets.

### Text-image texture preparation

The texture and rectangle preparation APIs share TextImage parameters through
text_draw.h. The renderer and flipbook consumer no longer cast descriptors to
byte pointers at these interfaces. Ghidra at 0x8003DD84 confirms the complete
image bounds from the descriptor's width and height. Rectangle preparation
uses the same descriptor fields and pixel storage; its image parameter is
nonnull even though a later internal check tests it. Both source units
preserve strict object matching. The header documents borrowed pixel lifetime
through completion of drawing commands.

### Text-image destruction and retained slots

text_image_create.h now provides the canonical TextImage destruction contract
for every caller, including null acceptance and completion of pixel users before
release. Ghidra at 0x8003F650 confirms ownership of the descriptor and pixels.
Window page/prompt slots, message-bar handles and the collection score image
are descriptor pointers rather than integer addresses. Window prompt width uses
the shared descriptor field. The message bar uses the image view of its existing
queue handle union. Changing only the destruction parameter with integer slots
altered generated code; typed storage and the canonical parameter together
preserve the strict object baseline. Remaining legacy byte image tables use
explicit descriptor views and remain recovery targets.

### Text-window image-slot overlap

text_window_images.h is the shared declaration for page images, the prompt
image and the selected buffer. Drawing uses five row pairs; initialization
clears 0x28 bytes starting at 0x800F1080. The prompt at 0x800F10A8 aliases
the first slot of the sixth pair in the cleanup loop's extended view. This
view is retained to preserve the original cleanup accesses rather than claiming
six independent page rows. Ghidra at 0x8002BF48 confirms two image slots per
row and the selected-buffer index; 0x8002C240 confirms cleanup ends at
0x800F10B0, covering all twelve slots. The final cleanup slot at 0x800F10AC is
unresolved: its producer or allocation-map evidence is needed before assigning
a separate role. Do not shorten the cleanup loop or move these linker symbols.
Unused message-bar and collection-score redeclarations were removed from
nonconsumers, including their stale integer views in terrain debug code.

### Shared text drawing state and rectangle interface

text_draw.h now owns the cursor-position, screen-area, display-list-position
and scaled rectangle drawing declarations. Their contracts document coordinate
origins, inclusive destination bounds, writable cursor outputs, source offsets
and positive floating-point scales. Ghidra at 0x8003D764 confirms both scale
parameters and descriptor width/height consumption. Repeated prototypes were
removed from eight source units after each unit preserved strict object matching.
The shared rectangle declaration retains the existing source-width and numeric
range preconditions; no clipping or command-generation behavior was changed.

### Text drawing state API consolidation

The shared text drawing header declares initialization, drawing-session setup,
reset, color, font, scale, spacing, default bounds and formatted drawing. Nine
source units use these declarations instead of local copies or implicit calls.
The scale interface keeps both f32 parameters; Ghidra at 0x8003D598 confirms
the two stored scale values, and drawing consumers establish their float use.
Initialization and formatting preconditions and the original variadic stack
layout limitation remain documented at the shared boundary. Header changes and
each local declaration removal preserve the strict object baseline.

### Runtime clock return contract

runtime_clock.h declares the resettable millisecond and whole-second readings
as unsigned 64-bit values, matching the runtime's counters. Ghidra at
0x8000B0FC confirms the paired-word millisecond return. All consumers use the
canonical declaration, including actor movement and terrain maze callers that
previously declared signed results. World-map focus transitions no longer pass
unused target/point arguments to the zero-argument clock getter. Their arguments
were simple pointer reads with no side effects; removing them preserved strict
object matching. The unused save-progress declaration was removed. Existing
32-bit snapshots, explicit truncation and mixed-width elapsed-time arithmetic
remain unchanged and are not claimed recovered by this interface correction.

### Item name and help-text interfaces

item_text.h declares the registry's borrowed persistent name, class suffix and
help-text results. Callers no longer declare name/help pointers as s32. The
script caller explicitly converts the name to the VM's existing target-width
string address, preserving that separate VM representation. Ghidra at
0x80114E74 confirms alternate-name selection with default fallback; the existing
ItemText entries establish persistent name/help storage. Null results for missing
text and unsupported selectors remain documented. All five producer/consumer
units preserve strict object matching; no string ownership or lookup behavior
changed. The class suffix retains its established encoded-byte pointer type.

### Actor inspection text and dictionary-name VM boundary

actor_text.h provides pointer-returning actor name and description contracts.
The producer loads string pointers from the existing registry interior views
and names its alternate pointers accordingly. Ghidra at 0x8001E680 confirms
alternate-name selection and default fallback. Companion and VM consumers
share the declaration; the VM explicitly retains its integer-address boundary.
The dictionary-name VM operation also uses voice_recognition.h and its u16
index contract. Retaining an integer bit-mask expression at the narrowed call
changed generated code; an explicit u16 cast expresses the same low-bit
selection and matches the preserved object. All retained changes match strictly.
Registry-wide field recovery remains separate from these string interfaces.

### Actor part-selection interfaces

actor_part_config.h declares configured attachment/default part IDs and the
borrowed ground-reference list. The list returns const s32 pointers rather
than integer addresses; both clearance consumers read signed 32-bit entries
and stop at a nonpositive value. Ghidra at 0x8001E5F0 confirms the registry
pointer load at the 0x54-byte kind stride. Attachment/default queries keep the
producer's s32 kind parameter, correcting narrower local declarations without
changing the existing u16 object-kind loads. Both source units preserve strict
object matching. The registry interior symbol views remain intentional until
full kind-table recovery establishes a matching shared record representation.

### Actor-kind creation record

The private ActorKindConfig record replaces the generic SpawnSpec in actor
creation. Its 0x54-byte size and key offsets are checked at target compile time.
The resident kind-one record at 0x8008801C, interior symbol addresses, lookup
functions and Ghidra at 0x8001E1C4 establish text pointers, hierarchy template,
ground-reference IDs, animated-texture parts, visibility radius, collision
dimensions, shadow segment count, reference IDs, initial flags and resource
index. Creation now names those fields directly instead of bounds/reserved
arrays. Existing interior linker views remain unchanged for strict relocations.
The marker query identifies 0x10/0x14 as marker depth/height (see below).
Dimension 0x34 only has a confirmed copy into actor state 0x208; find additional
consumers before assigning semantics or changing its representation. The record remains local
because only creation currently consumes this complete view; no unrelated
records were merged. The changed object matches the immutable baseline.

### Actor-kind marker dimensions

The actor marker query reads ActorKindConfig.markerHeight and markerDepth
at 0x14 and 0x10, with target-compatible offset checks. Ghidra at 0x8001E730
confirms the two field addresses and the three-float output order. The marker
consumer scales these values by target Y/Z scale and style height/depth,
establishing their role as unscaled placement dimensions. The query now uses
the recovered record instead of negative offsets from a byte cursor, preserving
the original interior-base relocation addends. actor_part_config.h declares
the shared output contract. Both source units and dependent units match strictly.

### Actor creation and replacement contracts

actor_create.h is the canonical interface for actor-kind creation and script-
preserving replacement. All 24 producer/consumer units share its floating-point
pose/scale arguments, borrowed script configuration, retained script argument
and pointer result. Wilderness Camp no longer declares an integer result or
casts its script resource to an integer. Front Yard and fishing-scene script
parameters likewise use the producer's pointer contract. Ghidra at 0x8001E568
confirms replacement transfers script state, clears the original reference and
requests original hierarchy destruction. Header contracts state runtime-managed
hierarchy lifetime and script resource lifetime. Every unit was rebuilt and
compared before proceeding to the next; all preserve strict object matching.

### Item creation interface

item_object.h now declares creation alongside destruction and material
selection. All sixteen producer/consumer units share the five f32 position/
orientation arguments, s32 kind and u8 object-pointer result; generic void
results in Front Yard and Abra Shop are removed. Ghidra for 0x80113944
confirms world/local position writes, X/Y rotations, negative-Y ground placement
and registration. Its inferred argument list is inaccurate, so source call sites
and the matching producer remain the authority for the six-argument ABI.
The shared Doxygen preserves initialization, lifetime, item-limit and target
address portability contracts. Each changed unit matches the strict baseline.

### Typed item creation and material playback state

Item creation initializes ItemStatePrefix fields for kind, flags, lifecycle
state, mode, owner, base radius, bounce count and search priority, and links
the named registry node. Access widths and initialization order are unchanged.
The partial bytes at 0x40 are now the existing ValueTrack, including the
retained material frame at 0x48. New offset checks protect both locations.
Initialization, animation advancement, material-animation changes, drawing and
fixed-frame selection share this view. Ghidra at 0x801145D0 confirms track
initialization at 0x40 and the active flag at 0x3C. All dependent objects match
the preserved baseline. The prefix remains a partial view, not an allocation
size; raw object-transform and later kind-specific state views remain separate
recovery targets.

### Item material-control contracts

Material animation, fixed frame, draw mode and tint setters now use typed
ItemStatePrefix locals. The material-track and RGBA fields have one shared
interpretation, with the original operation order and access widths retained.
item_object.h declares handle-based draw-mode and tint setters; the script
caller no longer declares alpha as s32 when the implementation accepts u8.
Ghidra at 0x80114654 confirms four byte stores at 0x4C through 0x4F and
missing-object no-op behavior. The caller already retains byte-sized channel
values, and the canonical declaration preserves strict matching.

### Item interaction-response and transformation contracts

item_response.h declares kind-response lookup, live-handle response lookup
and transformation. Ghidra at 0x801146A4 confirms byte-sized configured
responses; the script caller now shares the u8 result rather than declaring
s32. Live lookup uses typed item flags and kind fields. Six source units
share the transformation contract and its returned-handle lifetime rule.
The actor-script destruction path no longer passes an unused third object
argument; the two-argument call preserves strict matching. Existing numeric
response categories remain explicit pending complete semantic evidence for
each category. All affected units preserve the object baseline.

### Item collision-key and lighting ownership

Item destruction and state release use typed collisionKey and lighting fields;
lighting command construction reads the same prefix's kind and buffer views.
Ghidra at 0x80113464 confirms collision unregistration at state 0x50, release
of the lighting base at 0x60 and release of the full state allocation. The two
lighting pointers are alternate views into one 0x70-byte allocation, not two
independently owned allocations; the shared field documentation now states
that ownership and a new offset check protects collisionKey. The allocator's
lighting-buffer index is named explicitly. Full state allocation remains 0xF0;
the 0x6C prefix must not determine allocation size. The later lighting and direction-animation relationship is now recovered
through the shared paired-pose view described below. All retained changes preserve strict matching.

### Shared direction animation and item lighting poses

direction_animation_types.h shares the 0x7C-byte paired-pose animation layout.
The former generic Spin fields are pitch, yaw and their angular rates. Poses
remain raw 0x28-byte storage with checked size, preserving enclosing layout;
item rendering explicitly views the selected pose as SDK Lights2. The item
partial view places DirectionAnimation at 0x70, with poseIndex at item 0xE2.
Ghidra at 0x80032448 confirms mode at animation 0x70 and the alternating
selector at 0x72. Reset copies the same default pose into both slots, and
pose-angle writes update packed directions consumed by the renderer.
The provider and item renderer preserve strict object matching. The private
partial item view excludes the tail; the 0xF0 state allocation remains explicit.
The subsequent callback-contract recovery below resolves the legacy K&R
advancement calls without altering the ROM.

### Direction-animation callback argument forwarding

Both motion callbacks now explicitly accept and forward the selected pose
index to a normal two-argument directionAnimationAdvancePose declaration.
The shared callback typedef matches the dispatcher table. Disassembly confirms
the dispatcher supplies the selector in a1; retargeting's advance branch and
timed motion's advance branch preserve that incoming register through the call.
Ghidra at 0x80032148 omits the forwarded argument in its inferred signature,
so register flow and the unchanged object bytes provide the stronger evidence.
Explicit parameter forwarding produces exactly the original code, removing the
K&R missing-argument UB rather than retaining an unnecessary compatibility
exception. The former UB comment is replaced by the valid pose-index contract.

### Shared direction-animation playback interface

`direction_animation.h` supplies the reset, update, next-pose and timed-motion
contracts used by actor motion, item motion and action commands. The timed-motion
provider and both producers now share `DirectionAngularRates`: two floats for
pitch and yaw rates in degrees per second, with a checked eight-byte size.
Ghidra at 0x80032384 confirms two records are consumed at an eight-byte stride;
the provider copies their values and retains no pointer to the caller's storage.
The existing byte-storage animation parameters retain the enclosing actor/item
views while their documentation requires a complete initialized animation.
Each of the four source-unit changes independently preserves all 483 objects.

### Direction-animation field consumers

The playback provider now uses the shared selector, mode, duration, remaining-time
and angular-motion fields. Timed motion reads the selected pose through the shared
pose array instead of a shifted local frame type. The dispatcher explicitly
converts its signed mode field to u16, preserving its original unsigned load.
All 483 objects remain identical. Reset retains a documented word-aligned
`DirectionPoseCopy` view: substituting the byte-aligned shared `DirectionPose`
changes IDO's aggregate-copy code and the ROM layout. This is an alignment
constraint on the copy expression, not evidence of a different record layout.

### Packed normal and opposite-direction angle contracts

`vector_direction.h` consolidates packed normal conversion and look-angle APIs
across their providers, actor movement, queued item responses and direction
animation. Floating directions use three-component float arrays; packed normal
inputs use signed bytes. Direction animation no longer declares incompatible
local vector-pointer prototypes or unprototyped angle calls.
Disassembly of 0x8000CA60 proves a1/a2 remain intact while a0 is replaced with the
negated local vector before the call to 0x8003BFA0. Explicit pitch/yaw parameters
and forwarding preserve all 483 objects, recovering outputs omitted by Ghidra's
one-argument inference. The function documentation now describes those outputs.
Each affected source unit was rebuilt and strictly compared before the next edit.

### Direction construction and packed default slots

The shared vector-direction header now also declares unit-direction construction
for its provider, direction playback and particle effects. Its pitch is positive
downward, and yaw runs from positive Z toward positive X; all producers use the
same three-float output contract. Ghidra at 0x8000CE9C confirms the component
construction but overstates its parameters under the floating-point ABI; the
source definition, callers and unchanged objects establish the C prototype.
The default-pose interior symbol at 0x8009A6C0 now has a packed sixteen-byte slot
view rather than four purported floats. Only its first three signed bytes are
decoded; trailing bytes remain uninterpreted. The original interior symbol is
retained, preserving strict relocation addends.

### Queued object response records and item impulse contracts

`object_response.h` defines the checked twelve-byte response allocation seen at
0x8001C764. Its source handle, halfword flags, kind byte and four-byte payload
match creation, dispatch and release sites. Flag bit zero selects an owned pointer
payload; kind zero uses an inline impulse. Byte 0x07 and trailing bytes in packed
direction slots remain explicitly uninterpreted. The shared four-byte impulse
preserves the producer's by-value ABI and signed interpretation of components
and strength. Collision production and response consumption share this contract.
The item header now supplies the canonical float-vector velocity-change and
pointer-based visual-motion interfaces. The response handler explicitly converts
its legacy integer callback address at that boundary. The provider's extra
mode-change argument remains the previously documented ABI constraint.
Each source-unit change preserved all 483 objects and strict relocation addends.

### Object-kind hooks and response callback lifetime

The shared `ObjectKindCallbacks` layout replaces the hierarchy unit's private
24-byte table view. Existing destroy lookup at table+4 and response lookup at
+0x14 establish the two previously anonymous words as lifecycle and response
hooks. Initialization, update and drawing consumers establish the other slots.
Size and response-offset checks protect this shared layout.
Response dispatch and clearing now use `ObjectResponseHandler` with a typed
borrowed record, including the optional callback parameter. Ghidra 0x8001C5E4
confirms callback invocation precedes owned-payload and record release. The target
still uses its explicit original 32-bit object-address representation. The typed
strided response interior view preserves D_80078154 and strict relocation addends;
it is not an independently allocated record array. All changed units match.

### Typed response lifecycle and allocation boundary

Response callbacks and hierarchy dispatch/discard now consistently accept object
pointers. The shared lifecycle declarations replace integer-address declarations
and casts in the hierarchy caller. Handle and flag consumers use the recovered
runtime-object prefix, and creation checks the typed hierarchy root's flags.
Creation returns `ObjectResponse *`; attachment accepts that owned record and
converts its address only at the keyed pool's existing integer-key boundary.
The queue producer retains a typed pointer through attachment. Ghidra 0x8001C6BC
confirms the per-kind hook selection and clearing call; the original register ABI
is unchanged. Separate provider, caller and allocation passes each preserve all
483 objects, including strict relocations.

### Door creation and passage interface

`door.h` provides the creation, access-state mutation and borrowed-description
contracts to four scene producers, the script command and companion description
consumer. The provider reuses `PassageState` for its 0x10 allocation and access
mutation, and `RuntimeObjectPrefix` for kind, state ownership, position and handle.
Ghidra at 0x8001C150 confirms the four state words and object initialization sites.
Description buffers remain distinct shared storage with separate invalidation
contracts. Existing signed/unsigned text pointer conventions remain explicit.
The ten-entry registry capacity is a caller precondition, not a newly added check.
Each provider and caller mutation independently preserves all 483 objects.

### Shared runtime object lifecycle contracts

`object_lifecycle.h` consolidates default creation, hierarchy-root ownership,
active registration and destruction across nineteen source units. Previously
void declarations of pointer-returning root and registration helpers now agree
with their provider. The marker's integer address converts explicitly at the
pointer interface. The former mixed-return-type portability note is obsolete.
Ghidra at 0x800084E8 confirms root assignment and flags but infers void; the
provider, value-consuming callers and unchanged objects establish the pointer
return contract. Ownership comments require removing retained active/sibling
links before destruction. Each source unit was rebuilt and all 483 objects
strictly compared before proceeding to the next mutation.

### Shared object hierarchy template layout

`object_template.h` defines the checked 0x38 template copied by runtime creation:
scale, rotation, local position, radius, sibling/child definitions, display list,
kind and flags. The runtime object's shared prefix has the same leading fields,
but the template remains a separate borrowed definition rather than a live object.
Ghidra at 0x80008160 confirms the complete 0x38 copy and kind-selected initializer;
creation replaces flags and establishes runtime handle and ownership state.
The default, cave and scene-region copies now use this layout, and hierarchy
cloning uses its sibling and child fields. `ItemModel` aliases the same evidenced
layout instead of maintaining a duplicate structure. The shared-header dependent
build and each source mutation preserve all 483 objects and relocation addends.

### Typed actor hierarchy cloning

The clone provider accepts `ObjectTemplate *` and traverses typed sibling/child
links. Actor-kind configuration now stores the same borrowed template pointer,
and its caller uses the shared lifecycle declaration. Ghidra at 0x800083C0
confirms recursive child cloning, sibling traversal and common-root assignment.
The contract separates this operation from transform-buffer allocation: cloning
alone does not prepare the hierarchy for drawing. Existing runtime byte-pointer
results remain compatible with kind-specific object views. Both source-unit
changes preserve all 483 objects and strict relocations.

### Canonical complete-hierarchy creation

`object_hierarchy_create.h` supplies a typed `ObjectTemplate *` creation contract
to the provider and all fifteen calling source units. Existing model-specific
and byte-storage template views convert explicitly at this boundary. The marker's
former integer-return declaration now uses the canonical pointer result and an
explicit address conversion at its existing integer API. Ghidra at 0x80008450
confirms cloning followed by transform-buffer allocation and returning the root.
The comments distinguish borrowed template storage from model display-list
lifetime and owned runtime hierarchy storage. Each source unit preserves all
483 objects and strict relocation addends. The subsequent single-object creation recovery below resolves ambient audio's
extra-argument call and consolidates the remaining creation declarations.

### Canonical single-object template creation

`object_create.h` supplies the typed `ObjectTemplate *` contract to the provider
and all six callers. Creation copies one complete template and initializes its
kind; complete hierarchy cloning and transform-buffer setup remain separate.
Ghidra 0x80008160 confirms only the template parameter is read. Removing ambient
audio's extra kind/position arguments preserves every object; that call now
matches the provider's signature without a compatibility exception. Legacy
byte-storage template symbols convert explicitly at the shared boundary.
The provider and each caller were rebuilt and strictly compared separately;
all 483 objects retain their bytes, layout and relocation addends.

### Shared deferred and immediate hierarchy destruction

The lifecycle header now declares deferred hierarchy marking and immediate
recursive destruction, with distinct lifetime and registration preconditions.
All deferred callers share the declaration instead of maintaining local variants.
Ghidra 0x800087BC confirms deferred destruction marks the hierarchy with 0x62;
the hierarchy processor performs later cleanup rather than freeing at the call.
Maze node cleanup now obtains each object through its pointer reference view and
a named local before checking and requesting destruction. Direct pointer-field
substitution changed register reuse; the local retains the original generated
code without keeping an unprototyped interface. Every source-unit change strictly
matches all 483 objects. Remaining registration/link ownership rules are explicit.

### Shared hierarchy flag interfaces and presentation exception

`object_flags.h` defines the canonical pointer/u16 enable and disable contracts.
The provider now uses shared runtime-object flags and sibling/child fields.
Ghidra 0x80008DE8 confirms halfword flags and recursive hierarchy traversal.
Menu object addresses convert explicitly at this interface, and maze activation
no longer supplies an unused third clear-flags argument. Accepted provider and
caller changes each preserve all 483 objects.
`collection_presentation.c` retains its original declarations: exposing the void
set-flags prototype changes later registers in presentationPrepareActiveActor
(0x800286B0), even with an explicit pointer cast. Its original implicit-return
call remains a matching constraint. Next evidence needed is IDO allocation data
or a source representation that preserves the void interface and those registers;
the shared API was not weakened to accommodate that one caller.

### Cave state producers and shared query contracts

Cave creation, landmark association and description selection now use the shared
checked `CaveState`, while object position and owned state use the runtime prefix.
Ghidra 0x8001F4F0 confirms the 0x20 allocation, two copied anchors, initially null
landmark and byte description selector. The first-anchor query uses the same
record instead of an undifferentiated float pointer. `cave.h` consolidates scene,
companion and VM declarations, including the pointer-based landmark association.
Anchors are copied; the landmark is borrowed and must remain live when queried.
The zero-result and invalid-axis sentinel contracts are explicit, including the
nonnegative axis precondition. All five changed units match all 483 objects.

### Unified rectangular scene-region state

The region provider now uses the existing checked `SceneRegionState` shared with
script queries, removing its duplicate private layout. Scale, positions and state
ownership use the runtime prefix, while scene scale uses the shared configuration
prefix. `scene_region.h` consolidates creation and highlight query contracts.
The ochre scene passes persistent association-data addresses; the canonical
interface preserves their original 32-bit representation with explicit caller
casts. Their contents remain uninterpreted. Ghidra 0x8001F1F0 confirms the 0x10
allocation, half-extents, highlight initialization and association word.
Each of the four source-unit changes preserves all 483 objects and relocations.

### Path query interface and partial-state consumers

`path_query.h` consolidates first-point and following-waypoint coordinate queries
with their VM caller. The following-point provider uses the existing private
partial state pointer and the shared runtime world-position fields. Ghidra at
0x80118210 confirms the points pointer at 0x0C, twelve-byte point stride and
negative-X end marker. The first three state words and complete allocation are
still unresolved; allocation/initialization evidence is needed before extending
the view. Byte offsets within the packed point sequence remain explicit to retain
matching expression ordering. Adjacent coordinate differences are unchanged.
Both changed units strictly match all 483 objects, including relocation addends.

### Scene path-table query contracts

The path-query header also covers current-scene point counts and coordinates,
separately documenting indexed counted paths and sentinel-terminated path objects.
Ghidra 0x80117BA4 confirms the configuration's path table at 0x70, eight-byte
count/pointer entries and twelve-byte point stride. The shared declarations
preserve signed index checks: path indices must be valid, and point/axis indices
must be nonnegative; out-of-range upper indices produce -65536. Provider and VM
caller rebuilds preserve all 483 objects and strict relocations.

### Shared nearest-target spatial interfaces

`object_target_search.h` consolidates object and actor-range searches with a
checked shared `Vec3f` and three-float Euler-angle input. The provider, actor
wrapper, script queries and voice text panel no longer maintain incompatible
vector declarations or byte-pointer angle contracts. Ghidra 0x80009ED4 confirms
the three-component origin copy, optional view setup, kind bounds and shared
result state. The interface explicitly prohibits overlapping searches and returns
a borrowed object. Script kind aliases remain handled by the actor wrapper.
All four changed source units preserve all 483 objects and strict relocations.

### VM-facing nearest-handle contracts

Actor-near-position, object-near-position and object-facing-cone wrappers now
accept object and coordinate pointers consistently with their VM callers.
The shared search header replaces the caller's independent prototypes. The
providers use shared rotation/world-position fields, retaining numeric object
handles only for their results. Ghidra 0x80117C18 confirms the position/facing
forwarding and handle extraction. The live excluded-object precondition avoids
promising unsupported null-address arithmetic. Both changed units preserve all
483 objects and strict relocation identities.

### Text cursor and scene script argument contracts

Collection presentation passes its display-list cursor directly to the shared
text drawing interface, eliminating three integer-to-pointer argument mismatches.
Ghidra 0x8003CE24 confirms writable command storage and a retained command cursor.
Playground and entryway creation pass their declared byte-array script addresses
directly to the canonical actor creation interface. Each source-unit change
preserves all 483 objects, including strict relocation addends.

### Landmark creation and description contracts

`landmark.h` gives the provider and seven callers one creation return type and
shared description lifetime contract. Scene callers previously disagreed between
void and object-pointer returns. Creation now uses the checked runtime-object
prefix for scale, local/world positions and kind. Ghidra 0x8001F110 confirms these
offsets, hierarchy creation and active registration; its inferred floating-point
argument list is not authoritative. Each changed unit preserves all 483 objects
and strict relocation addends. Description storage remains borrowed and shared.

### Interaction selection interfaces and marker record reuse

`interaction.h` consolidates candidate classification, marker creation and query
contracts across the provider and eight callers, including previously unprototyped
VM queries. Classification documents its shared scan state and region/visibility
bits. Ghidra 0x8001E830 confirms scale eligibility, world-position/radius reads,
handle-based held-item preference and candidate replacement. The provider uses
checked runtime-object fields and the shared vector type. Its six retained
animation records reuse the checked twelve-byte `ValueTrack`; no new allocation
layout is inferred. All nine source units preserve all 483 objects and strict
relocation addends.

### Interaction marker state consumers

Marker update and drawing now use the checked private state fields for mode,
retained target handle, animation tracks and evaluated values. Track order is
texture frame, opacity, depth offset, height offset, Z scale and Y scale; values
are consumed only after track evaluation. A track-array offset check supplements
the 0x50 allocation-size check, confirmed by Ghidra 0x8001EAAC. Runtime placement
uses shared local/world positions, scales, kind, radius and private-state pointer.
Remaining item configuration and actor-selected-handle views still require their
own producer/consumer audit. The marker unit preserves all 483 objects and strict
relocation addends.

### Marker item and held-item views

The marker now reads the shared `ItemKind` table and `ItemStatePrefix.kind`,
removing its duplicate 0x74-byte item configuration view. The original 0x34/0x38
extent accesses remain `radiusX`/`radiusZ`; no new dimensional meaning is assigned
to the shared fields. Ghidra 0x8001EAF4 confirms table stride and access offsets.
Drawing reads the actor's checked `heldItemHandle` at 0x190. Using the runtime
object's typed actor-state member changes generated code here; retaining the
original byte-pointer state load with a typed actor-state consumer matches.
The private marker style has documented fields and a checked 0x24-byte stride.

### Camera visibility region and position contracts

`camera_visibility.h` consolidates the by-value 0x14-byte viewing region and
three-float position interfaces. The region documents inclusive forward-depth
limits, coordinate scales and lateral allowance, with size and margin-offset
checks. Ghidra 0x800024C8 confirms negative-Z forward depth and strict lateral
limits; its scalarized aggregate signature is not used as a C prototype.
The main provider uses float-array indexing; interaction and HUD callers share
canonical declarations with the area visibility provider. An unused duplicate
region in the controller unit is removed. All five units preserve all 483 objects
and strict relocation addends.

### Pending movement and camera-facing interfaces

`object_movement.h` consolidates pending local movement get/set/add and
camera-facing orientation across the provider and nine callers. Movement uses
three float components consistently instead of byte, vector and integer argument
contracts. Ghidra 0x800080A8 confirms independent component accumulation. The
header documents shared traversal-state lifetime and active-camera requirements.
The integer motion-state boundary retains an explicit float-pointer conversion;
the marker passes its existing component-array view. Every source-unit change
preserves all 483 objects and strict relocation addends.

### World-segment and camera orientation contracts

The shared movement header now covers full orientation and pitch-only adjustment
along world segments and toward/along the active camera. Segment arguments are
three-float positions, returns are segment lengths, and coincident endpoints leave
orientation unchanged. Ghidra 0x80009948 confirms component differences, length
handling and rotation writes. The provider uses shared rotation/world-position
fields; byte-backed camera storage retains explicit float-pointer boundaries.
Three external caller units share these declarations. All four changed units
preserve all 483 objects and strict relocation addends.

### Sibling traversal and actor attachment links

The lifecycle header declares final-sibling lookup with live, acyclic-chain and
borrowed-result requirements. Ghidra 0x800080E4 confirms traversal through offset
0x28. The provider uses the shared sibling field; actor creation uses checked
root and sibling links for its ground-reference and shadow attachments. Both
source units preserve all 483 objects and strict relocation addends.

### Actor creation shared-state producers

Actor creation now initializes shared actor-state fields for attachment mode,
creation argument, ground reference, collision dimensions, ground offset, fallback
handle and VM address. Runtime transforms and radius use the checked object
prefix. Ghidra 0x8001E1C4 confirms the corresponding producer offsets and order;
its inferred integer casts for float stores are not adopted. Offset 0x208 remains
explicitly unresolved as `unknown208`. An attachment-mode offset check is added.
The VM pointer remains explicitly represented as its original 32-bit address.
Provider and header-dependent rebuilds preserve all 483 objects and relocations.

### Actor replacement ownership consumers

Actor replacement uses shared world position, pitch/yaw, scale, creation argument
and VM-address fields. Ghidra 0x8001E568 confirms the retained creation argument,
VM transfer, clearing of the original address and deferred hierarchy destruction.
The creation header explicitly documents transfer of any existing script context
to the runtime-registered replacement. The action caller continues using that
same context after replacement. Provider and dependent rebuilds preserve all
483 objects and strict relocation addends.

### Hierarchy transform-buffer allocation ownership

`object_matrix_buffers.h` consolidates pointer-based allocation across the
provider and five callers. Ghidra 0x80008338 confirms one allocation of 128 bytes
per hierarchy object, with the second buffer starting 64 bytes per object into
that allocation. Root slot zero owns the base; the second buffer and descendant
slots borrow interior storage. The shared object field comment reflects this
ownership. The provider uses checked buffer fields and callers no longer carry
integer object-address declarations. All six units preserve all 483 objects
and strict relocation addends.

### Transform slots and cloned hierarchy links

Matrix-slot assignment now uses checked root/buffer fields and pointer arithmetic
for each 64-byte matrix slot, plus typed child/sibling links. Ghidra 0x800082A0
confirms the stride, root-buffer source and depth-first slot assignment. Its
contract retains nonnegative, allocation-bounded indices and representable signed
offsets. Template cloning uses shared flags and root/child/sibling fields without
changing ownership or traversal order. Both incremental changes preserve all
483 objects and strict relocation addends.

### Hierarchy bounds and count traversal

`object_bounds.h` consolidates inherited-scale bound adjustment with its three
callers, documenting live acyclic links and in-place compounding of stored bounds.
Ghidra 0x80008520 confirms local X-scale inheritance and recursive radius updates;
its inferred integer scale parameter is not adopted. Bounds and hierarchy counting
use checked child/sibling links, with shared scale/radius fields for bounds.
All four source units preserve all 483 objects and strict relocation addends.

### World-position refresh contracts and traversal fields

`object_transform.h` consolidates the world-position refresh pointer contract
with actor creation and terrain debug callers. Ghidra 0x80008FAC confirms the
returned input address and preservation of surrounding transform/movement state.
The header requires an acyclic hierarchy, sufficient transform-stack capacity and
nonoverlapping traversal. Its traversal provider uses checked object flags, kind,
local transforms, world positions and child/sibling links; matrix translation
storage retains its existing representation. All source-unit changes preserve
all 483 objects and strict relocation addends.

### Search-tree insertion and object ordering contracts

The shared tree header now declares insertion with a checked `SearchTree` pointer,
caller-owned detached node and retained key. The provider uses the typed root;
object and item registries share the declaration. Object ordering uses shared
kind/handle fields and explicitly retains unsigned handle comparisons, confirmed
by Ghidra 0x800085A4. Each source-unit change preserves all 483 objects and strict
relocation addends.

### Node-returning tree lookup contracts and caller exception

`search_tree_lookup.h` declares typed tree and borrowed-node results for exact,
successor and minimum lookup. The provider uses shared root/comparator fields.
Ghidra 0x80041184 confirms exact lookup returns the matching root or zero.
Both object and item registry callers discard the result: adopting the canonical
pointer-return declaration changes their generated objects. Their legacy void
boundary remains explicitly documented; IDO allocation evidence or a minimal
matching caller representation is needed before removing it. Provider changes
preserve all 483 objects and strict relocations with these caller exceptions.

### Search-tree removal and caller ownership

The shared tree interface now declares removal with a `SearchTree` pointer,
comparator key and caller-owned `TreeNode` result. Ghidra 0x8004131C confirms
root replacement, retained node storage and unchanged removed-node links. The
provider uses typed root and link fields; object and item registry callers use
explicit target-width object-address keys. All three units preserve all 483
objects and strict relocation addends.

### Ordered tree traversal contracts

`search_tree_traversal.h` consolidates all three traversal orders, shared visitor
signature and borrowed node lifetime. Traversal links use `TreeNode` fields while
the callback retains the established byte-address view. Contracts prohibit nested
or overlapping traversals and mutation of traversal links. Ghidra 0x80041454
confirms the shared callback lifetime. The object caller's unprototyped integer
return declaration is corrected to void and its diagnostic callback accepts a
node pointer. Item traversal passes its checked registry link directly. All
three units preserve all 483 objects and strict relocation addends.

### Search-tree drawing and nearest-candidate layout reuse

The object draw callback now reuses `RuntimeObjectPrefix` instead of a duplicate
private draw layout, including the root reference and matrix-buffer views.
Ghidra 0x80009548 confirms node payload, drawing callbacks, root kind lookup and
selected buffer offsets. Object nearest-target callbacks use `TreeNode.value`
and shared object kind/world position; item candidates use the same world-position
field. Both source-unit changes preserve all 483 objects and strict relocation
addends, including the existing fixed-matrix command address representation.

### Runtime object manager allocation consumers

The private `ManagedObject` remains a proven 0x68-byte manager allocation rather
than extending every prefix user to assume a complete object. Ghidra 0x80008160
confirms the 0x58 entry pointer and embedded node at 0x5C; the latter now has an
offset check. Creation initializes the checked entry, flags and handle fields.
Destruction and nearest-search consumers use shared management/object fields.
The embedded node is initialized during registration, not by the creation clear
that ends at 0x5C. All changes preserve all 483 objects and strict relocations.

### Hierarchy management pointers and flags

Root assignment now uses an object pointer instead of an integer address, while
root designation and active registration use shared object links and flags.
The proven root-ownership and active-registration bits have enum names in the
shared flag header; unrelated flag meanings remain numeric. Ghidra 0x800084E8
confirms root association and ownership designation; its inferred void return is
not adopted over the source's established pointer result. Clone and destruction
use the same ownership flag. Provider and header-dependent rebuilds preserve all
483 objects and strict relocation addends.

### Approximate object-distance interfaces

`object_distance.h` consolidates object-to-point and object-to-object distance
queries across the provider, item search, voice spinner and VM distance caller.
The provider uses checked world positions and shared `Vec3f`; existing scalar-array
and camera-storage consumers explicitly select the vector view. Ghidra 0x80009CA4
confirms component differences and the approximate-length call; its inferred void
return is contradicted by the source and result-consuming callers. All four units
preserve all 483 objects and strict relocation addends.

### Actor axis-rotation interfaces

`actor_rotation.h` consolidates set/add contracts across the provider and seven
callers, including the movement caller's former integer actor argument. The
contract specifies axis bounds, finite angles and normalized results. Ghidra
0x80015F94 confirms the actor-state orientation base at 0x0C and normalization.
The provider uses the shared object state pointer. Replacing its shifted scalar
views with direct `ActorScriptState.orientation` accesses changes generated code;
those views remain a matching exception pending IDO allocation evidence. Each
accepted source-unit change preserves all 483 objects and strict relocations.

### Shared action-state query contracts

`action_query.h` consolidates current-action attributes and movement-scale
queries across the provider and five callers. It requires initialized live actor
state and documents inactive-playback defaults (0x20100400 and one). Ghidra
0x8000F594 confirms the playback-mode test and retained action-attribute field.
The permission masks at consumers retain numeric forms until each meaning has
producer/consumer evidence. All six source units preserve all 483 objects and
strict relocation addends.

### Action-controlled orientation fields and permission

Action-controlled pitch/yaw updates use shared actor-state orientation fields,
while both conditional rotation helpers use the named proven turning-permission
bit. Ghidra 0x800160FC confirms the attribute gate and pitch/yaw offsets. The
indexed single-axis helper retains its matching scalar view. Rotation contracts
now require finite results and magnitudes for which full-turn adjustments change
the floating-point value, avoiding a false termination promise for huge angles.
Provider and dependent rebuilds preserve all 483 objects and strict relocations.

### Actor-owned direction animation layout

The checked actor-state view now embeds the existing shared `DirectionAnimation`
at 0x60 instead of leaving its 0x7C-byte extent in unknown storage. Ghidra
0x80014FE4 confirms initialization at this offset within the 0x408 allocation;
update and action-command consumers select the same field. An offset check
supplements the existing enclosing-size checks. The following 0x18 bytes retain
explicit unknown storage pending further ownership/field evidence. Both changed
source units and all header dependents preserve all 483 objects and relocations.

### Actor lighting draw-state layout reuse

The private lighting model view is removed in favor of shared actor-state draw
lists, direction poses and selected pose index. Packed direction poses retain an
explicit SDK `Lights2` view for drawing commands. The shared state now exposes
model environment RGBA at 0xE0 with an offset check, used by drawing, immediate
color updates and transition output. Ghidra 0x80014EA0 confirms draw-list slots,
pose stride/index and color components. Transition inputs after 0xE4 remain
unresolved storage pending their own record audit. All objects and relocations
match after rebuilding header dependents.

### Shared RGBA transition contract and actor target state

`color_transition.h` declares the transition helper with byte-component pointers
and a writable floating-point countdown, replacing conflicting integer and byte
pointer declarations. Ghidra 0x800010C0 confirms the countdown access; the blend
consumer establishes the four RGBA components. Actor setter 0x80015588 establishes
target components at 0xE4 and duration at 0xE8. These checked shared actor fields
now serve both request and update consumers. Storage at 0xEC remains unresolved;
no broader transition record or ownership is inferred. Provider and consumer
changes preserve all 483 objects, including strict relocation addends.

### Actor model environment-color interfaces

`actor_color.h` provides one documented contract for immediate and timed model
color changes across the provider and five caller units. The credits consumer's
alpha parameter now uses the provider's unsigned byte type rather than a signed
word. Ghidra 0x800155B8 confirms that immediate updates touch only current RGBA;
they leave the target and countdown pending. Each caller was rebuilt and compared
before moving to the next source unit; all 483 objects and relocations match.

### Actor tracked-sound state and playback contract

The model provider now uses shared `soundHandles`, `nextSoundSlot` and
`trackedVoice` fields in initialization, playback and spatial updates. Offset
checks cover the replacement cursor at 0x188 and voice handle at 0x18C as well as
the four-handle table at 0x178. Ghidra 0x80014B60 confirms their access widths and
selection behavior. The table tracks playback; it does not own or automatically
stop every forgotten handle. `actor_sound.h` supplies the actor-pointer contract
to the provider and all four caller units, replacing an integer actor declaration
and integer argument casts. Each source unit preserves all 483 objects and strict
relocation addends.

### Shared positional-audio evaluation contract

`audio_spatial.h` unifies the floating-point world-position interface and retains
the SDK matrix's by-value calling convention across the provider and all three
consumers. Ghidra 0x8000C944 confirms matrix copying and output writes, while source
and matching objects establish the aggregate ABI omitted by its inferred signature.
The contract records nonzero camera-space distance and ordered attenuation bounds.
Object playback and actor tracked-sound spatialization now use shared world-position
fields; the actor helper accepts float coordinates directly. Camera record views
remain private pending a full producer/consumer layout audit. Every changed unit
preserves all 483 objects and strict relocation addends.

### Shared camera rendering and audio layout

`camera_types.h` moves the rendering producer's established 0xE0-byte camera
layout into a documented shared header. Actor and ambient audio now reuse it
instead of duplicate padded matrix-only views. Compile-time checks establish the
matrix pointer at 0x24, affine view at 0x28 and viewport at 0xA8. Ghidra 0x80001844
confirms the record stride, matrix retention and view/viewport production. Matrix
storage is borrowed from frame buffers; unresolved camera bytes remain explicit.
The object's separate `gRenderCameraMatrixEntries` interior-symbol view is retained
to preserve its relocation identity and addend until that boundary is audited.
All three changed source units preserve all 483 objects and strict relocations.

### Shared camera pose configuration

The player's camera-transition snapshot now uses the shared camera record rather
than a duplicate eye/target prefix. `camera_pose.h` supplies the float-coordinate
setter contract to the provider and both overlay callers. The setter's destination
stores use shared eye, target and up fields. Ghidra 0x80000AD4 confirms the 0xE0
stride, copied coordinates and world-up values; 0x80112698 confirms the primary
camera snapshot. Inputs are borrowed and copied during the setter call. Each
changed source unit preserves all 483 objects and strict relocation addends.

### Camera position-and-angle contract

`camera_pose.h` now also declares the position-and-angle setter for both callers.
The presentation unit's X and Z symbol views are floating-point coordinates, not
integer values; their corrected declarations preserve the original symbol and
relocation boundaries. The provider uses named shared eye, target and up fields
while retaining its grouped position stores for matching scheduling. Ghidra
0x800009C4 confirms coordinate stores and angle consumers. All three source units
preserve all 483 objects and strict relocation addends.

### Camera overlay layout recovery

The shared camera record now names the primary blended output, base transition,
additional blend transition and independently drawn secondary transition, including
RGBA targets and remaining-time fields. The existing 0xE0 size and added offsets
check the drawing colors at 0xB8, 0xC0, 0xCC and 0xD8. Set/reset, update and drawing
consumers use these fields without changing their expression ordering. Ghidra
0x8000114C confirms the three transition calls and final color blend; source
producers establish their distinct roles. Existing primary-camera interior aliases
retain their symbol boundaries. All 483 objects and strict relocations match.

### Shared camera overlay controls

`camera_overlay.h` consolidates the four overlay control interfaces across the
provider and nine caller units, including duplicate declarations in the player
and world-map units. Comments document component ranges, camera selection and
transition timing without implementation narration. Ghidra 0x800015D4 confirms
that reset changes current output/base/blend colors while preserving pending
targets and countdowns; the contract makes that persistence explicit. Every
source unit was rebuilt and compared separately; all 483 objects and strict
relocation addends match.

### Camera viewport and backdrop consumers

Camera initialization, scissor setup and depth clearing now share the checked
viewport field, documented in left/top/right/bottom order. The backdrop-selection
word at 0xA4 is exposed with an offset check and a contract derived from its
`renderDrawBackground` consumer; the preceding 0x18 bytes retain unresolved names.
Ghidra 0x80002058 confirms the viewport reads and backdrop dispatch. This change
preserves initialization and graphics-command expression ordering and all 483
objects, including strict relocations.

### Camera-space point conversion contract

`camera_pose.h` declares the in-place camera-to-world point conversion with its
actual single coordinate-pointer argument. The held-item caller no longer passes
unused state and object arguments and uses the shared local-position field.
The provider reads the shared camera affine view and eye fields. Ghidra 0x8000258C
confirms the single input, in-place transform and active-camera data. Both changed
source units preserve all 483 objects and strict relocation addends; no legacy
extra-argument exception is needed for this interface.

### Camera-relative object orientation layout reuse

Four camera-relative orientation helpers now obtain eye/target coordinates from
the shared camera record. Horizontal billboarding also uses its eye field,
removing the duplicate padded `CamRec` layout. Ghidra 0x80009AB8 confirms the active
camera target selection; the established segment helper and matching objects
confirm endpoint order. Camera interior matrix aliases and the billboard yaw
helper's existing extra-argument exception remain unchanged pending their own
interface audits. All 483 objects and strict relocation addends match.

### Billboard yaw contract recovery

The horizontal billboard helper now uses the provider's single-vector yaw
contract from `vector_direction.h`, eliminating its unused object argument and
local conflicting declaration. Its object accesses use shared world-position,
rotation and flag fields. Ghidra 0x8000CB1C confirms angle normalization; its
inferred zero-argument signature omits the forwarded vector that the matching C
provider establishes. This supersedes the extra-argument exception recorded in
the preceding camera-layout audit. The actual one-argument interface and typed
fields preserve all 483 objects and strict relocation addends.

### Runtime object billboard interfaces

`object_billboard.h` gives horizontal and full camera-facing drawing callbacks
shared contracts, including active-camera initialization and mutable orientation
flags. The special-item caller now includes this interface rather than declaring
the horizontal callback locally. Ghidra 0x800088BC confirms the orientation helper
and conditional flag update. No additional flag semantics are inferred from the
numeric mask. Provider and caller preserve all 483 objects and strict relocation
addends.

### Collision registration key and shared lifetime contract

The actor slot at 0x210 is a signed registration key, not a storage pointer.
The shared actor field now reflects that width and role, and model update/release
use its address explicitly. `collision_registration.h` supplies object-pointer
and writable-key contracts to the provider, actor model, actor parts and item
registry. Ghidra 0x8002CCB4 confirms key testing/clearing; the registration provider
returns the keyed-pool spatial hash. Model-part release no longer passes an unused
third argument. The contract requires removing a retained registration before
replacement or object destruction. Every changed unit preserves all 483 objects
and strict relocation addends.

### Collision overlap query contract and matching boundary

The shared collision interface now documents overlap results as borrowed objects,
early termination on response exhaustion and shared keyed-pool iteration state.
Both query callers use it. Registration uses shared radius, flags and world-position
fields. Ghidra 0x8002CCF8 confirms overlap accesses and candidate traversal, and the
keyed-pool provider establishes its mutable cursor. The overlap function itself
retains raw object accesses: converting all fields failed the ROM gate and strict
object comparison; a reduced conversion of radius/kind/flags also failed. Restoring
those accesses restores all 483 objects. Next evidence needed is compiler-decision
analysis of the changed function before choosing a minimal matching typed view.
No comparison rule or baseline was changed.

### Collision registry lifecycle and keyed-pool interface

The collision provider now includes the existing keyed-pool contract for allocation,
iteration, insertion and removal. Its unused fourth removal argument and duplicate
impulse declaration are removed; the actual three-argument pool interface matches.
The shared collision header now declares initialization, destruction and alignment
refresh for the object-system caller. Alignment changes do not rewrite retained
keys; object updates unregister with stored keys before registering new positions.
Ghidra 0x8002C8B0 was also checked against the private impulse provider; no new shared
impulse API is introduced because it has no external source callers. All changed
units preserve all 483 objects and strict relocation addends.

### Animated model-part allocation layout

A private `ActorPartTextureState` now describes the complete 0x558-byte allocation:
three playback rates, three borrowed-program integer tracks, two 0x280-byte command
buffers, RGB tint and collision registration key. Ghidra 0x80013848 establishes the
allocation, channel setup, buffer stride and tint initialization. Compile-time
checks cover the extent and offsets 0x14, 0x50, 0x550 and 0x554. Initialization,
tint setters/drawing, playback-rate updates and collision lifecycle use these
fields; unresolved gaps remain explicit. Command-writing byte cursors retain
their buffer-offset form. The type stays local because no other unit owns or
consumes this record. All 483 objects and strict relocation addends match.

### Shared actor part tint and texture controls

`actor_part_texture.h` documents the three control interfaces, channel bounds,
borrowed animation-program lifetime and actor-pointer parameters. The action
command's speed uses a floating-point load and provider-compatible prototype,
replacing the previous integer bit-transport declaration without changing bytes.
Motion playback no longer supplies the unused second tint-reset argument. Ghidra
0x80014960 confirms channel stride and program selection; the matching provider
establishes the floating-point speed type. All three changed units preserve all
483 objects and strict relocation addends.

### Shared actor appearance controls and variant slots

`actor_appearance.h` consolidates lookup, scale and model-variant contracts with
actor-pointer parameters and explicit slot bounds. The lookup retains its
established floating-point scale-component view of a borrowed actor-owned part.
Action commands and motion playback no longer pass unused arguments to variant,
scale and reset functions. Actor state exposes two checked variant words at
0x2F0, shared by set/query and drawing consumers. Ghidra 0x800147E4 confirms the
slot-indexed writes. All four changed source units and header dependents preserve
all 483 objects and strict relocation addends.

### Linked model-part rotation follower layout

A private 0x10-byte `ActorPartLinkedRotationState` links initializer and update
consumers through named part identifier, axis, borrowed resolved part, captured
reference angle and response factor. Ghidra 0x800139B8 confirms the cached pointer
and one-time reference capture; the initializer establishes the allocation and
configured factor. Size and pointer/factor offsets are checked. Both consumers
also use shared object kind, root and state fields. Axis-based rotation byte
accesses and the original configuration-symbol view remain explicit. The state
stays local because its producer and consumer are in the same unit. All 483
objects and strict relocation addends match.

### Shared primary and secondary gameplay poses

Actor state now exposes checked affine transforms and folded Euler-angle triples
for the primary and secondary signed part sequences. Ghidra 0x80014590 confirms
transform production at 0x39C/0x32C and angle output at 0x3CC/0x35C. Gameplay pose
production and model-part orientation/scale followers use these shared fields.
The adjacent relative-part controller records remain explicit unresolved storage;
no layout is inferred merely from adjacency. All header dependents and the changed
source unit preserve all 483 objects and strict relocation addends.

### Shared temporary model-part shake state

Actor state now names the borrowed shaking part, remaining seconds and captured
reference-angle triple, with checked offsets 0x3D8, 0x3DC and 0x3E0. Start/update
consumers share these fields. Ghidra 0x80014360 confirms countdown, conditional
restoration and additive shake behavior; the child behavior word remains unresolved.
`actor_part_shake.h` provides the actor-pointer request contract to the messenger.
A focused header avoids changing unrelated default-part lookup declarations in
that unit: adopting the broader part header required an unrelated prototype change
that altered its object bytes. Those declarations are restored for separate audit.
All 483 objects and strict relocation addends match.

### Costed-action burst allowance and result fields

The remaining word at 0x3EC is `targetedBurstAllowance`, supplied by the costed
script action and consumed by paired target bursts. The adjacent `costedActionResult`
is returned to the script after action completion: the producer writes zero on
insufficient resource and three after starting; response commands can record one.
Ghidra 0x80116BC4 confirms this producer/result contract. Both fields have checked
offsets and shared producers/consumers. Numeric result values remain explicit
rather than inventing an exhaustive enum for unobserved outcomes. Both source
units and header dependents preserve all 483 objects and strict relocations.

### Shared actor resource-meter adjustments

`actor_meters.h` declares both bounded actor-meter adjustments for the provider
and all five caller units. The light-bulb consumer now passes its actor handle
through the pointer contract rather than declaring an integer parameter. Both
providers use the checked `ActorScriptState` meter fields at 0x3F4 and 0x3F8.
Ghidra 0x800149D0 and 0x80014A3C confirm the owning-root state, bounds and HUD
request behavior. Each source-unit change preserves all 483 objects and strict
relocation addends.

### Primary actor script clock and costed-action VM fields

`actor_script_time.h` supplies the explicit no-argument primary actor clock
query to its provider and presentation callers. Ghidra 0x80014B3C confirms
the retained actor VM and total-elapsed field at 0x10. The provider now uses
shared actor/VM fields rather than raw offsets. The costed-action handler
also uses the shared VM address, first-update flag and action-resource field;
Ghidra 0x80116BC4 confirms those consumers. Every changed source unit preserves
all 483 objects and strict relocation addends.

### Shared actor script action-handler selectors

`actor_script_action.h` names all fifteen selectors observed in the action
dispatcher and shares the scheduling/dispatch contracts across their provider,
the VM and three script-command consumers. Ghidra 0x801167A0 confirms the
initialization request and retained selector; 0x80117098 confirms the dispatch
map. All scheduling calls and dispatch cases use the shared enum constants,
while the existing signed argument and unsigned VM storage retain their ABI.
Redundant declarations of the two private wait helpers are removed. Every
source-unit change preserves all 483 objects and strict relocation addends.

### Shared action-script completion queries

`action_script_query.h` shares the command-stopped, last-action-ended and
combined completion queries across their provider and all declaration consumers.
The combined query now takes the actual actor pointer, and the acquisition
handler no longer supplies an unused VM argument. The provider reads the checked
command cursor and last-request fields. Ghidra 0x80012114 and 0x8001216C confirm
the recorded-request comparison and combined query contract. All six source
units preserve all 483 objects and strict relocation addends. Stop declarations
remain a separate audit: actorScriptStopActionScript already documents its
argument-register dependency, which prevents blindly adopting a stop prototype.

### Shared action-script startup and duration contracts

`action_script_playback.h` shares startup, inherited-mode, duration-update
and duration-limited-start declarations across all providers and declaration
consumers. Startup and setters use checked actor-state fields. Ghidra
0x80012034 confirms the initialized command cursor, request tracking and clocks;
0x80117AA0 confirms the duration wrapper forwarding. The wrapper now accepts
the actual actor pointer and float duration, agreeing with its script caller.
This removes its mismatched integer declaration and portability exception.
A union reinterpretation changed matching bytes; the direct float parameter
preserves all 483 objects, including strict relocation addends.

### Action-script stop contract and remaining presentation exception

`action_script_stop.h` shares the actor-pointer stop interface with its provider
and four caller units. The returned borrowed pointer identifies the actor
script-ID field at 0x158, rather than a separately owned script record; Ghidra
0x800120AC confirms this. Its command-cursor and ID stores and returned field
address use checked actor fields. Passing the actor explicitly in the script
stop command preserves all objects and removes its argument-register exception.
The collection-presentation declaration remains local: adopting the canonical
prototype and removing its unused presentation-state argument passed the ROM
gate but changed allocated object bytes. That trial is restored. Further
compiler scheduling evidence is required; the declaration records the exception.
All accepted changes preserve all 483 objects and strict relocation addends.

### Timed action command format and update interface

`ActionScriptOpcode` records the four handler selectors and the terminal
selector. The ROM-derived dispatch table at 0x8006EE50 establishes their order;
Ghidra 0x80011E6C confirms terminal-command handling and callback arguments.
`ActionScriptState` is explicitly an interior actor-state view at 0x158,
not an independent allocation. Additional clock offset checks cover elapsed,
duration and command time in both views. The program table now contains typed
command pointers and the model updater shares the documented sequencer update
interface. Header dependents and both changed units preserve all 483 objects
and strict relocation addends.

### Shared actor action-request contracts

`action_request.h` shares request submission, pending-request cancellation,
loop-exit requests and active/stored request-ID queries. Ghidra 0x8000F404
confirms assigned IDs, pending action/kind, the next-ID counter and idle
acceptance; 0x8000F6C0 confirms the retained selected ID at 0x128. These
providers now use checked actor fields, preserving the unsigned playback-mode
test and existing signed request representation. Movement callers pass actor
pointers explicitly. The provider and all three declaration consumers preserve
all 483 objects and strict relocation addends.

### Action continuation and transition-pose selectors

Actor state names the previous/current pose selectors at 0xFE/0xFF and signed
continuation action at 0x100, with compile-time offset checks. Ghidra
0x8000EAF8 confirms source/destination pose selection and continuation assignment.
The playback-stop provider uses checked start, loop-exit, request and pose fields;
0x8000F4D4 confirms its resets. `action_continuation.h` shares stopping and
continuation-chain contracts across the provider and all four caller units.
Two current-pose reads retain byte views: typed access in
`actionTryApplyCompatibleRecord` changes load order, and the transition-map key
in `actionResolveNextTransition` changes register allocation. Disassembly
isolated both differences; restoring those reads preserves all 483 objects and
strict relocation addends. Further compiler scheduling evidence is needed for
those two replacements. Other recovered field consumers remain typed.

### Shared actor action-cue records and playback state

`actor_action_cue.h` recovers the 0x14-byte cue/tag record and selection/update
contracts shared by motion playback and action-command execution. Ghidra
0x80024F9C confirms tag selection and 20-byte stride; 0x80024DD0 confirms timed
commands, jump handling and cursor updates. This is distinct from the 0x10-byte
action-sequencer command. The old Cmd14 and Ent14 views now share the record.
Actor state names the selected action kind at 0x13E and primary/secondary cue
clocks, cursors and retained primary program at 0x144 through 0x154. Checked
offsets and shared start/update/stop consumers establish borrowing and repeated
program selection. Every changed unit and header dependent preserves all 483
objects and strict relocation addends.

### Selected action cue resources and scale/duration word

Actor state names borrowed primary/secondary cue programs at 0x110/0x114
and the action scale-or-transition-duration word at 0x104, with checked offsets.
Selection, transition setup, fallback preparation and motion startup share
these fields. Ghidra 0x8000EC7C confirms pending-to-selected request/kind
transfer and primary-program fallback. The fallback resource and local program
pointer now use ActorActionCueCommand directly; the preparation helper accepts
that pointer rather than an integer address. The initializer word at 0x108
remains unresolved as an interface: its legacy no-argument callback convention
requires a separate caller/ABI audit. All header dependents and the source
unit preserve all 483 objects and strict relocation addends.

### Actor action initializer callback contract

The 330-entry ROM action table contains 41 non-null initializer slots targeting
25 distinct functions. Every target now has an `s32 (u8 *)` definition. Five
approach preset wrappers previously declared void now explicitly return their
helper result: their ROM assembly leaves that result in v0, and the dispatcher
consumes it. Integer actor parameters in the remaining initializer targets
are recovered as actor pointers. `ActorActionInitializer` documents zero as
acceptance and nonzero as immediate continuation selection; actor state checks
its offset at 0x108. Ghidra 0x8000E8D8 and the caller at 0x8000ECB4 establish
that contract. The indirect call now passes the actor explicitly, replacing
the legacy unspecified-argument call. All changed units and header dependents
preserve all 483 objects and strict relocation addends. This resolves the
initializer ABI audit left open in the preceding cue-resource recovery.

### Per-frame actor action callback contract

The ROM action table has 70 non-null per-frame callback slots targeting 38
distinct functions. All now have `s32 (u8 *)` definitions and shared documented
declarations in `actor_action_callbacks.h`. `ActorActionUpdateCallback` records
the observed contract from Ghidra 0x8000F14C: one ends motion immediately; zero
permits normal looping; other nonzero values inhibit looping when motion ends.
Seven preset wrappers explicitly return their movement helper results. Two
oscillating approach handlers return the forwarded zero on their movement path,
removing their non-void fallthrough exceptions. Four float-view callback bodies
retain direct casts: adding local view pointers changes stack slots or frame
size. Direct casts preserve all objects while providing a canonical actor
parameter. The callback field and playback consumer use the shared typedef.
All 483 objects and strict relocation addends remain identical.

### Shared ROM action-definition layout

`action_definition_types.h` describes the 0x2C-byte action resource copied
into actor-state bytes 0xEC through 0x117. It shares initializer, update and
cue-program types and checks size plus motion, continuation, scale, callback
and cue offsets. Unresolved bytes at resource 0x16 remain explicit. Actor state
now names the selected ID and borrowed name. Ghidra 0x8000EBA0 confirms the
44-byte copy and continuation overwrite; the ROM table and prior callback
audits establish callback/resource fields. Motion requests, synthetic pose
transitions, continuation traversal and table validation use recovered fields.
Word-oriented aggregate copies retain their existing representation to preserve
the compiler's copy behavior. Header dependents and the changed source unit
preserve all 483 objects and strict relocation addends.

### Typed action table, selection and aggregate copies

The action table is now an ActionDefinition array. Resource selection,
compatibility checks, transition resolution, name queries, continuation walks
and transition-map iteration use the recovered record directly. Ghidra
0x8000EA88 confirms compatibility testing and the complete resource copy.
Copies into the actor's selected-action region use ActionDefinition and the
checked actionId field address, eliminating both placeholder 0x2C records.
The previously retained word-copy representation is no longer necessary.
One transition-map comparison needs initial-pose before final-pose in the
source expression to retain the original branch operand encoding; the values
and condition are unchanged. The actor current-pose byte-view exceptions from
the earlier audit remain. All 483 objects and strict relocations match.

### Shared motion playback modes and frame interfaces

`motion_playback_types.h` names disabled, track, timed and pose-blend modes
without changing the signed 16-bit actor storage or unsigned mode tests.
Root/child dispatch and startup use the same values. Ghidra 0x8000E190 confirms
mode propagation to animated model parts; the startup/update bodies establish
the four policies. Reset and mode setters use checked actor fields, with new
loop-count, duration and remaining-time offset checks. `motion_playback.h`
shares reset, mode selection and root/child frame-update contracts with model
consumers. Both source units and all header dependents preserve all 483 objects
and strict relocation addends.

### Pose-transition and motion-query interface contracts

`action_continuation.h` shares the pose-transition request with both callers;
the presentation caller now supplies its actor address through the pointer
contract. Ghidra 0x8000F534 confirms stop/reset, transition setup and motion
request behavior. Selected-context preparation uses the checked pending/active
request and action-kind fields; current motion requests use checked attributes
and motion ID. `action_query.h` also documents the motion-ID sentinel and
borrowed-name query contracts. All changed source units and header dependents
preserve all 483 objects and strict relocation addends.

### Shared root-motion transform and heading

Actor state now describes the Matrix34 root-motion transform at 0x18 and
extracted heading at 0x48, with checked offsets. Root-track startup, orientation
application and pose blending share these fields. Ghidra 0x8000E00C confirms
orientation construction, heading extraction, yaw delta and retained heading.
The root-orientation interface uses float angles and an actor pointer; root
track evaluation uses actor/state pointers rather than integer addresses.
Both contracts are documented in motion_playback.h. Existing float transform
views preserve matrix-helper calling conventions. All header dependents and
the changed unit preserve all 483 objects and strict relocation addends.

### Pose-blend matrix storage

The transition builder now reuses `Matrix34` for its consecutive source and
destination matrices instead of a duplicate private twelve-float record. Root
transition storage at 0x23C remains a byte boundary because track playback also
uses that region; it is not an exclusively matrix-owned allocation.

`motionUpdateChildPlayback` now declares all twelve output floats rather than
relying on a three-float array followed by unused frame padding. Ghidra
0x8000F33C confirms the shared blend output and Euler-extraction input;
0x8000E81C confirms the second matrix at offset 0x30. Both changes preserve
allocated bytes, layout, symbols and relocations across all 483 objects.

### Shared three-channel animation tracks

`animation_tracks.h` supplies the curve descriptor, three-channel state and
incremental state used by the animation producer and motion playback. Descriptors
distinguish their curve kind from the signed constant or key reference; writable
state names per-channel times, key indices and previous values. Sizes 8, 0x30
and 0x3C and state offsets 0x18, 0x24 and 0x30 are compile-time checked.
Ghidra 0x8000A34C and 0x8000A3AC confirm relocation and initialization. Key data
is borrowed, not owned by these state records. The four shared interfaces retain
their existing channel-output and completion semantics.

The motion archive entry remains a distinct private record: equal two-word
layouts do not establish equal field meanings. Scalar curve helpers still carry
32-bit key-index addresses and raw descriptor views; those interfaces need
separate matching trials before changing their pointer contracts.

### Scalar animation pointer contracts

The scalar key advancement, cubic and linear evaluation, and curve dispatch
now pass writable key indices as `s32 *` throughout. Ghidra 0x8000A000 confirms
the fourth parameter is dereferenced and incremented. Curve dispatch uses
`AnimationCurveDescriptor *` and its named kind and value/reference members.
The key-slot alias view remains explicitly documented.

Direct array-member expressions for the channel descriptor, time and index
changed the generated ROM under IDO. Their byte-offset expressions are retained
against the checked shared layouts. Pointer contracts and descriptor member
accesses independently pass the strict object comparator.

### Scalar animation key records

The private `AnimationScalarKey` now documents the shared 16-byte keyed-curve
record, including cubic coefficients, linear slope/intercept, held value and
30 Hz duration. Size and duration-offset checks preserve the archive layout.
Ghidra 0x8000A000 establishes the common duration/hold fields and 16-byte stride;
0x8000A1F0 establishes the linear interpretation of the first two floats.

Advancement and both evaluators now use the same typed key-pointer cursor,
removing the incompatible float-pointer slot between those functions. The
archive descriptor's integer address word still requires a target-specific
pointer-slot view, documented at curve dispatch rather than hidden in helpers.

### Actor root playback workspace

Actor state now names the 0x78-byte root playback workspace at 0x23C and the
0x3C-byte movement delta track at 0x2B4. The former has distinct orientation-track
and consecutive pose-matrix views selected by playback mode, with its remaining
extent retained explicitly rather than assigned unsupported fields. Shared
size and actor-offset checks preserve both boundaries and the 0x408-byte actor.

Ghidra 0x80014FE4 confirms the two cleared extents; 0x8000E408 confirms the
orientation and delta-track evaluation addresses. Motion setup, root evaluation,
pose setup and pose blending now use the named views. Actor initialization clears
the records by address and checked size. Each source-unit conversion passes the
strict comparison of all 483 objects.

### Child-part playback state

`motion_playback_types.h` now defines `ChildMotionState`, with a 16-bit playback
mode and a mode-dependent rotation-track/pose-pair workspace at offset 4. Size
checks establish the 0x60-byte workspace and full 0x80-byte allocation. Remaining
bytes retain explicit offset names because optional part controllers use the tail.

Ghidra 0x80015110 confirms allocation, clearing and mode initialization;
0x8000E470 confirms the track boundary and mode reset on completion. Child track
setup, pose capture, frame update and allocation now use the shared layout.
Track keys are borrowed from motion data; the child object owns its state.
Both source-unit changes preserve all 483 strict object comparisons.

### Optional child-facing ownership

The child motion state now names its optional owned `facingController` pointer
at 0x7C, with a checked offset. Target-facing and camera-facing constructors,
updates and release share this member. The callback selects which private
controller layout applies; a single common record would misrepresent them.

The target controller is a checked 0x1C-byte record containing animated angles,
previous output angles and weight at 0x18. Camera-facing state is a checked
0x10-byte record containing animated angles and weight. Ghidra 0x80015160
confirms the target allocation and initial orientation histories. Release frees
the optional allocation before freeing child state. Existing allocator extra
arguments remain documented matching constraints.

### Facing-controller field consumers

Target- and camera-facing constructors now allocate their checked private
record sizes and initialize named angles and weights. The target-facing update
uses typed animated and previous-output angle histories throughout. Ghidra
0x80015704 confirms the two histories are distinct: active animation refreshes
the first, while every update records its resulting orientation in the second.
These field conversions preserve all allocated bytes and relocation identities.

### Facing consumers of shared camera and target state

Child-facing code now reuses `MatrixCamera.eye` instead of its duplicate private
camera prefix. Target resolution uses the checked actor target mode and stored
world position, and target-facing inhibition uses the shared behavior flags.
Ghidra 0x80015620 confirms the 0xE0-byte camera stride, stored-target selection,
and the fallback coordinates at actor offsets 0x1EC through 0x1F4. The lifecycle
functions have no external C callers in the current tree; no additional shared
header is required for those private interfaces. All converted accesses match
the immutable 483-object baseline.

### Motion archive part entries

Track playback and pose transitions now share a private, checked eight-byte
`MotionPartTrackEntry` with object kind and archive-relative curve offset.
Nonpositive kinds terminate the table, root kinds are below 0x26, and child
kinds select model parts. This record remains distinct from curve descriptors.
Ghidra 0x8000E208 confirms the entry stride and root movement descriptors
following the rotation descriptors at +0x18.

Playback timing, vertical/default pose offsets and movement-history clearing
now use checked actor fields throughout the affected motion unit. These changes
preserve the complete strict object baseline without changing expressions or
playback control flow.

### Motion playback state consumers

The motion unit now uses checked fields for action attributes, startup state,
loop count, pending action, callback reset and behavior inhibition. Root
track outputs name pose displacement and local motion. Child frame updates
read named parent timing fields, and child rotation normalization uses the
shared runtime object's rotation array.

Mode propagation uses the actor part count/table, runtime object kind and
child playback mode. Ghidra 0x8000E190 confirms this traversal and the child
kind interval 0x26 through 0x63. The part table retains its existing byte-pointer
representation at the shared boundary. All these consumers match the complete
strict object baseline; previously documented pose-read exceptions remain.

### Shared matrix orientation interfaces

`matrix_orientation.h` supplies the documented XYZ-degree construction and
orientation-blending contracts to both providers and every C caller. Ghidra
0x80039994 confirms all three rotation parameters are floating point. The
model-motion caller now reads its two angles as floats rather than passing
integer bit patterns through an incompatible local declaration; the corrected
prototype and caller preserve the original allocated object bytes.

The six affected source units were rebuilt and compared individually. The
shared contracts retain flat twelve-float matrix storage at the helper boundary
and document output and blending weight semantics without implementation detail.

### Shared linear-transform and Euler contracts

`matrix_orientation.h` also declares alias-safe linear-transform multiplication
and folded Euler extraction. Provider and callers now share float matrix/output
pointers instead of inconsistent byte and untyped pointers. Ghidra 0x8003B764
confirms the three writable angle destinations. Existing Doxygen contracts
describe composition order, permitted input/output aliasing and output angles.
The shared-header conversion rebuilt every dependent unit and preserves all
483 objects, including relocation identities.

### Local rotation pointer contracts

Local XYZ and ZYX rotation helpers now receive writable float matrix pointers
instead of integer RAM addresses. Ghidra 0x80039814 confirms the input matrix
is passed as both source and destination of composition. Hierarchy, model-part,
toolbox and interface-camera consumers use the shared contracts; the provider
conversion and dependent rebuilds preserve all 483 objects.

The same header now supplies world-Y affine rotation and motion Euler extraction
to their providers and motion consumer. It documents degree units, in-place
translation behavior and writable output destinations. Integer address boundaries
in unrelated camera interfaces remain for separate evidence and matching trials.

### Shared affine translation contract

`matrix_translation.h` declares local-space translation with the provider's
four-by-three float matrix and floating-point displacement parameters. Ghidra
0x8003973C confirms this contract. The interface-camera builder now reads its
position components as floats instead of passing integer bit patterns through
a conflicting local declaration. Hierarchy calls name the transform array,
and the view-builder call explicitly converts its retained address boundary.

Provider and all C declarations use the shared header. Five source units were
rebuilt and strictly compared one at a time; all 483 objects remain identical.

### Interface-camera transform contract

`camera_interface.h` now supplies one writable float-matrix output contract
for the interface-camera builder and its system-menu, toolbox, world-map and
collection consumers. The provider no longer carries the output pointer as
a signed integer. Its original 0x80000800 identifier is preserved in Doxygen.
Ghidra confirms the output flows through affine initialization, camera-position
translation and player-view rotation. Each affected source unit was rebuilt
and compared before moving to the next; all 483 objects match.

### Directional-search view interfaces

`camera_view.h` supplies the view builder's three-component position, Euler
angles and writable float-matrix contract. The provider no longer accepts its
output as an integer. Object and item searches use the shared declaration.
The object relative-yaw consumer no longer passes an unused fourth argument;
this correction preserves strict matching and removes that ABI exception.

`matrix_orientation.h` also supplies the ZYX rotation constructor to its provider
and all C callers. Ghidra 0x8000D6D8 confirms the view output is passed to that
constructor and local translation. All dependent units rebuild with the canonical
contracts and preserve all 483 objects.

### Shared affine point-transform interface

`matrix_point.h` replaces ten local declarations with one documented float
matrix, input position and output position contract. Ghidra 0x8003FB90 confirms
twelve matrix components, three point components and input/output aliasing.
Existing byte and private-record views convert explicitly at this interface;
their global definitions and linker identities are unchanged.

Rendering, motion, terrain, interface and search callers were rebuilt and
compared one source unit at a time. Every conversion preserves all 483 allocated
objects and strict relocation identities. The locally generated implementation
was not edited or added to the repository.

### Interface transform global layouts

World-map, toolbox and collection-presentation transform declarations now use
`Matrix34` instead of a single word or unsized byte array. Their camera-builder
producers and affine point-transform consumers establish the twelve-component
layout. Ghidra 0x8002DD70 confirms the world-map matrix is used for both focus
and actor placement. Original global identifiers and addresses remain documented.

The system-menu transform definition also reuses `Matrix34` instead of a private
twelve-word record. Its Doxygen description now precedes the declaration. All
four source-unit changes preserve BSS layout, symbol attributes and relocation
identities across the complete 483-object comparison.

### Shared presentation vectors and focus interface

World-map, collection and toolbox positions now reuse checked `Vec3f` rather
than three duplicate private coordinate records. This includes initialized
presentation positions, focus arrays, anchors and local placement vectors.
Their allocated layouts and global relocation identities are preserved.

`presentation_focus.h` documents immediate reset, timed target changes and
current-point evaluation using the same vector type for provider and consumer.
Ghidra 0x8002D348 confirms the output is three coordinates and retained as the
current focus. Each source-unit conversion passes all 483 strict object checks.

### Shared timed-vector interpolation contract

`vector_interpolation.h` supplies the float-vector interface used by presentation
focus and companion camera transitions. It documents positive duration, elapsed
time units, endpoint completion and separate endpoint/output storage. Ghidra
0x8000DBE4 confirms three component outputs, time clamping and the -1 completion
result. Provider and both caller units preserve all 483 strict object comparisons.

The companion comment claiming an integer duration argument was stale: its
existing declaration and call already used float duration. It is replaced with
the actual prepared-transition and valid-camera precondition.

### Companion camera-transition fields and preparation

The companion update now names the four retained camera vectors instead of
raw player offsets. Compile-time checks cover start/end eye and target vectors
at 0xA0, 0xAC, 0xB8 and 0xC4, plus remaining time and duration at 0xD0/0xD4.
Ghidra 0x801116D8 confirms the reversed endpoint order used with remaining time.

`player_camera.h` supplies the preparation contract to provider and script caller.
Destination vectors are copied from float pointers; the old byte-pointer view
and stale signed-integer duration restriction are removed. All affected source
units and shared-header dependents preserve the complete strict object baseline.

### World-map destination records

The private `WorldMapDestination` replaces the nineteen-word view with stage
area identifiers, the availability flag, presentation focus, icon rectangle and
stage badge textures. Size 0x4C and offsets 0x10/0x1C/0x40 are checked. Selection,
placement, focus, drawing and area-transition consumers use the named fields.
Ghidra 0x8002D474 confirms the icon copy, rectangle coordinates, badge offset and
record stride. Original table identifier and address remain documented.

Two identical private texture-rectangle layouts in the unit are consolidated,
removing casts between those records. No global data definition or linker layout
changes; both recovery steps preserve all 483 strict object comparisons.

### Saved progress-value interface

`progress_value.h` supplies the provider's unsigned field-selector and signed
value-return contract to fifteen caller units. The selector accepts a saved
field ID or an area-relative selector. Ghidra 0x80032C1C confirms resolution
followed by the saved-field read; the provider source establishes the types.
Two unused trailing arguments in world-map and companion calls are removed,
with their obsolete portability notes.

Toolbox retains its local signed-selector declaration: adopting the unsigned
shared prototype changes ROM bytes. Its compatibility contract and the ledger
record the exception and the need for IDO scheduling/conversion evidence.

### Shared progress-field limits and updates

`progress_fields.h` consolidates minimum/maximum queries, bounded set/add
operations and current-area selector availability across the provider and
eleven consumer units. Contracts identify valid selectors, signed adjustments
and the stored-value return after limits are applied. Original identifiers
and addresses remain attached to each declaration.

Provider reads, writes and selector resolution establish the interface types;
Ghidra 0x80032C44 confirms the resolver-to-store call chain. Each source-unit
conversion preserves all 483 objects with strict relocation comparison. The
separate read header keeps the documented toolbox signed-read exception local.

### Progress-record persistence contract

`progress_storage.h` unifies initialization, reset, gameplay restoration and
EEPROM request declarations across save storage, player profile, runtime, title
and system menu. Requests require initialized storage and an idle EEPROM
interface; setup acknowledgment is separate from transfer completion and errors.
Applying loaded state requires a valid record and a successfully completed read.

The payload-size table distinguishes one-byte control records 0/3 from 0x74-byte
progress records 1/2 and their backups 4/5. Progress read/write parameters now
document indices one and two rather than including the control record. Ghidra
0x80032DC4 confirms the fixed progress payload and backup lookup. Each unit
retains all 483 allocated objects and strict relocation identities.

### Progress-field representation records

`ProgressFieldTypeLimits` replaces the duplicate twelve-byte private range
record in save progress and toolbox. Its shared header documents bit width,
minimum and maximum with size and member-offset checks, preserving the original
table identifier and address. The eight-byte `ProgressFieldStorage` remains
private to save progress and names its byte offset and packed-bit shift.

Ghidra 0x8003267C confirms the twelve-byte limits stride and eight-byte location
stride, including shift at offset four. Layout construction and signed/unsigned
read/write cases establish the field roles without changing packed arithmetic.
Both units preserve every allocated object and strict relocation identity.

### Story-chapter and milestone interfaces

`story_chapter.h` declares the chapter query and visit-driven chapter update;
`story_milestones.h` supplies the chapter setter and unlock contract. Sixteen
units include the appropriate game-owned headers. The locally supplied SDK
system header also contains a historical declaration of the chapter query.
That checksummed external input remains unchanged; game callers now include
the explicit game-owned contract rather than relying on that declaration.

Ghidra 0x8002AD04 confirms the signed chapter input and milestone writes. The
system-menu legacy callback still omits that input; its existing portability
note and recovery ledger preserve the unresolved ABI requirement without
inventing a chapter value. Every converted unit preserves the strict baseline.

### Shared weighted-choice records and scene scattering

`weighted_choice.h` replaces the provider and maze consumer's duplicate records.
Entries carry keys and mutable weights/cumulative thresholds; the table stores
the last selectable index rather than a count. Eight-byte sizes and offset-four
fields are checked. Preparation requires at least two choices, a writable
terminator, nonnegative weights and a positive total fitting the signed range.
Ghidra 0x80032490 confirms mutation through the terminator and the cached index.

`maze_scatter.h` carries this table type through the maze provider and five scene
callers. Borrowed scene position arrays now use float elements and table symbols
use the shared record; original identifiers and addresses remain documented.
All seven source units preserve the complete strict object baseline.

### Shared item-kind availability selector

The checked signed halfword at item-kind offset 0x72 is now `availabilityField`.
Toolbox availability writes, registry queries and maze progress reads establish
its saved-field role; Ghidra 0x8010E6C4 confirms the signed load and 0x74 stride.
The maze uses the existing shared `ItemKind` layout rather than byte arithmetic.

`item_kind.h` now owns the canonical table declaration and its original symbol
note. Four consumers lose duplicate declarations, and save progress loses an
unused private 0x74-byte prefix and table declaration. Every source-unit change
and shared-header dependent preserves all 483 strict object comparisons.

### Item classification and effective-motion queries

`item_queries.h` supplies capacity, category, class, availability-selector and
effective-motion contracts to registry, script and physics users. Script
callers now agree with the provider's signed byte class and signed halfword
availability return. Ghidra 0x80115020 confirms the signed class load.

The kind's checked float at 0x28 is now `motionCoefficient`: the accessor's
result controls downward acceleration and horizontal friction, with a state
flag reducing the coefficient by half. Shared-header dependents and every
converted caller preserve all 483 strict object comparisons.

### Nearest-item and category-query contracts

`item_search.h` consolidates the nearest-item provider and two script callers,
including borrowed result lifetime, inclusive kind bounds, hold-state filtering,
angular limits and shared-state non-reentrancy. A stale integer view-transform
argument note is removed; the existing call already uses the float contract.

Private `ItemCategoryRange` now names its category and inclusive kind bounds.
Ghidra 0x80114FD4 confirms the twelve-byte stride and bound offsets four/eight.
The private nearest-query state documents score versus raw distance, copied
origin, angular transform and filters, with checks for size 0x5C and retained
interior-symbol offsets. All changed units preserve the strict object baseline.

### Item-search temporary comparator keys

The kind-search helper now reuses `RuntimeObjectPrefix` and `ItemStatePrefix`
inside explicitly temporary key storage. Its former message/request names
misrepresented the comparator inputs: offset 0x38 is the object handle and
0x54 is the private-state pointer; the compared kind is at state offset four.
Ghidra 0x80112F60 confirms the two handle boundary searches and state pointer.

Checked 0x5C/0xF4 storage extents preserve the original stack allocation. Only
kind, handle and state pointer are initialized, and the comparator contract
now distinguishes a temporary key from the live registered item. The existing
noncanonical lookup declaration remains the recorded IDO matching exception.
The shared-prefix representation preserves all 483 strict object comparisons.

### Item mode and interaction eligibility contracts

`item_mode.h` consolidates the setter across its provider and eight consumers.
The setter uses shared state flags and the signed-byte mode field instead of
word/byte offset views. Ghidra 0x8011443C confirms flags at state offset zero
and mode at offset seven. Removing the velocity helper's unused third setter
argument preserves the ROM and strict objects; its obsolete ABI note is removed.

Pickup and cooperation queries now share `item_queries.h` contracts across
registry, actor queries, companion and target interaction. Every source-unit
conversion and shared-header dependent preserves all 483 strict objects. Mode
values without sufficiently established semantics retain numeric spelling.

### Item release, launch and participation interfaces

`item_participation.h` consolidates release, throw, search registration and
search/collision exclusion across eight units. The script release caller now
passes its resolved object pointer through the provider's actual contract.

Release uses shared item fields and the actor's checked held-item handle rather
than raw offsets. Ghidra 0x80114044 confirms owner lookup, holder clearing,
release callback and reset fields. Hold-state dispatch retains an unsigned byte
view of the named signed member because a value cast changes IDO output; the
source note and ledger record that exception. All changed units preserve the
complete strict object baseline.

### Item launch motion and transform accesses

The launch path now names object rotation, radius and position; item velocity,
rotation rates and spin; the primary actor handle; the private-state pointer;
and its shared launch callback type. Ghidra 0x8011418C confirms the placement
and velocity sequence without changing control flow or floating expressions.

Isolated trials show that direct owner, hold-state, flags and kind members each
change ROM bytes in this routine. Those four read patterns remain explicit
matching exceptions in the source and ledger pending IDO scheduling evidence.
All accepted member conversions preserve the full 483-object strict baseline.

### Complete item-owned state allocation

`item_state.h` defines the allocator's 0xF0-byte state as the existing shared
prefix, unresolved bytes at 0x6C, owned direction animation at 0x70 and an
unresolved four-byte tail at 0xEC. Size and direction/pose-index/tail offsets
are checked. Ghidra 0x8011386C confirms allocation and clearing of 0xF0 bytes,
direction reset at 0x70 and separately owned lighting allocation.

The registry's private direction view is removed. Allocation/clearing use the
complete size, while lighting, reset, update and timed-motion consumers address
the named direction member. Prefix users retain their partial-view contract;
unknown bytes have no invented names or semantics. All conversions preserve
the complete strict object baseline.

### Item update state and kind configuration

The frame update now uses shared object links/flags and item hold state, flags,
kind, material activation, collision key and idle timer. Its kind configuration
pointer is typed and names the search-priority growth limit and rate. Ghidra
0x801135D4 confirms these widths and update stages.

Search-priority arithmetic at 0x38 retains an explicit raw float view because
the named member changes IDO floating scheduling. Forced source-unit rebuilds
isolate that exception; accepted conversions preserve all 483 strict objects.
The original debug-only subject branch remains unchanged.

### Item behavior mode dispatch

`ItemMode` names the seven ROM callback slots: ground/air, drifting flight,
stationary world placement, player-held, actor-held, stored and shrinking
disposal. The callback table at 0x8012B950 and its resolved function addresses
establish the mapping; Ghidra 0x80123390 confirms signed-byte mode dispatch.
The field remains signed-byte storage and the setter retains its integer ABI.

`item_mode.h` owns the update-handler type, table and dispatcher declaration.
The stationary handler now takes the common unused item pointer. Disposal
reuses `RuntimeObjectPrefix` instead of a duplicate scale/rotation view, and
physics transitions use named established modes. Shared-header dependents and
each source change preserve the complete strict object baseline.

### Named mode transitions and actor-held placement

All item-mode setter call sites now use the established `ItemMode` constants,
including nested scene creation calls, storage transitions and player/actor
attachment transitions. Each of the eight caller-unit conversions preserves
the complete strict object baseline.

Actor-held placement now names the item owner and mode timer, holder private
state, actor orientation, and object radius, position, world position, rotation
and scale. Ghidra 0x80122DE0 confirms holder lookup and attachment placement.
The shared-field representation preserves all 483 strict objects without a
new matching exception.

### Drifting flight and overloaded item phase storage

Drifting flight now uses named private state, velocity, radius and position
fields. Ghidra 0x801228F4 confirms the elapsed-time limit, oscillation and
terrain-constrained displacement. All converted accesses preserve strict
allocated-object bytes and relocations.

`ItemMotionPhase` documents the proven reuse of offset 0x30: piñata angle in
degrees, drifting-flight elapsed seconds and held light-bulb countdown seconds.
The union remains four bytes at checked offset 0x30. Flight, balloon launch,
piñata and light-bulb consumers use their corresponding views; the countdown
pointer also uses its named member. No additional state storage is introduced.

### Shared held-item and collision callback contracts

Physics now reuses `ItemHeldCallback` and `ItemCollisionCallback` from the
shared catalog instead of private function-pointer types. The held wrapper
accepts an object pointer and supplies only that pointer to the callback;
removing the unused state argument preserves all object bytes. Ghidra
0x80122A6C confirms callback lookup by the item's signed kind.

Collision dispatch names private state, kind, flags and world position. Both
callback lookups use the checked catalog size for their stride while retaining
interior symbols to preserve strict relocation identities. The impact-message
legacy return declaration remains its existing matching exception.

### Shared item sizing and suspension contracts

`item_size.h` consolidates uniform sizing, incremental shrinking and enabling
shrink updates across six units. The script sizing caller now passes the
resolved object pointer directly. Ghidra 0x80113348 confirms the shared radius,
scene/kind scale and bounds-refresh behavior. Function contracts describe
relative units, valid resources and return behavior.

The participation header now also owns hide/suspend and restore declarations
for script and scene users. Suspension addresses the checked collision-key
member instead of a raw offset. Every converted source unit and shared-header
dependent preserves all 483 strict object comparisons.

### Shared persistent profile counters

`profile_counters.h` owns the byte-returning read and saturating adjustment
contracts and the 83-byte storage declaration for 166 four-bit counters.
Script consumers now use the provider's `u8` return type instead of private
`s32` declarations. Profile reset derives its clear extent from the shared
storage bound. Ghidra 0x80114D04 confirms signed adjustment and saturation
at zero and fifteen; callers must supply a valid index and an adjustment
whose intermediate sum fits `s32`. All converted units preserve strict
object bytes and relocations.

### Player-view search interfaces

`player_view_search.h` consolidates item, actor and general-object handle
queries used by actor scripts. Contracts distinguish inclusive item distance
from exclusive actor/object distance and preserve their different cone limits
and kind aliases. Returned handles use the shared object member. Ghidra
0x80118544 confirms the player origin, combined viewing angle and handle
lookup. Direct named player-angle reads change ROM code generation; those
float-array reads remain a recorded matching exception. Provider and caller
changes preserve all strict objects and the clean ROM match.

### Actor destination fields and contracts

Escape-position selection now names the shared object world position and
actor target-position fields instead of raw offsets. Ghidra 0x8011866C
confirms the source and destination coordinates, randomized escape direction
and fallback behavior. `actor_target.h` also owns target-distance and fallback
position contracts; command callers use those declarations and the unused
menu declaration is removed. Each changed unit preserves all strict objects.
The target setter and escape wrappers were subsequently consolidated as
described below.

### Canonical actor destination updates

`actor_target.h` now also owns the target-position copy and escape-position
contracts with three-float input positions and `void` returns. Removing the
unused trailing object arguments in query wrappers preserves all 483 strict
objects; the former unprototyped declarations and portability notes are no
longer needed. Interaction, command and message consumers use the same
contracts and pass named world positions or their existing float arrays.
Ghidra 0x80118644 confirms the three copied coordinates; escape selection
retains its documented live-fallback precondition and original arithmetic.

### Held-item and pickup-history queries

`actor_held_item.h` consolidates signed-halfword kind queries and forced
release across three script units, correcting one private `s32` declaration.
Contracts retain the live-item precondition and stack-padding matching note.
Ghidra 0x80118938 confirms held-handle lookup, forced release and signed kind
return. `item_queries.h` owns the pickup-history predicate; its provider uses
the checked item flags member instead of a raw state word. Every converted
unit preserves all strict object bytes and relocations.

### Actor facing and player-object destination contracts

The actor facing-cone wrapper now accepts an object pointer and names its
world position and rotation, matching the general-object wrapper. Its shared
contract lives in `object_target_search.h`; Ghidra 0x80118984 confirms the
coordinate pointers and returned handle. `actor_target.h` consolidates player
primary-object tracking, secondary-object escape tracking and moving to the
stored destination for message consumers. All declarations retain original
function/address notes and each conversion preserves strict object bytes.

### Object distance and interactable-item query contracts

`object_distance.h` consolidates origin, player, target-handle distance,
relative height and horizontal-distance queries. Contracts preserve their
separate unavailable-target results and distinguish local-position player
distance from world-position searches. Ghidra 0x80118838 confirms the local
position offsets; the provider now uses the checked position member.
`item_search.h` owns the signed-halfword interactable-item kind query, fixing
the message caller's wider private return declaration. Item searches name
their world-position input. All converted providers and consumers preserve
strict object bytes and relocations.

### Target tracking and world-coordinate query contracts

`actor_target.h` owns horizontal target distance, target coordinate and
object-handle destination/tracking operations. `object_transform.h` owns
world-coordinate queries by pointer and available handle, including their
distinct unavailable-handle and invalid-axis results. Ghidra 0x80117480
confirms the signed upper-bound check: negative axes remain unsupported,
not silently accepted by the documented contract. Both object-coordinate
providers now name world-position members. Message and world-script users
share canonical declarations; all changed units preserve strict objects.

### Action-script and movement-result status contracts

`action_script_playback.h` consolidates elapsed time, duration and current
script ID queries, preserving seconds, the negative unlimited-duration
convention and stopped ID of minus one. `actor_movement.h` owns the unsigned
halfword movement-collision result query. Ghidra 0x80117500 confirms the
halfword at actor-state offset 0x1EA; this interface preserves its bits rather
than inventing flag semantics. Message commands share provider declarations.
All affected units preserve strict allocated bytes and relocations.

### Actor target modes and remaining tracking contracts

`ActorTargetMode` names fixed destination, follow and escape values proven
by tracking producers and destination-refresh consumers. All explicit mode
assignments and comparisons in six units use these constants. Storage and
script-supplied values retain their signed 32-bit representation; the enum
does not impose a new validity check. Ghidra 0x80117834 confirms the word
setter at actor-state offset 0x19C. `actor_target.h` now also consolidates
escape selection, available-target lookup and mode get/set contracts.
Each affected unit preserves all strict objects and relocations.

### Parent tracking and target-facing motion fields

Parent tracking and target-facing child updates now use shared hierarchy-root,
private-state, object-kind, world-position and rotation fields. Ghidra
0x80015620 confirms the target lookup through offset 0x50 and camera-eye
special case. Contracts identify that link as the hierarchy root rather
than assuming an immediate parent. The target-facing update names the child
playback-mode field and existing mode constants, retaining its unsigned
halfword interpretation. The target local is descriptive. Both isolated
field and mode conversions preserve all strict object bytes and relocations.

### Camera-facing and actor orientation fields

Camera-facing child updates now name the hierarchy root, actor inhibit flags,
child playback mode, world position and rotation. Ghidra 0x80015AE8 confirms
the root actor's camera-facing attribute and inhibit test, active-camera eye
coordinates and child mode selection. The unsigned mode interpretation and
original arithmetic remain intact. The adjacent actor orientation helper
uses actor orientation, root-motion transform and object rotation members,
with descriptive state/matrix locals. Every conversion preserves strict
allocated bytes and relocations without a new matching exception.

### Actor drawing and orientation interfaces

The referenced-matrix drawing callback now uses shared root, matrix-buffer,
flags and display-list members. Ghidra 0x80015E34 confirms double-buffer
selection and the returned drawing link. The single-axis orientation setter
now indexes the checked orientation array directly, removing its byte-offset
local without changing code. `actor_rotation.h` owns object-rotation refresh
and action-conditioned turning contracts, preserving finite-angle and
normalization preconditions. Additive axis helpers retain their existing
shifted float view pending a matching representation. All conversions match
strict objects and the clean ROM.

### Typed actor part tables

`ActorScriptState.partTable` is now an owned array of `partCount` borrowed
object pointers, rather than an untyped byte pointer. Allocation, clearing,
lookup, hierarchy population and playback indexing establish the element
width and ownership. Ghidra 0x800163FC confirms indexed pointer lookup.
Allocation derives its extent from the pointer element size, and population
names object kinds, child/sibling links and shared actor state. Default-part
lookup and part world-position reads use object members. All dependent units
and isolated consumer conversions preserve strict objects and relocations.

### Attachment selection and part-table lifecycle contracts

`actor_parts.h` consolidates part-handle lookup, configured attachment
position, empty table allocation and hierarchy population across motion,
configuration, model setup, action commands and item physics. Physics passes
the actor pointer directly instead of carrying it as an integer address.
Attachment selection reads the checked state member with its original
unsigned switch interpretation. Ghidra 0x800164FC confirms the midpoint,
configured first/second parts and fixed part-ID cases; uncertain anatomical
meanings remain numeric. Contracts document borrowed parts, allocation
ownership, output extents and unsupported-mode behavior. All conversions
preserve strict allocated objects and relocations.

### Ground-reference and motion vector fields

Named-part flag updates, lowest reference height and ground-clearance queries
now use checked object flags, kind and world-position fields plus actor
ground offset. Ghidra 0x80016BA8 confirms the reference-part minimum and
horizontal terrain query. Motion transformation names local/world vectors
while retaining the documented unused trailing state argument. Ground-pose
updates name pose displacement, accumulated vertical offset, actor ground
offset, orientation and output transform fields. Original floating expression
order is unchanged. Both isolated conversion groups preserve all strict
allocated bytes and relocations without new matching exceptions.

### Actor behavior inhibition contracts

`actor_behavior.h` names the four established inhibition bits for target
facing, camera facing, horizontal movement and terrain/gravity updates.
Other bits retain unresolved roles. Producer script toggles, presentation
setup and motion consumers use the constants; storage and helper parameters
remain unsigned halfwords. Ghidra 0x80017190 confirms the flag update at
actor-state offset 0x1E8. Setter/clearer providers use the shared field and
presentation/map callers share canonical declarations. Each changed unit
preserves all strict object bytes and relocations.

### Positive actor behavior controls

The shared behavior enum also names cooperation enablement for actor kind
sixteen and preservation of appearance variants across action changes.
The former is written by object activation and checked by cooperation
requests; the latter is written by appearance selection and checked before
action-start appearance reset. Ghidra 0x80119210 confirms the preservation
bit update. The enum description distinguishes these positive settings from
inhibition bits without changing the stored halfword or legacy field name.
All producers and consumers preserve strict object bytes and relocations.

### Unified actor behavior-word naming

The mixed-control halfword is now `ActorScriptState.behaviorFlags`, covering
inhibition, cooperation enablement and appearance persistence without
implying that every set bit disables behavior. All named consumers use the
new field. The visibility producer and model-update consumer establish
`ACTOR_INHIBIT_INTERACTION_VISIBILITY`; Ghidra 0x80015488 confirms its update
gate. Remaining cooperation and presentation save/restore raw reads use the
checked member. Cooperation permission also applies to actor candidates in
the interaction classifier, so its contract is not restricted to the single
activation producer's kind. All dependent and isolated conversions match.

### Presentation snapshots reuse shared actor state

Presentation save/restore uses `ActorScriptState` for its 0x408-byte behavior
snapshot. Its private object snapshot embeds the checked runtime prefix and
retains four unresolved trailing bytes, with a 0x5C size assertion. Ghidra
0x80028518 confirms both copy extents. Snapshot comments make the shallow
resource lifetime explicit; this does not clone or transfer actor ownership.
Restore names object transform/radius and actor orientation, root-motion
matrix/heading and behavior fields, removing duplicate block types and
float-index macros. The existing action-stop ABI matching exception remains.
Each isolated conversion preserves strict object bytes and relocations.

### Actor-kind stride and initial attachment selection

The actor-kind record's offset 0x4C is now `initialAttachmentMode`, correcting
its former behavior-flags interpretation. Ghidra 0x8001E1C4 confirms its copy
into actor attachment state. All configuration lookups derive their stride
from the checked 0x54-byte private record while retaining interior linker
symbols and strict relocation identities. Creation reads the orbit follower's
handle through the shared object field. The source-quality summary is updated
to current counts, with uncertain globals and raw candidates still explicit.
All isolated conversions preserve strict objects and the clean ROM.

### Sway and roll state recovery

Sway updates now reuse `RuntimeObjectPrefix` for rotation and the first-child
link, removing a duplicate private object view. The separate eight-byte
`ObjectRollMotionState` names the initial roll and velocity, with checked
size and velocity offset. Ghidra 0x8001DADC confirms allocation, clearing and
captured roll. The reset/update ordering and arithmetic are retained exactly;
no unreachable-looking branch is removed. These private layouts remain local
to configuration, and both isolated conversions preserve strict objects.

### Billboard integer-animation state

Billboard initialization and drawing reuse the checked twelve-byte `ValueTrack`
with typed borrowed key-program and owned state pointers. Allocation derives
its extent from the shared type; frame evaluation uses a descriptive output
local and named object flags/display list. Ghidra 0x8001DCFC confirms the
allocation and animation initialization. Integer-animation headers now mention
object texture users and retain original function/address notes. Both provider
conversion and dependent header rebuild preserve all strict objects.

### Actor resource descriptors reuse corrected-asset ranges

The private 0x24-byte `ActorAssetDescriptor` now embeds the shared
`AssetDescriptor` after the resource index and compression selector, with
size and member-offset checks. Cache initialization establishes the index;
queued reads and actor creation establish ROM/decoded ranges and biased
encoded/correction metadata. Ghidra 0x8001E04C confirms the allocation ranges
and 0x24-byte resource stride. The former `remaining` array is replaced by
named encoded end, correction start and correction-count metadata through
the shared type. Ownership stays with the cache and resident tables; the
record remains private. All isolated conversions preserve strict objects.

### Shared actor asset-cache lifecycle

`actor_assets.h` consolidates cache initialization, read-queue teardown,
request/wait operations and borrowed storage lookup. The contracts distinguish
reserved storage from read completion and corrected decompression readiness.
Ghidra 0x8001E190 confirms the cached-pointer lookup. Runtime and drawing
consume canonical declarations; drawing names the object kind directly.
The private storage table has thirty-six pointer slots, initialization derives
its clear extent from that bound, and staging/read destinations use pointers.
The lookup's retained address temporary preserves its existing control flow.
Every isolated provider and consumer conversion preserves strict objects.

### Shared VM lifecycle contracts

`script_vm_lifecycle.h` provides canonical creation, reset and release contracts
with borrowed-resource lifetimes and owned-context responsibilities. Creation
accepts the existing partial `ScriptVmConfiguration` view, and allocation and
reset extents derive from checked VM/value types. Reset uses the named saved-depth,
return-offset and local-frame arrays. Actor creation names the ready-state write;
model teardown uses the canonical pointer parameter. Ghidra 0x80115114 confirms
context allocation, borrowed configuration fields and the separately owned value
stack. The actor creation boundary retains its generic resource parameter until
its scene callers can be audited together. All isolated conversions preserve
strict object matches.

### Shared VM scheduling and global-variable interfaces

`script_vm_execution.h` defines bytecode-run and frame-update contracts, including
scheduling results, bytecode bounds and absent-context behavior. The interpreter
uses shared actor/object fields to locate its context. `script_vm_globals.h`
consolidates reset and integer-assignment contracts for the 500-value global bank;
reset extent derives from its existing typed storage. Seven consumer units use
canonical declarations. Ghidra 0x80116658 confirms timer and scheduling behavior.
A direct pointer cast in the companion caller changed register allocation and
failed ROM matching; a descriptive local actor pointer preserves the original
instructions and permits the canonical declaration without an exception.
All provider and consumer conversions preserve strict object matches.

### Typed VM variable references and borrowed string results

The variable-reference decoder returns `ScriptValue *`, uses the checked local
frame-base array and indexes the existing global/local value storage. Ghidra
0x801156A0 confirms frame -1 for globals and nonnegative local-frame bases;
invalid negative frames remain an explicit precondition/UB note. Indexed writes
use the value's byte representation. Opcode 33 retains its five-byte array view:
a typed indexed-address trial changes allocated instructions and fails matching,
while the independently tested write conversion preserves strict objects.
The string-pop contract now lives in `script_vm_stack.h`, including borrowed
string lifetime and shared numeric-text invalidation. Native command consumers
use its pointer result and canonical context parameter; an unused menu declaration
and three duplicate provider declarations are removed. The string-table lookup
uses the checked context member. All accepted conversions preserve strict objects.

### Native command scheduling results

`script_vm_commands.h` documents the native-command dispatch boundary and names
continue, yield and action-wait results separately from retained VM states.
The dispatcher names its command index/callback, native action commands use the
request constants, and action handlers return named retained states. The
interpreter consumes the canonical dispatch declaration. Ghidra 0x8011BD50
confirms the command-table lookup; matching assembly additionally confirms both
actor and context argument registers, which Ghidra's inferred signature omits.
The callback ABI and integer return types remain unchanged. Every isolated unit
conversion and shared-header rebuild preserves strict objects.

### Packed script-value conversion contracts

`script_value.h` exposes typed float reading and numeric/borrowed-string assignment
contracts. Their implementations and all call sites use `ScriptValue *`, named
byte-aligned tag/payload members and the established tag enum. The global integer
setter indexes the existing value bank directly. Ghidra 0x8011549C confirms tag 1
as the floating-point representation and integer conversion otherwise. Payload
copies retain their unaligned-safe representation and original operation order.
Conversion and decimal-string parsing locals now name the numeric values, string
index/address and category tests. The integer reader retains its documented
legacy definition/extra-argument compatibility constraint. All conversions and
header rebuilds preserve strict objects.

### Borrowed VM string-resource layout

Both `ScriptVmConfiguration` and `ScriptVm` now describe their string resource
as a borrowed table of borrowed null-terminated strings. Creation copies the
table reference, string-pop indexes it directly, and decimal parsing retains a
string pointer until the existing integer-address parser boundary. String-value
assignment accepts a borrowed string pointer; the address-based stack wrapper
converts explicitly. Ghidra 0x801155B0 confirms indexed address lookup and direct
string-address decoding. New offset checks cover configuration program, strings,
commands and capacity, and the VM's program/string pointers. The complete resource
allocation and configuration word 0x0C remain unresolved. All typed conversions
and dependent header rebuilds preserve strict objects.

### Actor pose, shake and meter consumers

Model-part primary/secondary pose followers, relative/blended pose updates,
orientation composition and shaking use the checked object rotation fields.
Follower scale/root/state accesses, both meter owners, the primary VM clock and
held-item diagnostic lookup reuse established object/actor members. Gameplay
pose evaluation names its controller words instead of computing their offsets;
the subsequent orientation-cache recovery establishes their capture policy. Ghidra 0x80013B90 confirms secondary
pose transforms and 0x80014A3C confirms owning-root state and action-resource
access. Arithmetic, control flow and the separate orientation-cache ABI remain
unchanged. Both isolated source-unit conversions preserve strict objects.

### Shared actor orientation-reference caches

`ActorOrientationCache` is a checked 0x34-byte record containing a capture request
and saved inverse-reference matrix at offset four. Ghidra 0x800142E0 confirms
reference capture, identity output on capture and subsequent relative evaluation.
The record replaces unresolved actor regions at 0x2F8 and 0x368 as secondary and
primary pose references, retaining both actor offset checks and total state size.
Single/paired model-part allocation and initialization derive their extents and
member accesses from the same type, and paired evaluation advances by one record.
The canonical evaluator contract describes initialization, output, signed part
sequences and ownership. Two duplicate declarations are removed. The full shared
header rebuild and both isolated provider conversions preserve strict objects.

### Linked model-part rotation definitions

The private checked eight-byte `ActorPartLinkedRotationDefinition` records a
halfword linked-part identifier, halfword axis and float response factor. Ghidra
0x800137C0 confirms widths, record stride and initialization of the owned
sixteen-byte follower state. Resident table and initializer use the recovered
record; allocation and clearing derive their extent from the state type. The
existing biased table address is retained to preserve strict relocation addends.
Callback locals now describe linked part, axis, angle source, delta and response.
Direct indexed rotation fields fail ROM matching, so the explicit named byte
offset is retained and recorded with the next compiler-evidence question.
All accepted source-unit conversions preserve strict objects.

### Actor texture-state consumers and track cursors

Texture allocation and clearing derive from checked `ActorPartTextureState`;
release uses its pointer and collision key. Tint/channel lookup uses the object
state member, animation selection indexes the three tracks directly, and model
radius uses its shared object field. Default key-program globals now borrow
`ValueKey` pointers. Ghidra 0x80014960 confirms channel rates and twelve-byte
track indexing. Drawing advances a typed evaluated-track cursor and named rate
array. Its separate retained-value byte cursor remains: moving both cursors to
the track-array base changes instructions and fails ROM matching. That constraint
is recorded without inventing an overlapping structure view. All accepted
source-unit conversions preserve strict objects.

### Shared resident texture-animation program table

`texture_animation_programs.h` consolidates nine source declarations into a
borrowed resident `ValueKey *` table used by actors, items, effects, markers,
text windows and HUD indicators. Consumers pass typed entries directly instead
of integer-to-pointer casts, and item material animation removes manual word
addressing. Ghidra 0x801145D0 confirms the item track/table lookup. The full table
bound remains explicitly unresolved; supported identifiers come from each
consumer's resources. Original global/address notes are retained at the canonical
declaration. Every consumer conversion was built and compared in isolation;
all nine preserve strict object matches.

### Opacity programs and attached-effect state

Four consumers share the borrowed resident `ValueKey *` opacity-program table;
attached-effect initialization names default entry zero. The complete table bound
remains unresolved. Private `AttachedEffectDefinition` names hierarchy template,
scale and opacity selection. Checked `AttachedEffectState` covers its complete
0x2C-byte allocation: effect kind, reverse-facing flag, borrowed owner, playback
rate, two value tracks, RGBA color and scale. Existing padding is retained and
initialization does not clear previously uninitialized bytes. Actor state now
names its latest attached-effect handle at checked offset 0x404, replacing only
the supported final word of its unresolved tail. Ghidra 0x80013130 confirms the
state fields, replacement handle lookup and publication of the new handle.
All four table conversions and the state/header conversion preserve strict objects.

### Attached-effect creation and placement contracts

`attached_effect.h` consolidates three caller declarations and the provider's
creation contract: initialized actor/anchor lifetime, supported resource indices,
previous-effect destruction request and object-system-managed result lifetime.
Placement/drawing use checked object position, rotation, scale, radius, world
position and display-list members. Camera-distance evaluation uses the checked
`MatrixCamera.eye` layout instead of a 56-float array view. Ghidra 0x800132E0
confirms anchor world position, camera-relative distance and owner size inputs.
Every provider conversion and all three consumer rebuilds preserve strict objects.

### Linked-effect motion and appearance state

The private checked 0x58-byte `LinkedEffectState` names owned motion state,
borrowed neighbors, target handle, playback rate and contact handoff. Two float
arrays become `ValueTrack` frame/opacity records. The documented callback receives
an output velocity and returns one to bypass damping and neighbor pulls; Ghidra
0x8001C9D8 confirms that contract. Ghidra 0x8001CD24 confirms contact consumption
and transfer to the next effect. Allocation and initialization derive their
extent from the state type, while motion, teardown and appearance use its named
fields and checked offsets. Three redundant declarations are removed; arithmetic,
neighbor unlink ordering and callback ABI remain unchanged. All isolated
source-unit conversions preserve strict objects.

### Linked-effect creation and hierarchy contracts

`linked_effect.h` defines supported initial motion kinds and the chain creation
contract, including target lookup, animation indices, neighbor constraints and
object-system-managed group lifetime. Three callers adopt canonical declarations
and named rebound/falling kinds. Motion and target queries reuse object position,
world position, radius, flags and owning-root fields; chain construction names
child/sibling links and state pointers. Drawing uses its retained frame value and
object display list. Ghidra 0x8001D3B8 confirms chain initialization. A named radius
sum initially swapped operand registers; target-plus-effect source order preserves
the original instruction bytes, documented as a matching constraint. All provider
and consumer conversions preserve strict objects.

### Interaction-marker track roles and style contracts

Six named marker track roles replace numeric frame, opacity, depth, height and
scale selections. `InteractionMarkerStyle` names program indices, sound identifier
and target-dimension factors, with size and offset checks. Transform programs
borrow typed `ValueKey` streams, marker allocation derives from its checked state,
and retained height/scale values use track members. Ghidra 0x8001EAF4 confirms
track selection and placement. Creation now returns the registered object pointer
through the existing shared interaction header. Drawing uses named display-list
and held-item fields; placing the actor handle first in the equality preserves
original register allocation, recorded as a matching constraint. Provider changes
and the shared-header rebuild preserve strict objects.

### Interaction classification and region interfaces

The existing interaction header names independent primary-region, secondary-region
and camera-visibility flags and consolidates player classification and four region
adjustment contracts. The classifier uses checked player selection/action,
permission and held-item fields plus object identity/state members. Producer and
script region queries use the shared flag enum. Ghidra 0x80111784 confirms player
selection updates and 0x8001F078 confirms paired primary scale assignment. Named
held-item equality matches with player handle first; the alternative source order
swaps load registers and is recorded as a constraint. Kind-specific unresolved
state remains explicit. All isolated conversions and dependent rebuilds preserve
strict objects.

### Interaction proxy creation contract

`interaction_proxy.h` provides the shared creation declaration for the player
implementation and the bedroom and Viridian entrance consumers. The position
is three floating-point coordinates read during creation. Help text is borrowed
for the proxy lifetime; the registered object is managed by the object system.
The private proxy state supplies the classifier result through its established
field rather than an independent offset view. Ghidra at `0x80112940`, the two
scene callers, and the existing description consumer support these contracts.
The context retains its caller-defined meaning, including remaining uses for
consumable proxies; no common semantics are inferred for unrelated object kinds.

The proxy description reader (`0x80110C0C`) now shares the private state layout
with creation, classification and consumption. Its borrowed text pointer at
`0xC` replaces the unused integer-address union alias. Target size and field
offset checks cover the 16-byte state, and initialization uses that allocation
size. The reader's other object-kind views remain separate because their state
layouts have different contracts. Ghidra initialization at `0x801128E8` confirms
the allocation size, zero initialization and default description pointer.

### Inspection layout consolidation

Player target-description preparation now uses `PlayerControlPrefix` for the
inspector and `RuntimeObjectPrefix` for the selected world object. Item catalog
lookups use `ItemStatePrefix.kind`; door prompt selection uses `PassageState.kind`
and `accessState`. This removes the private `InspectObject` and `InspectState`
views that conflated independent record kinds. Existing target offset checks,
creation code, description consumers and Ghidra at `0x80110C0C` support the
replacement. The proxy description branch keeps its separate private type.

The interaction classifier also reuses the passage kind, destination and access
fields, with target offset checks in `scene_types.h`. Its held-item exclusion
reads the signed 16-bit item kind through `ItemStatePrefix`; this is independent
of the proxy's 32-bit classification result at the same offset. The inspection
reader uses the shared primary-region flag name. Ghidra at `0x80111784` confirms
the classifier's widths and passage branches; strict object comparison preserves
the established held-handle comparison operand order.

### Message-bar interface contract

`message_bar.h` supplies canonical queue, clear, initialization and release
contracts to all declaring game units. Queue input is borrowed for the call and
copied into retained storage; its established maximum is 59 encoded bytes.
Empty text and a full queue are rejected, while an idle queue can refresh a
prefix-matching displayed message. The toolbox now uses the fixed three-argument
interface; removal of its unused fourth argument preserves every allocated
object byte and relocation. The VM and script command consumers share the same
float duration and byte-text types. Source lifecycle code and Ghidra enqueue
at `0x80027988` establish the ownership and return contracts.

The message bar's private `MessageBarState` now names its queue cursors, display
timers and image slots. Size and offset checks protect the queue and image
layout. Enqueue writes use a typed `QueuedMessage`, preserving text ownership
and the 68-byte record stride. The unused, conflicting state view in the terrain
debug unit was removed; the implementation remains private. Initialization,
activation, update and drawing establish the field roles; the final 60 bytes
retain an explicit unresolved-storage name.

Message image preparation reads a typed queued record. Activation retains its
biased byte cursor and signed literal stride: direct typed duration indexing
and a `sizeof` stride independently failed the ROM gate. These matching
constraints are recorded in the ledger. The display entry point now shares
its documented declaration with both main-loop source units. Ghidra activation
at `0x8002767C` confirms queue duration and copied text roles.

### Text-window presentation interface

`text_window.h` unifies open, append, first-page preparation and mode-entry
declarations in the provider and all declaring consumers. Pending text is
copied during append, with excess text discarded at the existing capacity;
opening an active window is rejected. Page preparation reports whether another
page remains, while mode entry displays the pending message. Byte-string casts
at signed-text consumers preserve their established storage. Provider code and
Ghidra opening at `0x8002C348` establish the contract; each consumer passed strict
object comparison independently. Background drawing's separate legacy callback
contract remains outside these four presentation operations.

Text-window page history, page preparation and continuation storage now use
borrowed byte-text pointers consistently. The retained text must remain valid
while it is navigable in history. Indicator initialization and drawing reuse
the existing `ValueTrack` layout rather than a duplicate three-word record.
Unused text-window declarations in the collision unit were removed. Ghidra page
preparation at `0x8002C03C` and provider readers/writers confirm continuation
roles; pointer recovery passes both the ROM gate and strict object comparison.

Text-window lifecycle phases now use private named constants for opening,
awaiting input, changing pages, closing and inactivity. The shared presentation
header also covers initialization, update, embedded drawing, mode drawing and
deactivation. Runtime, toolbox and main-loop consumers use these declarations.
Deactivation retains resources; full completion releases displayed images and
returns to gameplay. Provider transitions and Ghidra update at `0x8002C6A4`
establish these distinctions without changing state storage or callback ABI.

### Shared font configurations

`text_image_configurations.h` declares resident `FontImage` descriptors reused
by message bars, text windows, panels, HUD, title, collection and toolbox text.
The scrolling panel's configuration pointer now uses that established type.
The descriptor stride is 16 bytes; initialization, renderer consumers and Ghidra
configuration selection at `0x80012CF0` support it. Complete table bounds remain
unspecified. The system menu retains a byte view because typed declarations
change strict relocation addends despite a ROM match; the ledger records the
trial and the evidence still needed. Toolbox measurement now uses the recovered font-descriptor interface.

Font measurement now carries `FontImage` through the public measurement wrapper,
text-width evaluation, character advances and side-margin checks. Cell width
reads use the established descriptor field. The custom-font bitmap builder now also uses the font descriptor; its
cell dimensions and glyph reads share those types. Ghidra measurement at
`0x8003ECD8`, character-boundary queries and the toolbox title consumer establish
the recovered roles, and all objects remain identical.

Custom-font image creation uses `FontImage` throughout, and glyph reads return
borrowed byte pointers. The ROM-cache API's integer output remains a narrow
explicit boundary. Image-width queries use a typed three-argument prototype;
removing the unused fourth argument preserves strict object matching. Descriptor
checks now cover width, height and pixel-base offsets as well as metrics. Ghidra
glyph reading at `0x8003EC40`, bitmap creation and the cache lifetime contract
confirm the recovered pointer roles.

### Voice messenger launch contract

`messenger.h` shares camera-launch and projectile-creation declarations between
the provider and voice-command consumer. Coordinates, velocity and script
message are copied; the object system owns projectile lifetime. Camera-relative
launch uses the established `MatrixCamera.view` and eye position, replacing a
private matrix duplicate. The voice candidate distance path and messenger
creation reuse runtime-object positions, scale and handles. Ghidra launch at
`0x80026C58`, creation and delivery consumers support the homing/direct and
visibility contracts. All affected units pass strict object comparison.

Messenger flight callbacks share named launch, homing, dissipation, rebound and
collision-flight modes. Existing state fields replace timer, velocity, linked
target and copied-message offset views. Object state, hierarchy roots, kind,
positions and handles reuse `RuntimeObjectPrefix`. Allocation uses the checked
64-byte state size, with checks for timer, phase, target and animation offsets.
Ghidra update at `0x80026798`, transition writers and delivery readers establish
these roles. The draw callback retains its narrower signed-halfword opacity
view; it must not be replaced with a full-word animation value without evidence.

Flipbook geometry, row-list reset and text replacement reuse its private
record fields, including the two embedded text panels. Pending swaps, active
banks, row branches, color and outline indices have descriptive field names
and target offset checks. Outline reads retain signed-byte interpretation of
the 255 disabled sentinel. The model owns drawing allocation and panel entry
storage; existing panel color retention is unchanged. Ghidra text replacement
at `0x8002588C`, geometry initialization and bank selection support these roles.

Scrolling-board state now has a checked private 136-byte layout containing
borrowed text addresses, first visible line, text count, row offset, signed
speed and its owned flipbook. Update, drawing, text assignment and speed changes
reuse this layout, including the model's visible-row count. Text entries now use borrowed byte-string pointers consistently through
the public hierarchy interface and preset arrays. Ghidra update at `0x80025A44`, allocation
and text assignment establish the size, offsets and resource lifetimes.

`scroll_board.h` shares hierarchy creation, borrowed-text assignment and speed
contracts with script and preset consumers. The private flipbook text interface
and board state use the same string-pointer arrays. The two preset arrays have
a checked eight-byte descriptor instead of generic integer words. Ghidra text
assignment at `0x80025C5C`, null-terminated entry traversal and preset selection
establish pointer roles; hierarchy wrappers use the existing child-object link.

The uniform-transform helper is declared in `object_transform.h`, documenting
readable position, writable normalized rotation and uniform scale. Script board
creation and transform commands use real three-float arrays, with rotation
named according to its role. Their VM, scene scale and created-object handle
accesses reuse established layouts. Ghidra at `0x80025DF4` confirms in-place angle
normalization and local/world position copies; the vector representations
preserve every allocated object byte and relocation.

### Particle emitter interface

`effect_emitter.h` shares creation and named emission styles across the provider
and its four consumers. Callback selection establishes black/white wisps,
colored particles, scattered particles and audible bursts. Fallback position,
expiry, target and reference handles and signed offset have documented
contracts. The script emitter command uses a three-float array with descriptive
argument names; the final duplicate ScriptVector record was removed. Remaining
VM accesses in this script unit reuse actor state. Ghidra dispatch at
`0x8001940C` and callback signatures support the contracts; a stale integer
callback portability note was removed because callbacks already have a typed
function-pointer interface.

Emitter creation, target-relative updates and emission callbacks now use the
shared runtime-object transform fields. Creation and update carry typed private
emitter state, with descriptive target/reference locals and checked field
offsets. Allocation uses the established 24-byte state size. Ghidra update at
`0x800196D8` confirms target/reference lookup, signed horizontal offset, optional
callback and timer roles. Callback-local views that change to emitted-particle
state remain separate; their matching offsets do not imply identical records.

### Wisp state and ownership

The wisp creator, animation renderer, initializer and destructor share a checked
36-byte private state. Its two drawing banks share one owned allocation; texture
tracks are separately owned, while animation key data is borrowed. Named color
channels preserve the original compiler scheduling. Drifting motion uses the shared object transform fields, and colored emission
uses the same wisp color state as creation and rendering. The emitter callback
retains its local byte alias across the distinct emitter and emitted-wisp states.

### Ribbon controller state

Ribbon initialization and steering now share the checked private controller state.
The texture animation uses `ValueTrack`; destination, lifetime, steering cadence
and ground-contact state retain their original widths and offsets. Ghidra confirms
the access locations; the matching C remains authoritative for scalar types.

Ribbon steering and mirrored segment updates now use the shared object child,
sibling, root, rotation, position and world-position fields. The mirrored segment
retains the signed scale reversal and original update order.

Ribbon creation now populates the shared object transforms and controller state.
Drawing reads the checked animation value and the root matrix-buffer and flag
fields. The anchor retains its separately allocated pointer slot.

### Particle-cluster renderer state

The renderer owns one `ValueTrack`, with borrowed texture-frame key data. Its
initializer, updater, destructor and draw callback now share that type. Drawing
uses the shared root flags and matrix buffers. The original extra deallocator
argument remains documented for target ABI compatibility.

Particle-cluster creation uses shared object transforms, source handles, hierarchy
links and display-list fields. The root reuses `EffectEmitterState`: template 0x80078030 selects kind 169,
whose callbacks are `effectEmitterInitialize` (0x8001968C),
`objectReleaseStateAllocation` (0x800086E0), and `effectEmitterUpdate`
(0x800196D8). The initializer allocates and zeroes 24 bytes. Creation sets the
remaining lifetime and target handle; the zero callback disables emission.

### Falling-particle state

Creation and update share a checked 36-byte private state with lifetime, motion
style, world velocity, `ValueTrack` scale playback, and base scale. The initializer
allocates and zeroes exactly this size; animation keys remain borrowed.

Falling-particle creation also uses shared object transforms and display-list
fields. Its three motion styles are named billboard spin, billboard sway, and
tumbling, while the stored style retains its signed 32-bit representation.

The falling-particle creation contract and motion-style enumeration are shared
through `falling_particle.h` by the implementation, action-script commands and
maze scatter effects. Position and velocity inputs are copied; the object manager
owns the registered result. All callers retain the established signed scalar ABI.

### Converging-particle state

A checked 44-byte private state now connects allocation, creation, destruction
and lifetime/animation updates. It owns drawing commands and borrows texture and
opacity animation keys through two `ValueTrack` records. The update loop retains
its explicit byte alias when it switches from private state to scene objects.

Converging-effect drawing reads the typed texture value and the low byte of the
opacity value, preserving the original big-endian byte selection. Its motion
loop uses shared scale, position, world-position and sibling fields.

Converging-effect creation now uses the shared object transform, state-pointer
and sibling fields. Its action-script caller uses a checked operand view: signed style, floating
lifetime, and signed element count. `converging_particle.h` supplies the shared
creation contract. Typed lifetime access preserves the original script word bits
and matches the caller without an integer-to-float conversion.

### Flickering-effect state

Allocation, creation, update, drawing and destruction share a checked 12-byte
private state containing an owned display list, target handle and remaining
lifetime. The original extra deallocator argument remains documented.

Flickering-effect creation, following motion and drawing use shared object
transforms, child/sibling links, flags and matrix buffers. The initial sibling
chain is transferred into the root child link with the original ordering.

`flickering_effect.h` supplies the shared creation contract. The action-script
caller uses a checked count/lifetime operand record and the shared object handle
field; the lifetime remains a floating value at script-operand offset four.

### Animated radial state

Allocation, creation, update, drawing and destruction share a checked 36-byte
private state. It owns drawing commands, borrows texture and opacity animation
keys, and records its target handle and remaining lifetime. Opacity drawing uses
the low byte of the typed animation value, preserving the original selection.

Radial-effect creation, following motion and drawing use shared object transforms,
child/sibling links, flags and matrix buffers. Child Y-scale growth retains the
original floating expression and traversal order.

`animated_radial_effect.h` supplies the shared creation contract. Radial and
flickering action commands reuse the same checked count/lifetime operand record,
with descriptive field names and the original signed-count/floating-lifetime ABI.

### Expanding-orange state

Creation, initialization, update and drawing share a checked 24-byte private state
containing target handle, borrowed-key texture track, remaining lifetime and
uniform scale. Target unavailability does not end the effect before expiry.

Orange-pulse emitters use a separate checked 12-byte state with target handle,
remaining lifetime and emission cooldown. Expanding effects use shared object
scale, position, world-position and display-list fields. Both retain their
original expiry checks and target-availability behavior.

`orange_effect.h` supplies the creation contracts for individual expanding effects
and timed pulse emitters. Actor-script actions use the shared declaration; target
handles and floating lifetimes retain their established ABI and ownership rules.

`particle_cluster.h` supplies the creation contract to action-script, actor-action
and demo-item consumers. The demo caller now passes the source object as a pointer
through the canonical declaration; the original register arguments remain identical.

`ribbon_effect.h` supplies the shared ribbon-creation contract, including its
object-manager-owned return value and copied destination input. The script caller
now uses the canonical pointer return instead of its former void declaration;
its ignored return and all register arguments retain identical generated code.

`wisp_effect.h` supplies the creation contract to action scripts, collection
commands, item registry/physics and maze scatter consumers. All five consumers
and the implementation use the same signed style and floating-coordinate ABI.
