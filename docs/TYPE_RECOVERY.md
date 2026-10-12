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
