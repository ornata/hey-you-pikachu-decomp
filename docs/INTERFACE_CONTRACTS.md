# Interface contracts

The interface sweep covers written game C definitions and declarations, direct
calls to functions with a C definition, old-style callback tables, and shared
header coverage. Every finding in this scope has been resolved or reviewed in
[INTERFACE_CONTRACTS.json](INTERFACE_CONTRACTS.json). Assembly-only functions and
SDK interfaces are not assigned inferred contracts by this audit.

Retained findings describe matching-only call representations.
Each exception records its exact signature or arity, source location, reason,
and evidence needed to replace it. Arity exceptions also identify the caller.
The sole function without a shared header is the inactive `heapDebugNoOp` hook,
whose original callers disagree with its K&R formal argument count. Publishing
a portable prototype for that hook would misrepresent its retained ABI.

## Recovered interfaces

Functions referenced across game C source units now have shared, documented
declarations, except for that diagnostic hook. Headers describe purpose,
parameters, results, and established
ownership and lifetime requirements; their types reuse the existing shared
records. Original identifier and address notes remain available.

The sweep corrected the VM integer reader, ROM byte reader, text-window
background drawing, help-text string length call, chapter-assignment wrapper,
CRC result, effect result, actor action flags, area names, and several pointer
and scalar argument views. The menu chapter wrapper now explicitly forwards
its incoming argument; its registration site remains unproven. The four-bit
outline interface reuses `TextImage` instead of a duplicate bitmap record.

The remaining compatibility views include heap allocation, collection hierarchy
calls, integer versus pointer ROM offsets, the font helper's unused argument,
the progress reader's dead formal argument, pointer-shaped tree keys, device
word-count width, and the orbit query's return width. The interaction action
table now uses a common one-parameter callback contract. The pickup wrapper
explicitly returns the existing item-pickup result, as confirmed by the ROM
return register and dispatcher; no handler requires a controller argument.

## Shared layout and callback continuation

Shared headers document actor/object ownership, embedded VM and action state,
item and player storage, scene resources, MIDI/sequence playback and audio
lifetimes. Target size/offset assertions constrain the recovered layouts;
[OPAQUE_LAYOUTS.json](OPAQUE_LAYOUTS.json) retains unresolved-member evidence.

Actor and item direction poses now expose ambient RGB and two directional-light
slots in the original RSP layout. The named view reuses SDK component records
without imposing the eight-byte alignment of `Lights2` on enclosing game state.
Direction vectors are signed bytes at pose offsets 0x10 and 0x20; reset retains
its separate word-copy view for matching. `directionPoseSetAngles` borrows a
complete pose and changes one direction while preserving colors and slot tails.

The retained native-command table has 278 actor-only handlers. Object-kind
lifecycle, drawing, child-drawing and response slots agree across the audited
180 kinds (537 populated slots). Item kind/mode tables have 189 populated slots;
runtime and scene entry tables have 49. These linked-table checks supplement the
lexical declaration audit. Action, player-mode, controller, menu, effect and
presentation callback contracts are documented with their owning types and
dispatchers.

`make check-sequence-bank` bounds and parses the selected retail bank at ROM
0xB140C0 under its registered limit and storage of 20 bytes. Its 45 cues contain
no SysEx events and require at most 18 bytes per write. This covers the selected
bank, not alternate or constructed streams; the MIDI reader's SysEx contract
still requires min(payload length, limit) + 1 bytes including the status byte.

Offset-named opaque members retain individual reviews in
[OPAQUE_LAYOUTS.json](OPAQUE_LAYOUTS.json), with known facts, missing evidence and
matching constraints. Partial views and unused storage are deliberately explicit;
this does not assert that their unknown semantics have been recovered. Likewise,
reviewed target-ABI exceptions do not become portable C contracts merely because
the linked ROM matches. SDK and assembly-only functions remain outside the written
C interface audit.

## Verification and future changes

Run `make check-interfaces` for a concise audit or
`python3 tools/interface_contracts.py --json` for declarations and findings with
current locations. `make verify` runs the interface gate alongside the existing
ROM, frozen-name, hygiene, test, recovery-ledger, and strict object checks.
New findings, missing review evidence, changed exception fingerprints, and stale
exceptions fail the gate. Adding an exception is a manual evidence review; the
tool does not accept or refresh exceptions automatically.

This is a lexical audit, not a replacement for C type checking, callback and
ownership analysis, or ROM evidence. Some header includes retain their original
declaration boundary because moving all declarations to the top can change
earlier implicit call handling under IDO. Dependency tracking still rebuilds
every consumer when those headers change.

Acceptance requires a clean build printing `OK: ROM matches`, direct ROM byte
comparison, and strict comparison against the preserved 483-object baseline.
The comparator and its relocation-addend checks are unchanged. New and changed
interface headers are also checked independently with the target compiler.

## Heap validation and cursor selection

`HpHdr.first` anchors the circular block list; `HpHdr.last` is the allocation
search cursor. Block payload sizes are unsigned 32-bit capacities, while
`heapGetAllocationSize` preserves its signed return representation. The private
block states and validity markers are named in `src/game/heap.c`; their byte-sized
storage and the 16-byte header remain unchanged.

`heapValidateBlock` rejects an odd address before reading metadata. Otherwise the
caller must supply a readable header: validation checks the magic marker and,
when the requested state is positive, the tag. Nonpositive state requests accept
any tag. Validation does not establish heap membership or sixteen-byte alignment.

`heapAdvanceAllocationCursor` selects the block following a validated allocation.
`heapSelectAllocationRegion` searches from the cursor, stopping once its best free
candidate meets the requested size; if none meets it, the largest free candidate
wins. If there is no free candidate, the existing cursor remains. These helpers
return prospective payload addresses without reserving storage or changing block
ownership. Sixteen original full-helper probes cover validation, signed size
reads, selection, fallback and failure preservation; block metadata and payloads
remain unchanged by cursor selection. The legacy diagnostic-hook ABI exception
above remains in force.
