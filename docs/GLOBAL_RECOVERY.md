# Global-name recovery

The starting tree is `b606c304d1725a0905ea1d9fde2e4af3d1162b9d`:
984 distinct address-named globals, with 6,606 code references across 101 files.
Counts exclude comments and literals. Original identifiers and addresses remain
in declaration comments; `GLOBAL_RECOVERY.json` records accepted names and evidence.

## Controller device state and queues

Nine globals now describe enabled and connected port masks, cached device status,
accessory storage and lifecycle state, serial completion messages, and controller
thread commands. Initialization, accessors, connection refresh and device polling
establish their roles. Ghidra initialization at `0x80002998` confirms the queue
capacities, serial event registration, channel mask and accessory-state setup.

The storage representations, declaration order, queue capacities and accesses
are unchanged. Explicit symbol mappings preserve these names when regenerating
the ROM split. The matching baseline is preserved outside the checkout at
`/tmp/pikachu-global-names-baseline/build` and must not be refreshed during recovery.

This is an initial batch, not completion of global recovery. Rumble motor/pattern
records and processed controller samples are the next controller candidates.
Remaining globals require evidence from definitions, initialization, consumers,
allocation and ROM analysis; names must not be invented to reduce the counts.

Accepted verification for this batch: a clean build prints `OK: ROM matches`,
direct ROM comparison succeeds, and all 483 objects match the preserved baseline
including relocation addends. `make verify BASE=main
BASELINE=/tmp/pikachu-global-names-baseline/build` passes all 31 tests, ledger and
publication checks, with no name errors and five existing permitted port warnings.
The remaining inventory is 975 distinct address names and 6,552 references across
101 files; unknown-type, field-macro and temporary-name counts remain zero.

## Rumble, processed input and microphone session

A further 33 globals describe rumble motor and pattern records and their handler
tables, processed controller input and button timing, microphone session policy
and timing, the EEPROM request, and the microphone request ring. Rumble dispatch
at `0x80003078`, input processing at `0x80004BC0`, session updates at `0x80004204`,
EEPROM submission at `0x8000451C`, and microphone submission at `0x80004670` were
checked with Ghidra against the source readers and writers.

The primary controller's pressed, released and repeated words retain distinct
interior symbols. The sample base retains its existing scalar declarations in
consumers that view the primary held-button word. This naming pass does not merge
those symbols, change their declaration types or alter relocation addends.

Microphone dictionary state distinguishes requested, currently uploading and
successfully uploaded dictionaries. Trigger button history, trigger release time,
amplifier request times and the last disabled-amplifier policy poll are distinct.
The EEPROM request is owned by the EEPROM submission interface even though its
original definition shares the microphone session translation unit.

After these groups, 942 distinct address names remain, with 6,316 references
across 100 files. The first 42 names are recorded in `GLOBAL_RECOVERY.json`.
Each group preserves the ROM and all 483 objects against the immutable baseline.

The combined clean build and direct ROM comparison pass. Full `make verify`
passes with 483 matching objects, 31 passing tests, current ledger locations,
publication hygiene, no name errors and the same five permitted port warnings.

## Recognition engine and shared I/O resources

Eighteen recognition globals now name vocabulary data, dictionary selection and
reload policy, word registration progress, device availability, command status,
read requests, the persistent device handle, and retained recognition results.
The upload flag is deliberately named `gVoiceDictionaryBetweenUploads`: the ROM
uses the same nonzero value before dictionary setup and after registration ends.
It is not solely a completion flag. Ghidra upload analysis at `0x800406B4`
confirms both meanings. Existing volatile declarations and the poller's volatile
view of device availability are preserved.

The EEPROM transfer job and cached EEPROM probe result now have descriptive
names. The latter was misleadingly named `__osActiveQueue_2`; Ghidra initialization
at `0x800037AC` establishes that it is game-owned EEPROM state rather than an SDK
thread queue. This additional rename is outside the address-name metric.

The peripheral-interface manager queue and message storage are named from their
initialization at `0x80037AE8`. The shared game heap is named from initialization
at `0x80037F88` and its allocator consumers. These changes retain every existing
storage view, argument representation, allocation size and relocation identity.
SDK interfaces and SDK-owned globals retain their names.

The inventory now has 920 distinct address names, with 5,863 references across
92 files. This is 64 recovered address names plus the corrected EEPROM alias.
Unknown-type, field-macro and temporary-name counts remain zero.

Acceptance for the recognition/I/O/heap batch: clean-ROM gate and direct byte
comparison pass; all 483 objects match the original preserved build. Full
verification passes 31 tests, ledger consistency and publication hygiene, with
no name errors and the five existing permitted port warnings.

## Voice-command translation, action tables and runtime timing

Nineteen globals now name gameplay voice permissions, recognition status and
result storage, voice warning thresholds, display-name fallback, text-image
configurations, scrolling-panel backdrop resources, empty/line-break text, and
four-bit pixel masks. The fallback name points to “Detection impossible” in the
ROM; it is not an empty string. The legacy result-index fallback assigns a
recognition distance to the lookup index. Naming preserves this behavior and
the separate retained recognition result. Ghidra translation at `0x80012508`
confirms the status, thresholds, fallback and message conversion.

The action sequencer's program table and opcode handlers are named from start
and dispatch consumers, corroborated at `0x80011E6C`.

Thirty runtime globals now describe smoothed frame timing, the rolling sample
window, elapsed clocks, play-time accounting and UI pulse time, plus the three
save-prompt tables and their text. Ghidra clock update at `0x8000AF08` and prompt
handling at `0x8000B30C` corroborate these roles. The two earlier prompt tables
are still traversed up to the next table's base. Declaration adjacency and all
individual string symbols are retained, including separate blank lines.

Changed source ranges were formatted with the repository's `.clang-format`.
The remaining inventory is 869 distinct address names, with 5,412 references
across 91 files: 115 of the starting address names have been recovered. The
separately corrected EEPROM alias is additional to that count.

The final clean build and direct ROM comparison pass. Full verification confirms
all 483 objects match the immutable starting build, all 31 tests pass, ledger
locations are current, publication hygiene passes, and no name errors occur.
The five previously permitted port-binding warnings remain unchanged.

## Runtime transitions and scheduler ownership

Thirty-one runtime globals now name the scheduler event queue and client,
graphics command queue, current/requested programs and area entrances, game-mode
entry and timing state, profile-reset policy, save parameters, runtime thread
storage and control block, graphics completion count and rendering warm-up state.
The save prompt's eight words retain their separate symbols while naming the
established TextPanel fields; the acknowledgement button mask is separate.
Program update and initialization entries likewise retain their distinct symbols
and sixteen-byte per-program stride. Ghidra event-loop analysis at `0x8000BB8C`
corroborates the queue, completion count and program transition contracts.

Four scheduler globals now name pre-NMI state, display-event count, audio stack
storage and graphics stack storage. The counter counts every consumed display
message, including pre-NMI, rather than exclusively retraces. Initialization at
`0x8003FC30` corroborates the stack arguments and reset state. The base of audio
stack storage is also the retrace stack top, and the graphics stack base is the
audio stack top. The previously named voice device-port address also marks the
graphics stack top. Declaration comments and the ledger record these dual roles;
no additional storage or aliases were introduced.

The inventory now contains 834 distinct address names and 5,152 references across
91 files: 150 starting address names have been recovered. Field macros, unknown
type tokens and temporary-name tokens remain absent. Each group passes strict
comparison of all 483 objects with the original immutable baseline.

Final acceptance passes the clean-ROM gate and direct byte comparison, all 483
object comparisons, all 31 tests, current recovery ledger, publication hygiene,
and whitespace checks. The name checker reports no errors and the same five
permitted port-binding warnings. Changes remain on the recovery branch.

## Graphics tasks and camera/render resources

Twenty-two globals now name graphics task-pool ownership and cursors, framebuffer
selection, output/stack/yield buffers, microcode descriptor views and identification
strings, completion messages, configured video-mode storage and register aliases,
diagnostic glyphs, and the default text palette. Submission at `0x8003C5EC`
corroborates resource use, framebuffer rotation and distinct completion messages.
Microcode implementations and SDK symbols retain their existing names.

Twenty-nine additional symbols now describe camera storage and defaults, drawing
context and command buffers, per-task viewports and matrices, shared/area assets,
program drawing callbacks, elapsed-script tint policy, overlay transition state,
framebuffer storage and depth-buffer addresses. Rendering initialization at
`0x80000518` confirms the allocations and storage boundaries. Depth clearing at
`0x80000D3C` writes physical address `0x38F800`, while frame submission uses the
color framebuffers at `0x803B5000` and `0x803DA800`. The old z-buffer comment on
the first color framebuffer was corrected; matching diagnostic strings remain
unchanged. The depth-buffer base also bounds the game heap and serves as loading
scratch. These roles are recorded without splitting or merging any storage.

Original counter coverage includes only eight-digit address identifiers. Four
shorter names recovered here identify the shared asset ROM boundaries, physical
depth address and segmented overlay display list. They are additional to the
starting 984-name target, as is the earlier EEPROM alias. There are now 787
counted names and 4,524 references across 90 files: 197 starting names recovered,
plus those five additional symbols. The ledger contains all 202 renames.

Both source-unit groups preserve all 483 objects against the immutable baseline.
Changed ranges were formatted and redundant existing palette/framebuffer comments
were consolidated into their Doxygen declaration comments.

Final acceptance passes the clean-ROM gate, direct byte comparison, all 483
object comparisons, 31 tests, recovery-ledger consistency, publication hygiene
and whitespace checks. The name checker has no errors and the same five
permitted port-binding warnings. Main remains unchanged.

## Motion, texture and actor asset caches

Motion cache state, archive offsets and names now describe their roles; the
archive ROM start is also named. Ghidra at `0x80017228` confirms the next-offset
size calculation and archive-relative base contract. Texture cache state,
descriptor banks/counts, rectangle scale tolerances, archive ROM start and shared
rectangle setup display list are likewise named; image and palette read behavior
is corroborated at `0x8000C704`. Redundant old cache-state comments were consolidated.

Actor asset storage, the DMA queue, compressed staging storage and resource
descriptors now have descriptive names. Kind-table interior views name resource
indices, attachment/default parts, ground-reference parts, alternate/default
names and descriptions, and the creation view beginning at kind one. Their
original separate symbols and eighty-four-byte strides are retained. Contact
reactive effect templates, scale and idle/triggered animation programs are named
from their consumers. Initialization at `0x8001DFD4` confirms cache and queue
ownership; compressed loading still retains its single outstanding staging block.

The part registry now names per-kind slot counts, the packed registration data,
the owned kind/part lookup and its separate counter-reset span. Registration at
`0x80016248` confirms the root slot and packed kind/part keys. The active actor
reference is now `gPikachuActor`; creation at `0x80112CA0` explicitly constructs
kind 0x25 and stores it for the selected area. Existing integer and pointer views
of that reference are unchanged throughout its consumers.

The original inventory is down to 754 names and 4,240 references across 88 files:
230 of its 984 names recovered. The three shorter archive/segmented identifiers
in this batch are additional recoveries. Including those, the earlier four short
identifiers and the EEPROM alias, the ledger now records 238 names. Every group
preserves all 483 objects against the original saved baseline.

The final clean build and direct ROM comparison pass. Full verification passes
all 483 object comparisons, all 31 tests, recovery-ledger consistency,
publication hygiene and whitespace checks, with no name errors and the five
existing permitted port-binding warnings. Main remains unchanged.

## Object hierarchy traversal, doors and response storage

The object hierarchy batch names the current affine transform, pending local
movement, saved transform stack and depth, handle pool and next-handle view,
active hierarchy traversal pool/count, search tree and root view, nearest-target
search context, draw group, default template and per-kind callbacks. Viewport,
camera matrix and callback interior views retain their separate symbols and
original strides. Initialization at Ghidra `0x80009808` confirms ownership and
reset behavior; push/pop, hierarchy registration and nearest-target consumers
establish the individual roles. The shared segmented drawing setup list is an
additional recovery outside the eight-digit-name inventory.

The door registry now names its handles/count, destination/access text buffers
and exit descriptions. Response storage names distinguish the owned heap block,
heap header and keyed registry. Dispatch names distinguish the response-kind
handler table from the per-object-kind response callback view. Ghidra at
`0x8001C460` confirms response heap allocation and keyed-pool initialization;
record creation, attachment and cleanup establish ownership and indexing.
Redundant layout/table comments were consolidated into declaration Doxygen.

There are now 726 counted names and 4,036 references across 87 files: 258 of the
starting 984 names recovered. The ledger records 267 names including nine
additional short-address or alias names. Raw-offset candidates remain unchanged.

Each source-unit batch preserves all 483 objects against the immutable baseline.
The final clean build prints `OK: ROM matches`; direct ROM byte comparison,
31 tests, strict object comparison, ledger consistency, publication hygiene and
whitespace checks pass. The name checker has no errors and the same five
permitted port warnings. Main remains unchanged.

## Shared area configuration and terrain lookup state

Area overlay descriptors and configuration pointers now name their area-indexed
roles, along with full display names and short region names. Ghidra at
`0x8010E60C` confirms that destination activation installs the configuration
pointer for the selected area. Cross-unit declarations retain their original
representations; the borrowed configuration remains valid only while its scene
overlay is loaded. Overlay loading and destruction behavior remain unchanged.

The maze root, child-axis placement tables and midpoint-to-octant lookup now
have descriptive names. Flat and mound height samples and grid descriptors are
named from their data and terrain interpolation consumers. Material friction
and contact sound tables name their documented indexing contracts. Position
event state distinguishes the last node, selector and elapsed-time checkpoint;
near and far visibility state names the squared thresholds established by
configuration refresh. Existing redundant table comments were consolidated.

This batch recovers 21 starting names. There are 705 remaining counted names,
3,642 references and 67 affected files: 279 of the original 984 names recovered.
The full rename ledger now records 288 symbols, including nine additional names
outside that initial inventory. Raw-offset candidates remain unchanged.

Incremental and final clean builds preserve all 483 baseline objects and the
retail ROM bytes. Full verification passes 31 tests, ledger consistency,
publication hygiene and whitespace checks; there are no name-checker errors
and the same five permitted port-binding warnings. Main remains unchanged.

## Primary player views and interaction state

The primary player record now has a descriptive name. Its separate position,
view-angle, eye-height, permission, controller-port, held-item and mode-state
symbols are named from the recovered `PlayerControlPrefix` offsets and their
consumers. Existing integer, scalar and record views and the 256-byte player
stride remain unchanged. Ghidra at `0x8010E980` corroborates the current/saved
permission writes. No interior references were folded into record-plus-offset
relocations.

The saved player snapshot, collision query mode, help-text buffer/cursor,
interaction prompts and action callbacks, mode/control-scheme tables and footstep
thresholds now describe their roles. The buffer name identifies its first word;
adjacent storage symbols remain intact. Music-ducking state distinguishes the
active reduced-volume state from flags inhibiting its transitions. Original
identifiers and addresses are retained in declaration Doxygen, with redundant
existing comments consolidated.

This batch recovers 24 starting names. There are 681 names and 3,334 references
across 65 files remaining: 303 of the original 984 names recovered. The ledger
records 312 symbols including nine additional recoveries outside the original
inventory. Raw-offset candidates remain unchanged.

Incremental and final clean builds preserve all 483 baseline objects. The clean
ROM gate and direct byte comparison pass, as do all 31 tests, ledger consistency,
publication hygiene and whitespace checks. The name checker reports no errors
and the same five permitted port warnings. Main remains unchanged.

## Item catalog, registry and nearest-search state

The item search tree and live count now describe their registry roles; Ghidra
at `0x80113C78` confirms initialization and comparison callback selection.
Shared nearest-query state and its separate view-transform symbol are named
from query initialization and candidate traversal. Model hierarchy templates,
the item-kind catalog and inclusive category ranges now have descriptive names.

Catalog interior views name replacement kinds, cause-three responses,
classification bytes, friction coefficients, both tracked impact sounds, terrain
collision callbacks and held-item callbacks. Their offsets agree with the
recovered `ItemKind` layout and their consuming functions. All preserve their
separate original symbols and 116-byte record strides. Original identifiers and
addresses are retained in declaration Doxygen. Unresolved catalog fields retain
their explicit existing names rather than receiving speculative meanings.

This batch recovers 15 starting names. There are 666 remaining names and 2,996
references across 63 files: 318 of the starting 984 names recovered. The ledger
contains 327 recoveries including nine additional symbols outside that original
inventory. Raw-offset candidates remain unchanged.

All 483 objects match the immutable baseline before and after the final clean
build. The clean-ROM gate and direct byte comparison pass. Full verification
passes all 31 tests, ledger consistency, publication hygiene and whitespace
checks, with no name errors and the same five permitted port warnings.
Main remains unchanged.

## Collision candidate registry and shared text window

Collision storage now distinguishes the owned keyed registry from the X and Z
spatial-hash alignment offsets. Registration, overlap queries, randomization and
cleanup establish their roles; Ghidra at `0x8002CFA8` confirms the 499-entry
registry initialization and alignment reset. The matching object comparison
passed before starting the next defining source unit.

The text-window batch names page-image buffers, the prompt image, accumulated
message text, presentation settings, continuation address, page-history ring
and its write/oldest boundaries, selected image buffer, transition state and
elapsed-time checkpoint. Indicator animation identifiers, initialized tracks
and the separate indicator-indexed track base retain their original layout;
the initialized six tracks begin at index one in the drawing view. Ghidra at
`0x8002B6B0` confirms initialization, with page preparation, drawing, history
operations and transition updates supplying the consumer evidence.

This batch recovers 19 starting names. There are 647 names and 2,858 references
across 62 files remaining: 337 of the original 984 names recovered. The ledger
records 346 symbols including nine additional recoveries outside the original
inventory. Raw-offset candidates remain unchanged.

Both incremental batches and the final clean build preserve all 483 objects.
The clean-ROM gate and direct byte comparison pass. Full verification passes
31 tests, ledger consistency, publication hygiene and whitespace checks, with
no name errors and the same five permitted port warnings. Main is unchanged.

## Message-bar ownership and packed progress storage

The message-bar owner now names its queue, timers, scrolling and retained image
state. Separate interior views name the next work-image index, retained image
handles and current message text used to coalesce repeated requests. Ghidra at
`0x800278F4` confirms the state reset and initial work index; preparation,
enqueueing, drawing and cleanup provide the ownership and consumer evidence.
The complete object comparison passed before the next source-unit batch.

Progress storage now names the packed save record, EEPROM payload sizes and
block offsets, field-type widths/limits, field-type selectors, computed storage
layout, area-relative selectors and independently transferred save-control byte.
Ghidra at `0x8003267C` confirms type-based byte alignment and bit placement.
Read/write and EEPROM request consumers establish the remaining roles. Original
identifiers and addresses are retained in Doxygen, with layout and matching
representations preserved.

This batch recovers twelve starting names. There are 635 names and 2,673
references across 62 files remaining: 349 of the original 984 names recovered.
The ledger records 358 symbols including nine additional recoveries outside the
original inventory. Raw-offset candidates remain unchanged.

Both source-unit batches and the final clean build preserve all 483 objects.
The clean-ROM gate and direct byte comparison pass. Full verification passes
31 tests, ledger consistency, publication hygiene and whitespace checks, with
no name errors and the same five permitted port warnings. Main is unchanged.

## Toolbox display, item rows and transitions

The toolbox owner and its separate field views now name the item rows, selected
row, bonus/purchase slots and handles, placeholder handles, linked display part,
rotation step/angle and transition clock. Display model and placeholder templates,
row anchors, default stored items, description prompts, help messages, row titles,
navigation sprites and backdrop color are also named. Help timing, requested row,
exit action, initial draw count, interface transform and owned row-title images
now describe their roles.

Ghidra at `0x8002FD60` corroborates the owner reset, initial Item Storage row,
special-slot clearing and initial supply kind. Opening, storage/retrieval,
rotation, drawing and transition consumers establish the remaining meanings.
Separate symbols and original access representations are retained, including
linker aliases. Doxygen preserves original identifiers/addresses and redundant
old table comments were consolidated.

This batch recovers 32 starting names. There are 603 names and 2,430 references
across 62 files remaining: 381 of the original 984 names recovered. The ledger
records 390 symbols including nine additional recoveries outside the original
inventory. Raw-offset candidates remain unchanged.

The incremental and final clean builds preserve all 483 objects. The clean-ROM
gate and direct byte comparison pass. Full verification passes 31 tests, ledger
consistency, publication hygiene and whitespace checks, with no name errors
and the same five permitted port warnings. Main remains unchanged.

## System menu tree, text and audio snapshot

Persistent menu entries and lists now name their settings, sound/volume,
vertical-look, save/load, home confirmation, status and day-display roles.
Debug area, eye-catch, chapter, difficulty, checkpoint, disappear/item flags,
level-one counts and clear-flag entries/lists and their label tables are named
from their initializers and callbacks. These definitions retain their original
list links, callback representations and mutable selection fields.

The menu navigation owner, saved audio cue/rate and preview handle, interface
transform, double-buffered row/detail images, detail-text accumulator and length,
base model position, backdrop color and EEPROM status labels now have descriptive
names. Ghidra at `0x8011BAC8` confirms menu state/image initialization and model
ownership; audio entry/restore, text rebuild, drawing and callback consumers
establish the individual contracts. Separate item-row and state aliases retain
their original symbols and offsets. Original identifiers/addresses are preserved
in declaration Doxygen; redundant old buffer comments were consolidated.

This batch recovers 68 starting names. There are 535 names and 2,193 references
across 62 files remaining: 449 of the original 984 names recovered. The ledger
records 458 symbols including nine additional recoveries outside the original
inventory. Raw-offset candidates remain unchanged.

Incremental and final clean builds preserve all 483 objects. The clean-ROM gate
and direct byte comparison pass. Full verification passes 31 tests, ledger
consistency, publication hygiene and whitespace checks, with no name errors
and the same five permitted port warnings. Main remains unchanged.

## Backdrop descriptor views and HUD guide selection

Backdrop globals now distinguish the descriptor catalog, width/height/display
mode/scroll-rate interior views, active identifier, camera backdrop-image view,
primary image width and accumulated scroll. Camera eye/target coordinate views
and the direction tolerance and pitch/yaw conversion constants are named from
view-offset calculation. Ghidra at `0x80033A70` confirms descriptor selection,
view dimensions, scroll reset and source-loading dispatch. Fixed/scrolling
drawing and descriptor accessors establish the other contracts. Separate symbols
and record strides remain intact.

HUD guide state now names the selected style, active callback, callback table
and four sprite descriptor tables. Motion-indicator animation state and the HUD
feature mask are named from their selection, evaluation and drawing consumers.
Ghidra at `0x80021CDC` confirms style selection and callback lookup. Original
identifiers and addresses remain in declaration Doxygen.

This batch recovers 25 starting names. There are 510 names and 2,104 references
across 61 files remaining: 474 of the original 984 names recovered. The ledger
records 483 symbols including nine additional recoveries outside the original
inventory. Raw-offset candidates remain unchanged.

Each source-unit batch and the final clean build preserve all 483 objects.
The clean-ROM gate and direct byte comparison pass. Full verification passes
31 tests, ledger consistency, publication hygiene and whitespace checks, with
no name errors and the same five permitted port warnings. Main is unchanged.

## HUD recognition, guidance and display state

HUD state now distinguishes microphone presentation/animation/color selection
from recognition presentation, animation and retained scale. The microphone
track's retained-frame interior symbol is named from its transition-feedback
consumer and agrees with the recovered `ValueTrack` value offset. Ghidra at
`0x80022110` corroborates microphone-state selection and animation transitions.

Horizontal and vertical off-screen Pikachu guidance tracks are named with their
separate middle and inner track views. Movement animation bands, horizontal
remaining time, vertical/horizontal motion tracks, action-selection tables and
sprite descriptors now describe their consumers. Alternate overlay sprites,
auxiliary icon sprites, area-label image and visibility flags are named from
initialization and drawing. Original identities remain in Doxygen; separate
symbols, representations and layout boundaries remain unchanged.

This batch recovers 30 starting names. There are 480 names and 1,993 references
across 61 files remaining: 504 of the original 984 names recovered. The ledger
records 513 symbols including nine additional recoveries outside the original
inventory. Raw-offset candidates remain unchanged.

Incremental and final clean builds preserve all 483 objects. The clean-ROM gate
and direct byte comparison pass. Full verification passes 31 tests, ledger
consistency, publication hygiene and whitespace checks, with no name errors
and the same five permitted port warnings. Main remains unchanged.

## Startup and title-screen presentation

Title globals now name logo, startup-panel and silhouette asset descriptors and
loaded images, menu labels and cursor frames, prepared text images, logo fade,
Start-prompt sprites/phase and introduction timing. Current state/choice,
load-game selection, saved main/file choices, enabled file choices, menu request
and demonstration/backdrop selection describe the state machine's input and
transition contracts. Startup controller/microphone and save-load error messages,
overwrite text, file-menu index lists and the owned message panel are also named.

Ghidra at `0x800341C0` confirms artwork loading, menu text-image preparation and
initial selection reset. The startup checks, file-selection/overwrite handlers,
message setup and drawing consumers establish the remaining meanings, including
minimum artwork display checkpoints. Separate symbols and representations stay
unchanged, with original identifiers/addresses retained in declaration Doxygen.

This batch recovers 35 starting names. There are 445 names and 1,797 references
across 61 files remaining: 539 of the original 984 names recovered. The ledger
records 548 symbols including nine additional recoveries outside the original
inventory. Raw-offset candidates remain unchanged.

Incremental and final clean builds preserve all 483 objects. The clean-ROM gate
and direct byte comparison pass. Full verification passes 31 tests, ledger
consistency, publication hygiene and whitespace checks, with no name errors
and the same five permitted port warnings. Main remains unchanged.

## Game audio initialization and playback ownership

Audio globals now name the sequence slot array/end boundary, custom reverb
preset, audio heap storage, initialized flag, game-owned synthesizer instance,
loaded bank file, allocated sound-effect player, output mode, sequence driver
and voice storage. The shared next-playback-handle counter is named consistently
for sequence and sound-effect allocation. Ghidra at `0x8001F680` corroborates
resource initialization and ownership; shutdown and playback consumers establish
the lifetimes and shared counter contract. SDK and microcode symbols are retained.

The audio bank and sample ROM starts are also named; their shorter original
identifiers are additional to the initial eight-digit inventory. Separate
symbols and existing representations are preserved, with original identities
and addresses retained in Doxygen.

This batch recovers twelve starting names and two additional ROM boundaries.
There are 433 names and 1,726 references across 60 files remaining: 551 of the
original 984 names recovered. The ledger records 562 symbols including eleven
additional recoveries outside the original inventory. Raw offsets are unchanged.

Incremental and final clean builds preserve all 483 objects. The clean-ROM gate
and direct byte comparison pass. Full verification passes 31 tests, ledger
consistency, publication hygiene and whitespace checks, with no name errors
and the same five permitted port warnings. Main remains unchanged.

## Audio sample DMA cache and rendering worker

Sample-cache globals now distinguish pending reads, cache age/capacity, page
size/shift, page-to-slot lookup, entries, read-ahead bytes, I/O records and DMA
completion queue. Initialization, eviction, sample resolution and frame-end
completion establish their roles and ownership.

Worker globals now name shared stack/thread/render storage, the thread object,
AI-event queue, scheduler task-completion queue, shutdown-acknowledgement queue,
their single message slots and the stop-request flag. Ghidra at `0x800496B8`
confirms allocation and queue/thread setup; worker rendering and stop consumers
establish the synchronization contracts. Original symbols and access
representations remain intact, with identifier/address notes in Doxygen.

This batch recovers 19 starting names. There are 414 names and 1,630 references
across 60 files remaining: 570 of the original 984 names recovered. The ledger
records 581 symbols including eleven additional recoveries outside the original
inventory. Raw-offset candidates remain unchanged.

Incremental and final clean builds preserve all 483 objects. The clean-ROM gate
and direct byte comparison pass. Full verification passes 31 tests, ledger
consistency, publication hygiene and whitespace checks, with no name errors
and the same five permitted port warnings. Main remains unchanged.

## Collection presentation positions, stages and text ownership

The collection presentation now names its camera-relative item/actor positions,
card and collection stages, accumulated score, displayed item, owned title/score
images, alternating subtitle images/index and card dismissal deadline.
Initialization, drawing, score award and text replacement consumers establish
these roles. Ghidra at `0x80028FFC`, `0x80028B84` and `0x80029CD4` corroborates
subtitle replacement, card progression and results progression respectively.
Original representations and symbol addresses remain unchanged, with original
identifiers and addresses retained in declaration Doxygen.

This batch recovers 13 starting names: 583 of the original 984 recovered, with
401 names and 1,541 references across 60 files remaining. The symbol ledger has
594 entries including eleven additional recoveries outside the starting count.
Raw-offset candidates remain unchanged.

The final clean build prints `OK: ROM matches`, and direct ROM comparison passes.
All 483 objects match the immutable baseline, including strict relocation checks.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and naming checks, with no name errors and the same five permitted
port warnings. Progress reporting was regenerated. Main remains unchanged.

## Area scenario and presentation resource ownership

Presentation globals now name focus targets, decoration/panel templates,
chase-stage texture selections for areas 5/11/13, the previous response variant,
results wait checkpoint, card suppression, card audio/action choices, the
presentation-to-world transform, region-name image and saved actor/behavior
records. The area scenario index is shared by cards, titles, actor script
selection and scene population; its name and contract reflect all consumers.

Ghidra at `0x80028518`, `0x800285B8`, `0x8002966C` and `0x8002AA08` corroborates
actor snapshots, coordinate conversion, response selection and area initialization.
Drawing and card-update consumers establish template and chase-stage selections.
Original identifiers/addresses remain in Doxygen; symbol representations and
layout are unchanged. Old block comments on the focus targets and card panels
were consolidated into the declaration documentation.

This batch recovers 17 starting names: 600 of 984 recovered, with 384 names and
1,412 references across 58 files remaining. The ledger contains 611 symbols,
including eleven additional recoveries outside the starting inventory. Raw-offset
candidates are unchanged.

Final clean build and direct ROM byte comparison pass. All 483 objects match the
immutable baseline with strict relocations. Full verification passes 31 tests,
ledger consistency, publication hygiene and whitespace; the name checker has no
errors and the same five permitted port warnings. Progress reporting was
regenerated. Main remains unchanged.

## Eye-catch interlude resources and playback

Eye-catch globals now identify the selected/active group, group descriptors,
per-layer animation tracks, stage and elapsed seconds, foreground asset descriptor
and loaded image, foreground visibility/opacity, audio stage and randomized
entry/exit sound choices, backdrop corner coordinates/vertices, projection and
model-view matrices, and stage callbacks. Initialization, resource release,
drawing, weighted selection and timed playback establish ownership and roles.
Ghidra at `0x80036AF4` and `0x80037610` corroborates resource setup and audio/fade
playback. Original identifiers and addresses remain in Doxygen. SDK names,
representations, allocations and control flow remain unchanged.

This batch recovers 18 starting names: 618 of 984 recovered, with 366 names and
1,295 references across 57 files remaining. The ledger contains 629 symbols,
including eleven additional recoveries outside the original inventory.
Raw-offset candidates remain unchanged.

The final clean build prints `OK: ROM matches`; direct byte comparison passes.
All 483 objects match the immutable baseline with strict relocation addends.
Full verification passes 31 tests, ledger consistency, publication hygiene and
whitespace, with no name errors and the same five permitted port warnings.
Progress reporting was regenerated. Main remains unchanged.

## Archive Huffman decoder workspace

The archive decoder now names its busy flag, auxiliary and literal/match-length
Huffman tables, extended decode-table storage and allocation cursor, current
symbol code lengths, and code-space weight lookup. Table preparation and symbol
consumers establish these roles, including reuse of the auxiliary table for
code-length symbols and match-distance widths. Ghidra at `0x80049F74` and
`0x8004A8A4` corroborates decoding and extended table construction. Shared storage,
serialized-use contract and original access representations are preserved.
Original identifiers/addresses remain in declaration Doxygen.

Seven starting names are recovered: 625 of 984 recovered, with 359 names and
1,259 references across 56 files remaining. The symbol ledger contains 636 entries,
including eleven additional recoveries outside the original inventory. Raw-offset
candidates remain unchanged.

The clean build prints `OK: ROM matches`; direct ROM byte comparison passes.
All 483 objects match the immutable baseline with strict relocation addends.
Full verification passes 31 tests, ledger consistency, publication hygiene and
whitespace, with no name errors and the same five permitted port warnings.
Progress reporting was regenerated. Main remains unchanged.

## World-map destination selection and shared focus interpolation

World-map globals now identify destination records, current/previous destination,
menu state/checkpoint, directional hold duration, selection highlight opacity/step,
icon/badge/highlight templates, locked-marker angle, difficulty overlays, actor
placement/transform, voice destination mapping and special/other-message focus
points. Shared presentation focus state now distinguishes the start, target,
current point and transition checkpoint. Consumers establish these roles without
changing layout, access representation or state transitions.

Ghidra at `0x8002E200`, `0x8002D474`, `0x8002D348`, `0x8002E6C4`, `0x8002DD70`,
`0x8002D904` and `0x8002DBE0` corroborates input, destination rendering, focus
interpolation, menu progression, actor placement, locked markers and overlays.
Original identifiers and addresses remain in declaration Doxygen. The unused
`D_8009B584` declaration remains pending its owning subsystem's investigation.

This batch recovers 22 starting names: 647 of 984 recovered, with 337 names and
1,127 references across 56 files remaining. The ledger has 658 symbols, including
eleven additional recoveries outside the original inventory. Raw-offset
candidates remain unchanged.

Final clean build and direct ROM byte comparison pass. All 483 objects match the
immutable baseline with strict relocation addends. Full verification passes all
31 tests, ledger consistency, publication hygiene and whitespace, with no name
errors and the same five permitted port warnings. Progress reporting was
regenerated. Main remains unchanged.

## Controller timing constants, diagnostics and rumble requests

Controller globals now name the separate seconds/microseconds conversion
constants used by initial repeat, repeat interval and hold reporting, the
processed-input and invalid-port diagnostic formats, and the eight asynchronous
rumble request slots and current slot index. ROM values confirm each timing
constant is 1,000,000; conversion consumers and Ghidra at `0x80004F64` establish
the units. Ghidra at `0x80004970` confirms rumble request preparation and queue
submission. Original identities/addresses remain in Doxygen.

Naming the input trace at the beginning of its binary rodata section initially
changed Splat's generated label size from one to 96 bytes. The object comparator
correctly rejected that representation. Keeping the original section label and
providing the trace name through `rodata_aliases.ld` preserves every defined-symbol
attribute without changing comparator rules or refreshing the baseline.
Unreferenced controller BSS words remain explicit pending further evidence.

Seven starting names are recovered: 654 of 984 recovered, with 330 names and
1,110 references across 55 files remaining. The ledger has 665 symbols including
eleven additional recoveries outside the starting count. Raw-offset candidates
remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocations and symbol attributes. Full verification
passes 31 tests, ledger consistency, publication hygiene and whitespace, with no
name errors and the same five permitted port warnings. Progress reporting was
regenerated. Main remains unchanged.

## Interaction candidate scan and marker configuration

Interaction globals now name the model template, two view-space regions and
classification bits, current candidate score/handle/flags, evaluated marker mode,
per-mode animation/sound/placement styles, marker animation unit scales and
primary/secondary region scale/margin units. Interior margin symbols retain their
separate representations. ROM data confirms region bits 2/1 and six 0.1 unit
constants. Ghidra at `0x8001E830` and `0x8001EAF4` corroborates candidate ranking,
marker target retention, animation selection and placement. Original identifiers
and addresses remain in Doxygen; shared animation program tables remain for their
broader ownership investigation.

Sixteen starting names are recovered: 670 of 984 recovered, with 314 names and
1,048 references across 55 files remaining. The ledger contains 681 symbols,
including eleven additional recoveries outside the original inventory. Raw-offset
candidates remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocation addends. Full verification passes 31
tests, ledger consistency, publication hygiene and whitespace, with no name errors
and the same five permitted port warnings. Progress reporting was regenerated.
Main remains unchanged.

## Math lookup tables and random-selection state

Math globals now name the retained four-by-four identity matrix, affine identity
matrix, four-word pseudorandom generator state, inverse-sine samples and quarter-turn
sine samples. The unused identity is identified from its initializer; no reader is
claimed. The separate integer matrix mask remains unresolved. ROM samples and
lookup consumers establish the angle-table meanings. Ghidra at `0x8003AA88`,
`0x8003A058`, `0x8003B318` and `0x8003C3DC` corroborates seeding, identity copying
and angle lookup.

After verifying the math unit, the vector helper unit now names the history-aware
random range and retained choice ordering. Ghidra at `0x8000DE58` confirms their
initialization and selection use. Original representations, table contents and
original identifier/address Doxygen are preserved.

Seven starting names are recovered: 677 of 984 recovered, with 307 names and
1,003 references across 54 files remaining. The ledger has 688 symbols, including
eleven additional recoveries outside the original inventory. Raw-offset candidates
remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocations. Full verification passes 31 tests,
ledger consistency, publication hygiene and whitespace, with no name errors and
the same five permitted port warnings. Progress reporting was regenerated.
Main remains unchanged.

## Actor movement timing, rates and deformation presets

Actor movement constants now name ground-spin and lift timing bounds, spin and
height-approach yaw rates, special approach speed, tumbling forward speed,
airborne vertical step, single-pulse amplitude and damped-pulse amplitudes/phase
threshold. ROM constants and each consuming behavior establish the units and
scope; equal-valued constants remain distinct symbols. Ghidra at `0x8000F7BC`,
`0x8000F934`, `0x800106E4`, `0x80010854`, `0x80010940`, `0x80011470` and
`0x80011528` corroborates these consumers. Original identifiers and addresses
remain in Doxygen; expressions, timing, layout and control flow are unchanged.

Thirteen starting names are recovered: 690 of 984 recovered, with 294 names and
977 references across 53 files remaining. The ledger contains 701 symbols,
including eleven additional recoveries outside the original inventory. Raw-offset
candidates remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocation addends. Full verification passes 31
tests, ledger consistency, publication hygiene and whitespace, with no name errors
and the same five permitted port warnings. Progress reporting was regenerated.
Main remains unchanged.

## Actor configuration consumers and creation diagnostics

Remaining actor-configuration globals now name contact reaction chain pull/scale/
damping, billboard texture animation, child-motion yaw/roll phase steps, the
Pikachu kind configuration, actor-creation heap labels and ground-shadow segment
radius scale. The Pikachu configuration view is kind 0x25: its address is 36
0x54-byte records after the configuration view beginning at kind one. Original
interior symbols remain separate. ROM diagnostic text identifies the eight
creation checkpoints, including resource loading, hierarchy/matrix allocation,
part-table preparation and staging release.

Ghidra at `0x8001D7D8`, `0x8001DCFC`, `0x8001DE24` and `0x8001E1C4` corroborates
contact reactions, billboard initialization, sway and actor creation. Original
identifiers/addresses remain in Doxygen; expressions, calls, tables and SDK names
are unchanged.

Sixteen starting names are recovered: 706 of 984 recovered, with 278 names and
943 references across 52 files remaining. The ledger contains 717 symbols,
including eleven additional recoveries outside the original inventory. Raw-offset
candidates remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocation addends. Full verification passes 31
tests, ledger consistency, publication hygiene and whitespace, with no name errors
and the same five permitted port warnings. Progress reporting was regenerated.
Main remains unchanged.

## Flipbook palette and messenger templates, animation and tracking

Flipbook/messenger globals now name the shared default text palette, scrolling
board and messenger hierarchy templates, flight and dissipation animation
programs, the tracked messenger handle and distinct direct/homing display lists.
The tracked handle replaces the misleading `gzip_data_0000` label, preserving its
original identifier and address alongside the other declaration Doxygen notes.
The short segmented display-list identifiers and tracked-handle alias are
additional recoveries outside the eight-digit starting inventory.

Ghidra at `0x80026AB8`, `0x80026024`, `0x800268D0`, `0x80025BE0` and `0x800256C8`
corroborates launch/replacement, initial animation, dissipation, board creation and
palette use. Shared palette consumers and the VM handle query are renamed
consistently. Access representations, lifetimes and gameplay remain unchanged.

Five starting names plus three additional identities are recovered: 711 of 984
recovered, with 273 names and 900 references across 48 files remaining. The ledger
contains 725 symbols including fourteen additional recoveries outside the starting
count. Raw-offset candidates remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocation addends. Full verification passes 31
tests, ledger consistency, publication hygiene and whitespace, with no name errors
and the same five permitted port warnings. Progress reporting was regenerated.
Main remains unchanged.

## Motion playback and action pose-transition tables

Motion playback globals now identify action definitions, their separate fallback
identifier view, motion durations, archive-relative track-table offsets, explicit
pose-transition records, the owned transition lookup map and default action
command sequence. Request, track initialization, transition construction and
resolution establish the roles. Ghidra at `0x8000E08C`, `0x8000E208`, `0x8000F00C`
and `0x8000ECB4` corroborates these paths. Original identifiers/addresses remain
in Doxygen; the fallback view retains its separate symbol and 0x2C stride.
Playback behavior, table contents and expression structure are unchanged.

Seven starting names are recovered: 718 of 984 recovered, with 266 names and
868 references across 47 files remaining. The ledger has 732 symbols, including
fourteen additional recoveries outside the original inventory. Raw-offset
candidates remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocation addends. Full verification passes 31
tests, ledger consistency, publication hygiene and whitespace, with no name errors
and the same five permitted port warnings. Progress reporting was regenerated.
Main remains unchanged.

## Linked-effect motion thresholds and spawn template

Linked-effect globals now name randomized rebound/falling release thresholds,
rebound target-motion timing, horizontal turn coefficients, launch-angle spacing,
and the mutable spawn template with its separate bounds and model/kind/flags
portions. The segmented model display list is an additional recovery outside the
original eight-digit inventory. Ghidra at `0x8001CF3C`, `0x8001D074`, `0x8001D194`
and `0x8001D3B8` corroborates motion selection, velocity initialization and chain
creation. Original identifiers/addresses remain in Doxygen. Existing block notes
on the launch-angle literal and template were consolidated into those contracts.
The shared animation tables remain pending their broader ownership analysis.

Eleven starting names plus one additional model identifier are recovered: 729 of
984 recovered, with 255 names and 842 references across 47 files remaining. The
ledger contains 744 symbols including fifteen additional recoveries outside the
original count. Raw-offset candidates remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocation addends. Full verification passes 31
tests, ledger consistency, publication hygiene and whitespace, with no name errors
and the same five permitted port warnings. Progress reporting was regenerated.
Main remains unchanged.

## Attached effects and shared integer appearance programs

Attached-effect globals now name the per-kind definition table, its default frame
program view, minimum camera-dependent scale and scale coefficient. Shared integer
program tables now distinguish texture-frame selection from opacity animation.
Attached-effect drawing/alpha updates, marker frame/alpha consumers, linked effects,
particle alpha/frame tracks, actor textures and interface users support the shared
roles. All consumers keep their original scalar/array declarations and access
representations. Ghidra at `0x80013130` and `0x800132E0` corroborates creation,
program selection, view sizing and opacity progression. Original identifiers and
addresses remain in Doxygen.

Six starting names are recovered: 735 of 984 recovered, with 249 names and
797 references across 44 files remaining. The ledger has 750 symbols, including
fifteen additional recoveries outside the starting inventory. Raw-offset
candidates remain unchanged.

Final clean build and direct ROM comparison pass. All 483 objects match the
immutable baseline with strict relocation addends. Full verification passes 31
tests, ledger consistency, publication hygiene and whitespace, with no name errors
and the same five permitted port warnings. Progress reporting was regenerated.
Main remains unchanged.

## Ambient-audio emitters and terrain-conforming shadows

Ambient-audio globals now identify the emitter creation template and the gain
applied to combined spatial volume. Ground-shadow globals identify the object
creation template and its 64-by-64 four-bit intensity texture. Ghidra at
`0x80033D20`, `0x80034094`, `0x800369D4` and `0x8003674C` corroborates spatial
mixing, template-based creation and the texture command address. Original
identifiers and addresses remain in Doxygen; types and expressions are preserved.
The segmented texture is an additional recovery outside the original inventory.

Three starting names are recovered: 738 of 984 recovered, with 246 names and
791 references across 42 files remaining. The ledger has 754 symbols, including
sixteen additional recoveries outside the starting inventory. Field macros,
unknown-type tokens and temporary names remain at zero. Raw-offset candidates
remain unchanged.

Each source unit passes strict comparison of all 483 objects. Final acceptance
uses a clean build ending in `OK: ROM matches`, direct ROM byte comparison and
full verification against the immutable global-name baseline. All 31 tests pass,
the recovery ledger matches, publication hygiene passes, and the name checker
reports no errors with the same five permitted port-binding warnings. Progress
reporting was regenerated. Main remains unchanged.

## Landmark, cave, orbit-follower and scene-region records

Recovered globals identify creation templates for landmarks, cave interactions,
orbiting followers and rectangular scene regions; shared cannon/cave description
storage; the cave description format; and the scene-region highlight display list.
Ghidra at `0x8001F110`, `0x8001F194`, `0x8001F4F0`, `0x8001F614`, `0x8002728C`,
`0x8001F1F0` and `0x8001F47C` corroborates the creation, text formatting and draw
paths. Existing declarations, scalar/array representations and expressions are
preserved. Original identifiers and addresses remain in Doxygen. The segmented
region display list is an additional recovery outside the original inventory.

Seven starting names are recovered: 745 of 984 recovered, with 239 names and
775 references across 38 files remaining. The ledger contains 762 symbols,
including seventeen additional recoveries outside the starting inventory.
Field macros, unknown-type tokens and temporary names remain at zero; raw-offset
candidates remain unchanged.

Each source-unit change passes strict comparison of all 483 objects. Final clean
build prints `OK: ROM matches`, and direct ROM byte comparison succeeds. Full
verification passes all 31 tests, ledger consistency, publication hygiene and
whitespace checks; the name checker has no errors and retains the five permitted
port-binding warnings. Progress reporting was regenerated. The immutable baseline
and compiler settings are unchanged. Main remains unchanged.

## Fault reporting state and register diagnostics

All 48 remaining fault-handler globals now describe their roles: saved cause,
status and floating-point control/status flag descriptions and labels; individual
single/double register labels; thread-summary heading and value formats; pending
diagnostic text/value; and fault-thread/event-queue storage. ROM strings establish
the label and numeric-format contents, while saved-context consumers distinguish
register identity and precision. Ghidra at `0x8004237C`, `0x800426B4` and
`0x800427C8` corroborates the report, notification loop and thread startup. The
queue address also serves as the stack-end argument; its existing representation
is preserved. No pending diagnostic producer or more specific meaning for its
integer value is asserted. Original identifiers and addresses remain in Doxygen.

793 of the 984 starting names are recovered, with 191 names and 673 references
across 37 files remaining. The ledger contains 810 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

Strict comparison passes for all 483 objects before and after the clean build.
The clean build prints `OK: ROM matches`, and direct ROM byte comparison passes.
Full verification passes 31 tests, ledger consistency, publication hygiene and
whitespace checks; there are no name-checker errors and the same five permitted
port-binding warnings. Progress reporting was regenerated. The immutable baseline,
compiler settings and main are unchanged.

## Text rendering state and glyph cache

Twenty-four game-owned text globals now identify default and active inclusive
text-area bounds, screen dimensions, the display-list cursor, RGBA storage and
component aliases, pixel cursor coordinates, font-bank selection, spacing mode,
horizontal/vertical scales, borrowed palette, glyph ROM cache and the column-range
diagnostic format. Initialization, reset, setters, glyph drawing and image loading
establish the roles. Ghidra at `0x8003CD30`, `0x8003CF78`, `0x8003CE24`,
`0x8003D3E4` and `0x8003ECD8` corroborates initialization, reset, command cursor,
pixel cursor and measurement paths. The disabled trace format remains retained;
its existing trailing block note is consolidated into Doxygen. Color components
retain the original overlapping array and scalar-symbol representations, with
individual declaration contracts. SDK microcode names remain unchanged.

817 of the 984 starting names are recovered, with 167 names and 501 references
across 37 files remaining. The ledger contains 834 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

All 483 objects pass strict comparison before and after the clean build. The clean
build prints `OK: ROM matches`, and direct ROM byte comparison succeeds. Full
verification passes all 31 tests, ledger consistency, publication hygiene and
whitespace; the name checker reports no errors and retains the five permitted
port-binding warnings. Progress reporting was regenerated. The immutable baseline,
compiler settings and main remain unchanged.

## Actor appearance models, texture channels and tracked poses

Fifteen actor-model globals now identify appearance-slot model tables and part
identifiers; linked-rotation definitions; default integer texture programs and
texture resource segments; signed part sequences for relative roll, paired
orientation blending and primary/secondary tracked poses; randomized shake duration;
and action-meter display thresholds. Creation, setters, orientation consumers,
texture command generation and shake countdown establish these roles. Sequence
names describe verified consumers without asserting anatomical identities.
Ghidra corroborates the paths at `0x800137C0`, `0x80013848`, `0x80013CDC`,
`0x80013D7C`, `0x80013EDC`, `0x800144D4`, `0x80014590`, `0x80014760`,
`0x80014880`, `0x800148B0` and `0x80014A3C`. Original identifiers and addresses
remain in Doxygen. Existing declarations, biased table indexing, program-pointer
representations and expression order are preserved.

832 of the 984 starting names are recovered, with 152 names and 470 references
across 36 files remaining. The ledger contains 849 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

All 483 objects pass strict comparison before and after the clean build. The clean
build prints `OK: ROM matches`, and direct ROM byte comparison succeeds. Full
verification passes 31 tests, ledger consistency, publication hygiene and
whitespace checks; the name checker reports no errors with the same five permitted
port-binding warnings. Progress reporting was regenerated. The immutable baseline,
compiler settings and main remain unchanged.

## Audio cue selection and timed action-command tables

Nine globals now identify selected audio cue ID, playback handle and requested
rate; cue-bank ROM address and buffered reader; launch-mode vertical speed factors,
heading offsets and throw flags; and timed action-command handlers. The latter
name distinguishes this timed sequencer from the existing motion-script command
handler table. Ghidra at `0x80023150`, `0x8002327C`, `0x80023788` and
`0x80024DD0` corroborates cue transitions, bank selection, launch-mode consumers
and dispatch. Original identifiers/addresses remain in Doxygen. The launcher table
block note is consolidated into the individual declaration contracts. Types,
expressions and the existing assembler-input scheduling exception are preserved.

841 of the 984 starting names are recovered, with 143 names and 424 references
across 35 files remaining. The ledger contains 858 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

All 483 objects pass strict comparison before and after the clean build. The clean
build prints `OK: ROM matches`, and direct ROM byte comparison succeeds. Full
verification passes all 31 tests, ledger consistency, publication hygiene and
whitespace; the name checker has no errors with the same five permitted port
warnings. Progress reporting was regenerated. The immutable baseline, compiler
settings and main are unchanged.

## Particle-effect templates, styles and appearance programs

All 37 remaining globals in the particle-effect unit now identify wisp template,
track-count and opacity-index tables; per-style texture-frame programs and default
opacity; ribbon hierarchy templates and the frame program shared with cluster
renderers; callback emitter and cluster templates; falling-particle models and
initial/disappearance scale programs; converging/flickering/radial effect styles,
models and templates; and expanding-orange effect/emitter resources. Numeric wisp
style identities are retained where visual semantics have not been established.
Names describe verified resource roles rather than guessing visual identities.
Original identifiers and addresses remain in individual Doxygen contracts,
including split declarations for ribbon templates and the shared messenger program.

Ghidra corroborates initialization and creation paths at `0x80017844`,
`0x80017918`, `0x80017D18`, `0x80018514`, `0x800195B0`, `0x80019B08`,
`0x80019C80`, `0x80019DEC`, `0x8001A624`, `0x8001AEDC`, `0x8001B618`,
`0x8001B9EC` and `0x8001BE0C`. Update/draw consumers establish whether each
integer program supplies texture frames, opacity or scale. Existing types,
resource indexing, allocations and expressions are preserved.

878 of the 984 starting names are recovered, with 106 names and 348 references
across 34 files remaining. The ledger contains 895 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

All 483 objects pass strict comparison before and after the clean build. The clean
build prints `OK: ROM matches`, and direct ROM byte comparison succeeds. Full
verification passes 31 tests, ledger consistency, publication hygiene and whitespace;
the name checker reports no errors with the same five permitted port warnings.
Progress reporting was regenerated. The immutable baseline, compiler settings
and main remain unchanged.

## Startup scheduler storage, rendering guards and controller diagnostics

Nineteen globals now identify the main scheduler context, idle-thread stack top,
initial-render-state guard, camera-pitch and float-store tolerance bounds, optional
frame-hook enable flag, controller thread/client, rumble initialization/command
formats and three seconds-to-microseconds factors. Runtime startup establishes
that the former boot/main stack-area symbol is scheduler storage: its address also
serves as the boot-entry stack top. The controller scheduler-client address also
serves as its thread stack top. Both dual roles are documented without changing
the original declarations or layout. Duplicate declarations in the startup and
controller units remain consistent; each scalar alias receives its own contract.
Original identifiers/addresses remain in Doxygen; trailing format-string block
notes are consolidated into those contracts.

Ghidra corroborates startup, drawing guards/tolerances and controller setup at
`0x80000450`, `0x800004B4`, `0x8000C040`, `0x800008AC`, `0x800009C4`,
`0x800014E8`, `0x80002384`, `0x80002764`, `0x80002BD4` and `0x8000317C`.
ROM reads confirm the diagnostic strings and all three 1,000,000 conversion
factors. Main-unit consumers and controller-unit consumers were accepted in
separate strict object-comparison steps.

897 of the 984 starting names are recovered, with 87 names and 289 references
across 32 files remaining. The ledger contains 914 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

All 483 objects pass strict comparison, including the final clean build. The clean
build prints `OK: ROM matches`, and direct ROM byte comparison succeeds. Full
verification passes 31 tests, ledger consistency, publication hygiene and whitespace;
the name checker has no errors and the same five permitted port warnings.
Progress reporting was regenerated. The immutable baseline, compiler settings
and main remain unchanged.

## Tree traversal callback, audio modulation and indicator programs

Six globals now identify the active search-tree visitor, sequence tremolo and
vibrato waveforms, MIDI microseconds-to-seconds factor, horizontal HUD movement
animation and interaction-marker transform programs. Tree traversal sets and
clears its borrowed callback; nested traversals remain unsupported by that shared
state. ROM reads establish sixteen-sample unipolar/bipolar modulation tables and
the 0.000001 timing factor. Marker consumers establish two position-offset and
two scale tracks, distinct from texture-frame and opacity tracks.

Ghidra corroborates these roles at `0x80041454`, `0x80006254`, `0x8000645C`,
`0x8000530C`, `0x80022838` and `0x8001EAF4`. Original identifiers and addresses
remain in Doxygen, including the function-pointer declaration. Each defining
source unit passes strict comparison before advancing to the next mutation.

903 of the 984 starting names are recovered, with 81 names and 266 references
across 27 files remaining. The ledger contains 920 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

Final clean build prints `OK: ROM matches`, and direct ROM byte comparison passes.
All 483 objects match the immutable baseline with strict relocation addends. Full
verification passes 31 tests, ledger consistency, publication hygiene and whitespace;
the name checker reports no errors and retains five permitted port warnings.
Progress reporting was regenerated. Compiler settings and main remain unchanged.

## Terrain collision masks, visibility parameters and retained runtime storage

Six globals now identify per-policy obstructing surface masks, maze visibility
parameters, the retained diagnostic block-count word, runtime frame-sample alignment
padding, retained padding before prompt-panel storage and the second processed
controller-port sample. Ghidra at `0x8010C788`, `0x8010D540`, `0x8010E4C0`,
`0x8000AF08` and `0x8000B30C` corroborates collision-policy indexing, visibility
scale initialization, diagnostic reset, timing and secondary input traversal.
Padding roles follow the existing declarations and retained target layout; no
runtime semantics are invented for unused storage. The second controller sample
is a scalar view at one 0x18-byte ControllerState stride beyond the input base.
Original identifiers and addresses remain in Doxygen. Anonymous visibility storage
retains its unknown tail, and the unproven maze vector remains pending.

909 of the 984 starting names are recovered, with 75 names and 251 references
across 27 files remaining. The ledger contains 926 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.
Terrain and runtime source-unit changes each pass strict comparison of 483 objects.

The final clean build prints `OK: ROM matches`, and direct ROM byte comparison
passes. Full verification passes 31 tests, strict comparison of all 483 objects,
ledger consistency, publication hygiene and whitespace. The name checker has no
errors and the same five permitted port warnings. Progress reporting was
regenerated. The immutable baseline, compiler settings and main are unchanged.

## System-menu model templates and interior navigation views

Fourteen globals now identify transition/secondary/interactive model templates,
the randomly chosen closing-effect text row, selection/setting marker images,
music-preview labels, history depth, phase, retained model views and phase-start
timestamp views. Initialization, list drawing, closing effects and history changes
establish those roles. Ghidra corroborates initialization and selection at
`0x8011BAC8`, `0x80119A90`, `0x8011B880`, `0x80119610` and `0x80119670`.
Original identifiers and addresses remain in individual Doxygen contracts.

Interior state views remain linker aliases, including the low timestamp word;
no additional split symbols or member-offset rewrites are introduced. Existing
scalar declarations and full-timestamp casts retain the target representation.
The alias file is updated consistently with every consumer. Model templates,
resource addresses, expressions and menu behavior remain unchanged.

923 of the 984 starting names are recovered, with 61 names and 206 references
across 27 files remaining. The ledger contains 940 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

All 483 objects pass strict comparison before and after the clean build. The clean
build prints `OK: ROM matches`, and direct ROM byte comparison succeeds. Full
verification passes 31 tests, ledger consistency, publication hygiene and whitespace;
the name checker reports no errors and retains five permitted port warnings.
Progress reporting was regenerated. The immutable baseline, compiler settings
and main remain unchanged.

## Chapter messages and collection response tables

Six globals now identify chapter friendship messages, three per-item-category
actor-response action tables, the actor presentation joint-height midpoint and the
collection-results bypass flag. Response categories retain their numeric identities;
no unsupported preference or emotional interpretation is assigned. Shared message
consumers and the retained world-map declaration are updated consistently. Original
identifiers/addresses remain in Doxygen; the friendship table block note is
consolidated into its declaration contract.

Ghidra at `0x8002966C`, `0x800286B0`, `0x80029BE4` and `0x80029CD4`
corroborates category-indexed actions, joint-height averaging, chapter message
selection and bypass-state updates. Existing types, expressions and table layouts
are unchanged.

929 of the 984 starting names are recovered, with 55 names and 189 references
across 26 files remaining. The ledger contains 946 symbols, including seventeen
additional recoveries outside the starting inventory. Field macros, unknown-type
tokens and temporary names remain at zero; raw-offset candidates are unchanged.

All 483 objects pass strict comparison before and after the clean build. The clean
build prints `OK: ROM matches`, and direct ROM byte comparison succeeds. Full
verification passes 31 tests, ledger consistency, publication hygiene and whitespace;
the name checker has no errors with the same five permitted port warnings.
Progress reporting was regenerated. The immutable baseline, compiler settings
and main remain unchanged.

## Profile name, script globals and packed counters

Three globals now identify the player-name string borrowed by actor scripts,
500 packed five-byte global script values and 166 four-bit profile counters.
The reset routine's storage extents agree with the VM's global-variable indexing
and the counter reader/writer. Existing byte and typed storage views are retained;
no layout, expression or prototype changes are introduced. Original identifiers
and addresses remain in consolidated Doxygen declaration comments.

Ghidra at `0x800214E0`, `0x801156A0` and `0x80114D04` corroborates
profile initialization, global-variable reference resolution and counter updates.
932 of the 984 starting names are recovered; 52 names and 173 references across
25 files remain. The ledger contains 949 symbols, including seventeen additional
recoveries outside the starting inventory. Field macros, unknown-type tokens and
temporary names remain at zero; raw-offset candidates are unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`, and direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene and
whitespace. Name checks have no errors and retain five permitted port warnings.
Progress reporting was regenerated. Compiler settings, verification tooling,
the immutable baseline and main remain unchanged.

## Primary-player profile and control aliases

Five interior player-state names now identify the points balance, friendship
level, pending friendship milestone, current chase stage and companion target.
Existing access expressions and types remain unchanged. Linker aliases preserve
the established player storage boundary: explicit symbol-map entries were rejected
because strict comparison detected changed BSS defined-symbol attributes.

Ghidra at `0x8011AD24`, `0x801125AC`, `0x801127DC` and `0x801101D0`,
together with the shared player layout and companion-control setter, corroborates
these views. Original identifiers and addresses remain in Doxygen comments.
937 of the 984 original names are recovered; 47 names and 141 references in
25 files remain. The ledger contains 954 symbols, including seventeen additional
recoveries. Other quality categories are unchanged.

Strict comparison passes all 483 objects before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks (zero errors, five permitted port warnings).
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable object baseline remain unchanged.

## Companion-control messages and interaction proxy data

Four globals now identify companion-mode status messages, friendship milestone
progress flags, the unit-scale interaction-proxy template and the encoded label
used for object kind 0x89. The last retains its numeric kind because its wider
meaning is not established. Existing storage, linkage, types and expressions are
unchanged. Redundant table/template block notes are consolidated into Doxygen,
and all four declarations preserve original identifiers and addresses.

Ghidra at `0x80110C0C`, `0x80112294`, `0x80112598` and `0x80112940`
corroborates the consumers and their indexing/template roles. 941 of the 984
original names are recovered; 43 names and 132 references across 25 files remain.
The ledger contains 958 symbols, including seventeen additional recoveries.
Other quality categories remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`, and direct ROM byte comparison succeeds.
Full verification passes 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Direction-animation defaults and dispatch

Three globals now identify the default direction pose, its interior packed-direction
view and the motion-mode update table. Reset, retargeting and dispatch consumers
establish these roles. Ghidra at `0x800322B8`, `0x80032148` and `0x80032448`
corroborates the accesses; the ROM table contains the two update-function addresses.
The interior view remains a linker alias at default-pose offset 0x10. Existing
storage types, callback signatures, expression order and layouts are unchanged.
Doxygen preserves original identifiers and addresses.

944 of the 984 starting names are recovered; 40 names and 126 references across
25 files remain. The ledger contains 961 symbols, including seventeen additional
recoveries. Other quality categories remain unchanged. The unreferenced save-data
word initialized to minus one remains unresolved pending evidence of its role.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`, and direct ROM byte comparison succeeds.
Full verification passes 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Item-catalog semantic aliases

Six interior aliases now identify motion coefficients, opaque catalog tags,
collection values, presentation response categories, availability progress flags
and the collection value for kind 0x8F used in chapter progression. The opaque
tag and numeric response categories retain their unresolved wider meanings.
All consumers and existing linker/symbol declarations use the recovered names;
existing catalog storage and access expressions remain unchanged. Redundant view
comments are consolidated into Doxygen with original identifiers and addresses.

Ghidra at `0x8010FC48`, `0x8011FFA8`, `0x80029560`, `0x8002966C`,
`0x80028240` and `0x8002AD9C` corroborates the consumers. The existing item
motion-coefficient and availability getters provide additional layout evidence.
950 of the 984 original names are recovered; 34 names and 107 references across
24 files remain. The ledger contains 967 symbols, including seventeen additional
recoveries. Other quality categories remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## VM conversion text, costed actions and shared command text

Three original globals now identify the VM numeric conversion buffer, costed
script-action configuration and shared actor-script command text. Two companion
storage declarations also receive descriptive names. The shared text retains its
split declaration layout, now documented as target-specific contiguous storage.
No types, layouts, expressions, callback signatures or control flow are changed.
Original names and addresses remain in Doxygen; redundant BSS layout notes are
consolidated into the declaration contracts.

Ghidra at `0x80116104`, `0x80116BC4` and `0x8011C040` corroborates
numeric formatting, action-meter checks and bounded shared-text construction.
Each defining source unit was rebuilt and compared before moving to the next.
953 of the 984 original names are recovered; 31 names and 85 references across
21 files remain. The ledger contains 972 symbols, including nineteen additional
recoveries. Other quality categories remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Scrolling-board presets and item-mode dispatch

Two globals now identify the scrolling-board preset text references and item-mode
update callbacks. Ghidra at `0x80119508` and `0x80123390` corroborates
preset selection and mode-indexed dispatch. Existing reference representations,
callback types, storage and expressions remain unchanged. Doxygen preserves the
original identifiers and addresses. Each defining source unit was rebuilt and
strictly compared before moving to the next.

955 of the 984 original names are recovered; 29 names and 81 references across
19 files remain. The ledger contains 974 symbols, including nineteen additional
recoveries. Other quality categories remain unchanged. Remaining voice-device
identifiers need SDK ownership/canonical-name review before further renaming.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Camera-facing direction transform and sequence stop-fade volume

Two globals now identify the fixed direction transform used by camera-facing
child orientation and the sequence volume target used before a scheduled stop.
Ghidra at `0x80015AE8` and `0x800201B8` corroborates their consumers. ROM
bytes establish the transform coefficients and the stop-fade target of 0.3;
the latter is not described as zero volume. Existing representations, expressions
and scheduling-sensitive source layout are retained. Doxygen preserves original
identifiers and addresses. Each source unit was rebuilt and compared separately.

957 of the 984 original names are recovered; 27 names and 77 references across
18 files remain. The ledger contains 976 symbols, including nineteen additional
recoveries. Other quality categories remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Terrain diagnostic-grid configuration

Three globals now identify the diagnostic property-view selector, grid-root object
template and grid-cell object template. Ghidra at `0x8002B058` and `0x8002B380`
corroborates terrain-flag color selection and separate root/cell construction.
The two template declarations have individual Doxygen contracts with their
original identifiers and addresses. Existing raw layouts, expressions and
control flow remain unchanged.

960 of the 984 original names are recovered; 24 names and 70 references across
18 files remain. The ledger contains 979 symbols, including nineteen additional
recoveries. Other quality categories remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Completed-visit card state and item query/creation policy

Three globals now identify the pending completed-visit card/progress update,
nearest-item query origin and new-item hierarchy-flag-two policy. Ghidra at
`0x8000B30C`, `0x8002A0E4`, `0x80112FC0` and `0x80113944` corroborates
runtime entry, card consumption, distance queries and item creation. The policy
name describes its established flag effect; broader gameplay intent remains
unresolved. Original identifiers/addresses remain in Doxygen. Existing types,
expressions and control flow are unchanged.

The query origin remains an interior alias. Adding an explicit symbol-map entry
for the creation-policy word changed a data symbol extent, so the accepted form
uses a linker alias preserving the original extent. The comparator is unchanged.
Each defining unit was built and compared before proceeding to the next.
963 of the 984 original names are recovered; 21 names and 60 references across
16 files remain. The ledger contains 982 symbols, including nineteen additional
recoveries. Other quality categories remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Bedroom name catalog and selection history

Six globals now identify bedroom name-category codes, name string references,
a 100-entry selection-history ring and its insertion index, oldest index and
retained count. Ghidra at `0x80120D78`, `0x80120DB8` and `0x80120DFC`
corroborates catalog lookups and random selection excluding retained history.
Binary scene data stays intact; linker aliases expose the established views.
Existing declarations, expressions and control flow remain unchanged. Doxygen
preserves original identifiers and addresses, and the scene-loading requirement
remains explicit in the consuming interfaces and alias notes.

969 of the 984 original names are recovered; 15 names and 44 references across
15 files remain. The ledger contains 988 symbols, including nineteen additional
recoveries. Other quality categories remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Saved voice count and primary controller axes

Three aliases now identify the saved player voice-message count and primary
processed controller X/Y stick inputs. Messenger delivery to Pikachu increments
the counter; save/load, script queries and the status menu expose it. Ghidra at
`0x800260F8` and `0x8011A684` corroborates delivery counting and menu tilt.
The processed controller layout and producer establish the signed axis offsets.
Existing storage, expressions and declarations retain their matching views;
linker aliases preserve owner boundaries. Doxygen preserves original identifiers
and addresses, and the combined axis declaration now has separate contracts.

972 of the 984 original names are recovered; 12 names and 33 references across
13 files remain. The ledger contains 991 symbols, including nineteen additional
recoveries. Other quality categories remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Retained controller configuration storage

Two address identifiers now name retained reserved words: one precedes the
aligned repeat timing and one ends controller configuration. Source references,
target layout and Ghidra reset/update review at `0x80004AD0` and `0x80004BC0`
identify no code readers. Their names describe their storage context only;
original purpose remains explicitly unresolved. Both declarations retain their
width/order and original identifier/address in Doxygen. Redundant pad notes are
consolidated into the contracts. No storage is removed or repurposed.

974 of the 984 original address names now have descriptive replacements; ten
names and 31 references across twelve files remain. This count does not claim
semantic recovery of the reserved words. The ledger contains 993 symbols,
including nineteen additional recoveries. Other quality categories are unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Canonical VI mode table and audio RSP boundaries

Three address identifiers now expose the canonical SDK `osViModeTable` and the
audio RSP microcode text/data starts. The local SDK declares `osViModeTable`;
its binary records and scheduler/runtime consumers establish the correspondence.
Ghidra at `0x8003FC30` and `0x8001F680` corroborates video selection and audio
worker initialization. The audio text start also bounds RSP boot-code size in
audio and graphics tasks; its declaration contracts document both roles. No
specific SDK microcode variant name is asserted without identification evidence.
Microcode and SDK contents remain unchanged. Combined audio-boundary declarations
now have separate contracts preserving original identifiers and addresses.

977 of the 984 original names have descriptive or canonical replacements; seven
names and eighteen references across seven files remain. The ledger contains
996 symbols, including nineteen additional recoveries. Other quality categories
remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Voice-unit type and converter initialization commands

Two globals now identify the probed voice-unit type and five A/D-converter
initialization commands. The retained diagnostic names the type
`osEngVoiceUnitType`, and recognition requires its value to be one. Ghidra at
`0x8004C8A0` and `0x800404E8` corroborates device probing, type checking and
command submission. Individual converter-command meanings remain unresolved;
no unsupported hardware interpretation is assigned. Existing values, types,
expressions and SDK interfaces remain unchanged. Doxygen preserves original
identifiers and addresses.

979 of the 984 original names have descriptive or evidenced replacements; five
names and eleven references across five files remain. The ledger contains
998 symbols, including nineteen additional recoveries. Other quality categories
remain unchanged.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Collection title and score format strings

Two globals now identify the compiler-emitted collection title and score format
strings. Ghidra cross-references resolve `D_800A558C` to title creation at
`0x80028FB0` and `D_800A559C` to score refresh at `0x80029298`.
The ROM strings are `%s` and `@Y@Z%d`. Their retained extern views in message-bar
and diagnostic units are updated consistently; compiler-emitted literal uses
remain unchanged. Doxygen preserves original identifiers and addresses.

981 of the 984 original names have descriptive or evidenced replacements; three
names and seven references across three files remain. The ledger contains
1000 symbols, including nineteen additional recoveries. Other quality categories
remain unchanged. Remaining cases need further purpose evidence, rather than
unsupported semantic labels.

All 483 objects match the immutable baseline before and after the clean build.
The clean build prints `OK: ROM matches`; direct ROM byte comparison succeeds.
Full verification passes all 31 tests, ledger consistency, publication hygiene,
whitespace and name checks with zero errors and five permitted port warnings.
Progress was regenerated. Main, compiler settings, verification tooling and the
immutable baseline remain unchanged.

## Final three names: evidence review and corrected maze contract

The remaining address identifiers are not yet semantically recovered:

- `D_8009ADC0`: a retained initialized minus-one word in save-progress data.
  Both resident Ghidra images report only its exported entry-point reference;
  source inventory identifies no reader. A producer/consumer or original-symbol
  provenance is needed before assigning a gameplay role.
- `D_800A6D30`: a retained three-by-three integer table with rows `1,1,0`,
  `1,1,1`, `0,1,1`. Source inventory identifies no reader. Both Ghidra images
  fail to resolve the address, so their empty results are not proof of absence.
  A direct instruction/reference audit or provenance is needed.
- `D_80133DE8`: three adjacent BSS words cleared by `mazeInitialize`.
  Ghidra identifies the initialization stores at all three addresses, but no
  reader. The previous movement-vector description was unsupported and is now
  corrected in source and recovery notes. A consuming access is needed to
  establish vector semantics or a gameplay role. Split-symbol adjacency and
  aliasing constraints remain unchanged.

An aligned ROM pointer-literal scan finds no pointers into these storage ranges;
this does not rule out computed addresses, base-relative access or embedded
non-pointer data. No generic semantic names are invented to lower the count.
981 of the 984 original names remain replaced; three names and seven references
in three files remain unresolved. The objective remains active.

The source-comment correction passes strict comparison of all 483 objects before
and after a clean build. The build prints `OK: ROM matches`; direct byte comparison
succeeds. Full verification passes 31 tests, ledger consistency, hygiene,
whitespace and name checks (zero errors, five permitted port warnings). Progress
was regenerated. Main, the immutable baseline and verification tooling are unchanged.

## Direct reference audit of the unresolved ranges

`GLOBAL_RECOVERY_REFERENCE_AUDIT.json` records a named-symbol relocation audit
of all 483 current objects and 24,220 allocated-section relocations. Only the
known maze initialization address construction resolves into the three target
ranges. The audit explicitly excludes section-symbol addends, computed/base-relative
addresses and raw instructions; it is not a proof of complete absence of readers.
Linked disassembly low-immediate candidates were reviewed: cave description
addresses use upper half 0x800B, and the voice dictionary lookup uses 0x8010.
Neither accesses the math table at 0x800A6D30. The maze stores are confirmed
at offsets zero, four and eight from 0x80133DE8.

The second resident Ghidra image confirms only an exported entry-point reference
for the save word. Both images fail symbol lookup for the math table, while raw
byte reading succeeds and confirms its nine integers. This separates a tool
lookup limitation from actual evidence of unused data. No supported gameplay
purpose emerges for the final three names; the count remains 981/984. Further
reference reconstruction or provenance is still needed. No source or binary
changes are made by this audit.

## Section-relative follow-up

The reference audit now includes the three owning objects' relevant allocated
sections. Their 83 HI16/LO16 pairs resolve without unmatched high relocations:
44 pairs target math rodata, two target save-progress data and 37 target maze
BSS. Every high immediate is zero in the relocatable objects. None targets the
math table or save word. Save data references resolve to 0x8009ADC4 and
0x8009ADDC, the two known initializer images. Only the previously identified
maze initialization pair targets its unresolved three-word range. Full decoded
pair locations and targets are recorded in the JSON audit.

This closes the owning-unit section-symbol-addend gap for those relocation
forms, but cannot reconstruct a gameplay purpose for data without a consumer.
The final three remain unresolved; generic names or speculative vector/mask
meanings would not meet the evidence requirements. Original symbols, another
build with surviving consumers, or new runtime evidence are needed to proceed
with semantic naming. No source, object or ROM changes accompany this audit.
