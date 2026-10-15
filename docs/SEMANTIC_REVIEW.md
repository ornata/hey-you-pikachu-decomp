# Semantic review

ROM identity establishes machine-code fidelity. It does not establish that a name,
constant, field interpretation or comment is correct. These optional tools check
predeclared observations against original ROM instructions.

## Bounded review

```sh
nix develop .#review --command bash
uv run --locked --group review python -m tools.semantic_review.emulator.build_core
# Pass the printed library path:
uv run --locked --group review python -m tools.semantic_review check --core /path/to/library
```

The review runs six integer ISA probes in Unicorn and the pinned Mupen64Plus
interpreter, then selected function experiments with three interpreter repetitions.
Cases and expectations live under `tools/semantic_review/` and `tests/semantic_review/`.
They use ROM-authenticated code with synthetic input state; they do not establish
that the state occurs during gameplay. Reports distinguish supported, contradicted,
inconclusive and unsupported claims. Exit zero covers only the reported experiments.
Exit one indicates a contradicted observation; exit two indicates unsupported execution.

Unicorn uses an R4000 model, not an R4300. It excludes device accesses, interrupts
and full-system timing. Integer ISA probes do not validate FPU rounding, MMU or
hardware fidelity. Use the full-system backend for device and scheduling claims.

## Voice integration and live captures

```sh
uv run --locked --group review python -m tools.semantic_review.emulator.build_rsp
uv run --locked --group review python -m tools.semantic_review.emulator.build_rice
uv run --locked --group review python -m tools.semantic_review.emulator.integration \
  --rom rom/baserom.us.z64 --core /path/to/library \
  --source build/semantic-review/upstream/core --rsp /path/to/rsp-plugin
uv run --locked --group review python -m tools.semantic_review.emulator.voice_integration \
  --rom rom/baserom.us.z64 --core /path/to/library \
  --source build/semantic-review/upstream/core --rsp /path/to/rsp-plugin \
  --gfx /path/to/rice-plugin
```

The startup scenario observes dictionary clearing and uploads. The command scenario
compares an injected VRU recognition result with a no-speech control. It follows
message translation, delivery, VM branching and action dispatch, without proving
physical speech recognition or an animation's emotional meaning. Both run repeated
fresh sessions with local saves. Graphics are required for task completion; audio
is dummy. Give each invocation a fresh output directory.

`capture SCENARIO.json --core LIBRARY` records selected checkpoints and device
events. A scenario with `replay_slice` can export a bounded live snapshot for
`replay SNAPSHOT.json`. Snapshots require explicit ROM provenance and resolved
overlay identity. Guest memory injection is marked synthetic; the ROM on disk is
unchanged. Scenarios and expected observations must be written before execution.

## Numeric recovery

```sh
nix develop .#lint --command bash
uv run --locked python tools/clang_tidy.py --output build/clang-tidy/numeric-review
uv run --locked python -m tools.magic_numbers \
  --report build/clang-tidy/numeric-review/report.json
```

Clang's documented K&R parsing exception exits nonzero but retains an explicitly
incomplete report. The numeric inventory rejects stale sources, deduplicates header
findings and groups literals by source/function/domain. Its categories are lexical
review hints, not recovered meanings. Data tables, ABI assertions, unknown values
and SDK macro expansions are retained unless stronger evidence supports a change.

Reuse existing constants first. Preserve storage width, signedness, literal suffixes
and floating-point precision. A named enum does not require changing a byte field's
type. Every source correction still needs the clean-ROM and strict object gates.

## Other emulator evidence

The current automated capture backend is Mupen64Plus. Upstream ares exposes N64
GDB register/memory hooks and breakpoint support in its
[system debugger interface](https://github.com/ares-emulator/ares/blob/master/ares/n64/system/system.cpp)
and [debugger server](https://github.com/ares-emulator/ares/tree/master/nall/gdb).
An ares adapter would need a pinned version, isolated configuration/saves,
register-format validation, ROM/overlay authentication and bounded stop handling
before its observations can enter these verdicts. Launching ares alone supplies
no automated semantic verdict; no ares cross-validation is claimed here.

Reports, saves, upstream clones and native binaries stay under `build/semantic-review/`.
Native object files use temporary directories removed after building, so strict game
object verification retains its full inventory. Captures may contain original game
bytes and remain untracked. Normal builds do not require review dependencies.

## Reviewed numeric domains

The numeric scenarios include integer VM operators, finite floating equality,
bytecode calls/returns and frames, packed array access, native continuation/yield/
action-wait results, actor flag widths, item holder restrictions, forced release,
item display-name selection and the nominal live-item threshold. They retain
numeric encodings as independent expectations rather than importing the recovered
C enum values. Detailed observations and engine limitations remain in the reports.

Object draw/update group bits live in `object_group.h`, separate from interfaces
in `object_flags.h`; importing constants need not introduce a new function prototype
into a unit with a reviewed legacy call representation. Item transfer permission
bypasses specific holder checks, not every pickup restriction. Model-dimension
sentinels retain their exact floating-point literal and do not change catalog data.

System cases cover cached EEPROM device/state predicates, the retry-score boundary,
scene destination wrapping, and audio output mode/pan behavior. EEPROM retry cases
enter an explicitly bounded original failure branch; they establish CPU behavior
at that boundary, not physical EEPROM failures or retry timing. Scene identifiers
follow the retained overlay catalog. Audio cases preserve the different handling
of raw modes by the sequence and sound-effect drivers, including their storage
widths; they make no claim about audible output.

The unsigned RNG unit scale retains its exact double literal. Substitutions at
direct calls and verified local-variable uses preserve casts and evaluation order; subsequent conversion to float
can still round a result to one. The saved mono-output field retains its one-bit
range and the existing subtraction used to convert between saved and driver values.

Progress-field cases exercise the retail signed-word, unsigned-halfword, signed-byte,
unsigned-byte and packed four-/two-/one-bit encodings. Synthetic aligned field
locations isolate sign extension, clamping and preservation of neighboring bits
across the complete progress record. They do not establish the full layout
allocator or physical persistence. The signed-halfword encoding is retained from
the original accessor branches and limits table; no retained retail field selects it.
Runtime review should use an immutable linked ELF while source builds are active.

Actor-cue cases distinguish action-kind selection tags from timed operations. They
cover due-time boundaries, jump targets and clock resets, synchronization of motion
timers, both stopping codes, and deterministic/random clock handlers. The retained
handler table establishes the operation-to-function mapping; these cases do not
exercise every effect, sound, or item side effect of those handlers.

Isolated interpreter replay defaults to the game's paired 32-bit floating-register
layout. A temporary guest `MTC0 Status` establishes the core's register pointers
before target entry; its scratch bytes and the requested registers are restored.
The contrasting wide-register probe checks that the mode selection takes effect.
Neither probe certifies floating rounding state: FCR31 is unavailable through this
debugger API and remains an explicit limitation.

Runtime-mode request cases check raw identifier storage, entry/render flags,
low-word millisecond timestamps and saved play-time accounting. They include
unresolved and out-of-dispatch values because requests do not validate modes.
Names for supported modes come from their update/render consumers and request
sites; these bounded request cases do not validate UI rendering or save I/O.

Spatial-grid cases explicitly identify one of the 23 scene overlays and retain
its authenticated retail directory and cell data. Float arguments use the paired
O32 register inputs established by the original sampler instructions. Each scene
has nonzero interior and negative-near-zero observations, absent-layer controls,
and lower/upper coordinate bounds. These check lookup geometry and preserve
truncation at negative boundaries; raw cell values do not establish collision or
event semantics. Vertical layer spacing and horizontal cell spacing have separate
private names even when their numeric values coincide.

Amplifier timing constants retain OS-tick units and exact original encodings for
held-trigger refresh, release grace and off-state re-enabling. Static instruction
and call-site evidence establishes those roles; the names do not certify physical
amplifier timing or recognition behavior.

MIDI decoder cases retain independent numeric streams for variable-length quantity
boundaries, running status, channel-message lengths, SysEx/meta events and truncated
payloads. They check consumed input, the full reader record and output guards.
The original SysEx running-status retention and logical lengths of discarded
payloads remain unchanged. Message status, system-family classification, controller
identifiers and quantity continuation bits have separate names or canonical SDK
constants; equal numeric values do not make those roles interchangeable.

Controller-state cases cover coarse/fine bank, pan and data-entry updates,
registered-parameter selection and stored controller values with an empty voice
list. The controller identifiers agree with the
[MIDI Association table](https://midi.org/midi-1-0-control-change-messages);
these names do not normalize portamento values or change the engine's fractional
bend-sensitivity representation. Active-voice effects and audible conformance
remain outside these cases.

## Retained numeric values

Zero magic-number findings is not a recovery target. Keep ABI size/offset checks
literal so they remain independent of the definitions they check. Numeric enum
definitions, authored tables, mathematical coefficients, component indices and
format geometry also have legitimate numeric representations. The inventory's
categories are lexical: a shift may encode a packed field rather than a flag,
and a bracket may select a component rather than declare a capacity.

Unresolved values remain explicit. These include the VM's distinct ignored opcode
bytes 0–2, raw voice command statuses, action-kind tags, runtime modes 9/10,
resource selectors and unidentified flag bits. Naming them requires evidence from
their specific producers, consumers or resource catalogs. Observing a branch alone
does not establish a device state, animation meaning or gameplay intent.

Selector scenarios contrast the ignored VM bytes with a following literal and
check unchanged stack storage. Numeric-tag cases compare tags 0 and 2 at signed
integer boundaries, reject both in the decimal-string parser and contrast their
reader with fractional floating input. These are synthetic consumer tests, not
evidence for the original language's terminology. The local configuration catalog
separately identifies tag 2 as a code-reference encoding at bytecode targets and
native-command selectors. Additional original-instruction cases exercise that tag
through jumps, conditional branches and native continue/yield/action-wait results.

The numeric inventory and category/domain retention report belong under
`build/magic-numbers/`. Runtime reports retain each engine's verdict separately;
an interpreter observation cannot clear an unsupported Unicorn path or override
a contradiction. Combining reports must check the registered case inventory and
preserve the provenance of each input report.
