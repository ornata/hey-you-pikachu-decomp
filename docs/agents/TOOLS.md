# Tools guide

Navigation for the repository's `tools/` directory. Each tool's `--help` and module
docstring are authoritative. Run commands from the repository root.

Use `make asm-fallbacks` to list functions still represented by assembly.
Compiler-decision diagnostics live in the separate ido-diagnostics repository.

## Rules that still apply

- Use Ghidra MCP to recover behavior and data references; decompiled C is a hypothesis.
  Verify against the generated assembly and the ROM.
- A change is accepted only after `make clean && make -j8` prints `OK: ROM matches`
  (SHA1 `da1a1e47a86720f9d54fb2d2d247480041bda824`).
- Keep one source-unit mutation active per checkout.

## Ghidra MCP: behavior and call/data context

Use the configured local MCP endpoint `http://127.0.0.1:8765/mcp`; prefer its
read-only query tools for matching work. The server also exposes mutation tools
(prototype/type/name/comment edits and import/delete operations); do not use
those in shared analysis projects unless the task specifically requires it.
The shared project contains the resident analysis binary
`/pikachu-resident-functions.elf-b65af7`; scene overlays may need their own
program. The server launcher/project are machine-local, not repo files.

### If the Ghidra tools are missing or the server is down

The server is `pyghidra-mcp` on `127.0.0.1:8765`, started by the maintainer's
launcher `~/ghidra-projects/hey-you-pikachu/start-mcp.sh`. The repo's `.mcp.json`
registers it as the `ghidra` MCP server for Claude Code sessions, but a session
only gets the native tools if the server was up when it started, and a subagent
spawned earlier cannot add tools mid-session. Use the shell helper instead; it
needs only Python 3 and works for any agent:

```sh
python3 tools/ghidra_mcp.py status        # up/down
python3 tools/ghidra_mcp.py start         # launches the configured launcher if down; waits until it answers
python3 tools/ghidra_mcp.py tools         # tool names and arguments
python3 tools/ghidra_mcp.py call search_symbols_by_name binary_name=/pikachu-resident-functions.elf-b65af7 query=func_8003D030
python3 tools/ghidra_mcp.py call decompile_function binary_name=/pikachu-resident-functions.elf-b65af7 name_or_address=func_8003D030 include_callees=true
python3 tools/ghidra_mcp.py call disassemble binary_name=/pikachu-resident-functions.elf-b65af7 address=0x8003D030 count=60
python3 tools/ghidra_mcp.py stop          # only a server this helper started
```

- Start it only when `status` says down; many agents share one server, and one
  start is enough. Do not delete the project, its `pikachu.lock`, or re-import
  binaries. The launcher path can be overridden with `GHIDRA_MCP_LAUNCHER` (and
  the URL with `GHIDRA_MCP_URL`) if this machine differs. Its log is
  `$TMPDIR/ghidra-mcp.log`.
- Binary names are the identities from `list_project_binaries` (the resident
  analysis binary is `/pikachu-resident-functions.elf-b65af7`; the project also
  holds the IDO `as1` and `uopt` binaries, e.g. `/as1-8547d2` and `/uopt-01965f`,
  useful for compiler-behavior questions). A wrong name makes the tool list the
  valid ones.
- Use the read-only query tools (`decompile_function`, `disassemble`, `read_bytes`,
  `list_xrefs`, `search_*`, `gen_callgraph`, `list_project_*`). Do not call
  `rename_*`, `set_*`, `save`, `import_binary` or `delete_project_binary` on the
  shared project for matching work.
- If `start` fails (launcher missing, Java/Ghidra not found, project locked by a
  live Ghidra GUI), report the error and ask the maintainer rather than working
  around it; the Ghidra output is a hypothesis either way, and the generated
  MIPS assembly and ROM remain the ground truth.

| MCP tool | Use it for | Important limit |
| --- | --- | --- |
| `list_project_binaries`, `list_project_binary_metadata` | Discover a program and check architecture, hash, and analysis status. | Metadata/index status is not byte-coverage proof. |
| `search_symbols_by_name` | Find exact function/data symbols before decompiling. | Addresses can be ambiguous when normal and `.NON_MATCHING` symbols overlap; prefer exact names. |
| `decompile_function` | Behavioral draft, helper/caller context, strings, and bounded xrefs (`include_callees`, `include_xrefs`, `include_strings`). | Inferred types/prototypes, caller sets, and pseudo-C control flow can be incomplete. Check assembly. |
| `disassemble` | Inspect instruction operands and delay slots at an exact address. | It may skip undecoded/unreachable bytes or run into a neighboring function. Check the first address and full requested span. |
| `read_bytes` | Authenticate a continuous byte span against ROM/ELF and catch listing gaps. | Static bytes authenticate provenance, not semantics or C equivalence. |
| `list_xrefs`, `search_strings`, `search_code`, `gen_callgraph` | Find callers, data/string use, related code, and call neighborhoods. | Search and call graphs are discovery aids; indirect callers and memory effects may be missing. |

Avoid changing prototypes, variable types, comments, or other shared Ghidra
database state for ordinary matching work. Use [`tools/prepare_ghidra_symbols.py`](../../tools/prepare_ghidra_symbols.py)
to prepare a separate analysis ELF when duplicate symbols obstruct import; never
use that analysis copy as a build input. 


## Tools in this tree

| Tool | Purpose |
| --- | --- |
| `make asm-fallbacks` / `tools/asm_fallbacks.py` | List functions the default build still assembles (`src/nonmatching/`). |
| `tools/progress.py` | Matched/unmatched counts per unit. |
| `tools/mkctx.py` | Preprocessed context for m2c/decomp.me after header changes. |
| `tools/check_clean.sh` | Tracked-content hygiene check (no ROM, assets or blobs). |
| `tools/ghidra_mcp.py` | Start/query the local Ghidra MCP server (section above). |
| `tools/prepare_ghidra_symbols.py` | Prepare a separate analysis ELF when duplicate symbols block import. |
| `tools/extract_assets.py`, `tools/rom.py` | Normalize the ROM dump and extract raw asset segments (keep outputs external). |
| `tools/materialize_ultralib.py` | Verify or import locally supplied SDK-related headers (`docs/LOCAL_SDK_FILES.json`); no downloads. |
| `tools/compile_positioned_unit.py` | Link a hand-written unit's accepted C into its original function slots (`POSITIONED_UNITS`). |
| `tools/ido_binasm_patch.py` + `tools/binasm_patches/` | Compiler command for the one unit (`action_script_commands`) with an accepted assembler-input exception. |
| `tools/set_o32abi_bit.py` | Tag IDO `-mips3 -32` objects as o32 so GNU ld links them. |
| `tools/asm-processor/` | `GLOBAL_ASM` / data injection used by every C unit's build. |
| `tools/run_game.sh` | Optional local runtime launch. |

Nix supplies the pinned IDO compiler. Optional m2c, asm-differ and decomp-permuter
checkouts must be obtained separately; their `tools/` paths remain git-ignored.

Tests: `python3 -m pytest tests`.
