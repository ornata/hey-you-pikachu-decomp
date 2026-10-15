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
