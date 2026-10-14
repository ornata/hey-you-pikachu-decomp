# Workflow

Game functions in this tree match the ROM. SDK implementations are generated locally
from the supplied ROM and must never be committed. See `docs/LOCAL_SDK.md`.
Remaining game assembly fallbacks are listed by `make asm-fallbacks`.

- Inspect `git status --short` before editing. Keep one source-unit mutation active per
  checkout; use isolated checkouts for parallel work.
- Build with `make -j8`. Every default build reports assembly fallbacks and checks the ROM
  SHA1 (`da1a1e47a86720f9d54fb2d2d247480041bda824`).
- The Makefile owns the compiler pins: IDO 5.3, usually `-O2 -mips2`, with per-unit exceptions.
- Before renaming symbols, read `docs/FROZEN_NAMES.json`. libultra and its libraries (os, audio, gu,
  libc, libm, mgu and the hand-written asm units) are always protected: renaming one is an error.
  Only game-side names the recompiled port binds (`main`, the controller/voice overrides) warn and
  say what to fix in the port. `make check-names` (optionally `BASE=<ref>`, `STRICT=1`) runs it.
- Format with `.clang-format`. Acceptance is a full object comparison and
  `make clean && make -j8` ending in `OK: ROM matches`.
- Run `make check-interfaces` after interface changes. See `INTERFACE_CONTRACTS.md`
  for scope and the reviewed ABI exceptions; do not refresh exceptions without
  reviewing their source and matching evidence.
- After accepting a change: `uv run python tools/mkctx.py` if headers changed,
  `python3 tools/progress.py`, `bash tools/check_clean.sh`, `git diff --check`, and
  `python3 -m pytest tests`.
- Use the Ghidra MCP (see AGENTS.md) and `tools/m2c` for first drafts; `tools/asm-differ` and
  `tools/decomp-permuter` are fetched by `tools/setup.sh` if you want them.
- `src/nonmatching/` holds the ROM's own assembly for functions with no matching C yet;
  SDK assembly belongs only in ignored `asm/`, never `src/nonmatching/`.

## Incremental builds and verification

C and assembly objects have generated `.d` files under `build/`. The dependency
scanner follows literal nested includes, assembly `.include` / `.incbin` inputs,
and `GLOBAL_ASM` files. Both branches of conditional includes are tracked, so a
header can trigger more rebuilds than strictly necessary. Optional missing headers
are tracked through their nearest existing directories; an active missing header
still fails compilation. Macro include operands are rejected explicitly. Missing
`.d` files force their objects to rebuild. The compiler and its flags are unchanged.

Before editing, preserve a matching baseline with `make expected`, or copy the
whole accepted `build/` directory elsewhere. Do not overwrite that baseline during
the change. Then run:

```sh
make clean
make -j8
make verify BASE=main BASELINE=expected/build
cmp rom/baserom.us.z64 build/pikachu.z64
```

`make check-sequence-bank` validates the selected retail cue bank against the
registered event limit and storage; see `SHARED_LAYOUT_AUDIT.md` for scope.

`make verify` checks the ROM SHA1, frozen names, sequence-bank storage, publication hygiene, Python tests,
object comparison, and whitespace. `BASE` selects the name-checker Git reference;
`BASELINE` selects the object baseline directory. Verification fails if the baseline
is missing. Header changes also require regenerating context and reporting progress
as described above.

The standalone command `python3 tools/compare_objects.py OLD_BUILD NEW_BUILD` (or
`make compare-objects BASELINE=OLD_BUILD`) compares every object and requires both
directories to contain their linked `pikachu.elf`. It checks object inventory,
allocated section bytes and layout (including BSS), defined symbol attributes,
and relocation identities resolved through each linked ELF. Debug data and symbol
spelling are ignored. Relocation addends remain strict: a record-plus-offset rewrite
can fail this comparison even when the final ROM matches and needs explicit review.
Object comparison supplements the clean-ROM gate; it does not replace it.

## Type and name recovery

See `TYPE_RECOVERY.md` for the recovered interfaces, baseline counts and remaining
matching constraints. `make source-quality` reports semantic placeholders separately
from matching-function progress; `JSON=1` includes occurrence locations and context.
After source edits, update `TYPE_RECOVERY_NOTES.json` if evidence changes and regenerate
`TYPE_RECOVERY.json` with `python3 tools/source_quality.py --ledger`. `make verify`
checks that this ledger still matches the source. Do not treat raw-offset candidates
as errors or invent types/names to make the inventory reach zero.

Use `python3 tools/layout_contracts.py --json` to inventory offset-named opaque
record members (both unknown and unk spellings) separately from source-quality token counts. See
`SHARED_LAYOUT_AUDIT.md` for the inventory's lexical scope and limitations.

`make check-layouts` requires a current review of every offset-named opaque
record member in `docs/OPAQUE_LAYOUTS.json`. It runs under `make verify`. Review
known layout facts, missing evidence and matching constraints manually before
changing a declaration's review; the tool never accepts or refreshes entries.
Source-line movement does not invalidate a declaration's review.

`make check-native-commands` checks every retained actor native-command table
target against its written callback signature and runs under `make verify`.
Use `python3 tools/native_commands.py --json` for the current selector catalog.
The check requires the linked build; it does not infer stack effects or cover
arbitrary caller-supplied VM tables.

`make check-object-draw-callbacks` validates lifecycle, drawing, child-drawing and response
callback signatures for known retail kinds zero through 0xB3 and runs under
`make verify`. Alternate tables and resource validity are outside this check.
`python3 tools/object_draw_callbacks.py --json` lists the audited targets.

`make check-item-callbacks` validates held, release, launch and collision hooks for
all 166 retained item kinds, plus the seven required mode-update entries. It runs
under `make verify`. `python3 tools/item_callbacks.py --json` lists the targets.
The explicit table extents come from the C catalog and mode dispatcher; linker
symbol sizes also include or omit neighboring data and do not establish these bounds.
Alternate tables, resource validity and callback side effects are outside the check.
