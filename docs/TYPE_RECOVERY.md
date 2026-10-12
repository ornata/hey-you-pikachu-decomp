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
| Address-named globals | 6,650 / 102 | 6,606 / 101 |
| `temp_*` | 2,480 / 58 | 0 / 0 |
| Raw offset candidates | 4,040 / 86 | 3,995 / 84 |

Address-global counts are references, not declarations: unique address spellings
fell from 995 to 984. Twelve globals were named, including the controller repeat
mask formerly named `__additional_scanline_2`, which was outside that metric.
Temporary-name uniqueness is spelling uniqueness, not distinct local variables.
The raw-offset heuristic deliberately includes scalar arithmetic; it is a review
list, not a count of incorrect pointer accesses.

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
