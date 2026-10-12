# Hey You, Pikachu! (USA) — Matching Decompilation

```
                                 ,'\
                   _.----.    __,'  |
                 ,'      _`-,'   _,'
                /    _.-'      ,'            HEY YOU, PIKACHU!
               /  ,-'     o  _/                  (NPGE / USA)
              |  /    __..--''
              | |_.-''  ____ \              a byte-matching N64
              ,'    _.-'    \ \                 decompilation
             /  _.-'         \ \
            (_,'   \          > )
              \_    `.__     / /
                `-._    ``--'_/        ROM:  16 MB,  CIC-NUS-6102
                    `--..__,'          SHA1: da1a1e47...41bda824
```

This repo rebuilds the retail *Hey You, Pikachu!* ROM **byte for byte**
from game C, locally generated SDK assembly, and binary asset slices extracted
from a user-supplied ROM. Every
build ends with a SHA-1 check against the retail dump — if it prints
`OK: ROM matches`, the output is indistinguishable from the cartridge.

## Quick start

```sh
# 0. put your legally dumped ROM in rom/  (any of .z64/.v64/.n64 byte orders)
./tools/setup.sh          # binutils (brew), checksum-pinned IDO, ROM normalize+verify
python3 tools/materialize_ultralib.py --from /path/to/local-sdk-inputs
make -j                   #   -> build/pikachu.z64  + SHA-1 check
make verify-clean         # publication hygiene checks (not legal clearance)
```

Make is the build engine; decomp tooling (splat, asm-differ, decomp.me) assumes the
Makefile layout and the toolchain is inherently non-hermetic, so there is no other build
front end. Everything the build downloads is pinned: compilers by SHA-256, helper
repositories by commit. SDK-related headers must be supplied locally; their SHA-256
hashes are pinned in `docs/LOCAL_SDK_FILES.json`. See [Local SDK inputs](docs/LOCAL_SDK.md).

Related repositories: `ido-remarks` (compiler-decision diagnostics) and `hey-you-pikachu-ai`
(native port with local voice control).

## How a matching decomp works

```
 rom/baserom.us.z64 ─┬─(extract)──> assets/*.bin ───────────────┐
                     │                                          │
 asm/*.s ────────────┼────────(GNU as)──> build/asm/*.o ────────┤
                     │                                          ├─(ld + pikachu.ld)─> ELF
 src/*.c ──(asm-processor + IDO 5.3 cc)─> build/src/*.o ────────┘        │
                                                                   (objcopy)
                                                                         ▼
                                                              build/pikachu.z64
                                                                 sha1 == retail
```

* **IDO 5.3** is the original 1998 SGI IRIX compiler, statically
  recompiled by [decompals/ido-static-recomp] to run natively on this
  machine (Apple Silicon included). Only this compiler reproduces the
  retail game instruction stream; the default is `-O2 -mips2 -32 -G0`.
* **asm-processor** lets one `.c` file mix matched C with
  `#pragma GLOBAL_ASM(...)` blocks, so code is decompiled one function
  at a time while the ROM keeps matching.
* **splat** produced the original split (`pikachu.yaml`,
  `symbol_addrs.txt`, `pikachu.ld`); re-run it only when re-splitting
  segments. Make regenerates the split when its inputs change.

Base split (yaml/symbols/asm layout) derives from
[farisawan-2000/hey_you_pikachu]. SDK-related headers in `include/` are
local inputs excluded from version control; SDK implementations are generated
under ignored `asm/` from the supplied ROM.

## Status

The build reproduces the retail ROM byte for byte. Game functions remain in C,
with one game assembly fallback under `src/nonmatching/`. SDK implementations
are excluded from the source tree and regenerated locally. Run `make progress`
for current game-source counts; generated SDK functions are excluded from those counts.

| region | state |
| ------ | ----- |
| header / IPL3 | extracted binary |
| boot segment, ROM 0x1000–0xA9800 | game C; SDK assembly generated locally |
| resident overlay, ROM 0xB3C70–0xDCF60 | split into 15 C units with owned rodata and bss |
| 23 scene overlays, ROM 0xDCF60–0x4B4990 | all text split; 11 scenes fully matched, all spatial lookups matched; scene data retained as binary |
| remaining assets | extracted binary; inventory research continues |

Agents should start with [docs/AGENT_WORKFLOW.md](docs/AGENT_WORKFLOW.md) for tool selection,
bounded searches, verification, and interruption recovery.

Every default build prints an assembly fallback summary. Run `make asm-fallbacks`
for function names and source paths. Machine-readable manifests remain in `docs/`.

## Reading the code

[Voice interaction and actor scripts](docs/VOICE_AND_SCRIPT_VM.md) traces recognition,
message delivery, cooperative VM execution, and the lower-level action sequencer.

## Undefined behavior policy

Matched C is a witness for machine code, not portable ISO C. Every site
that leans on UB or implementation-defined behavior is annotated with
`UB:` / `NONPORTABLE:` markers and indexed in **[docs/UB.md]** — read
that before retargeting any of this code at a modern compiler, where
marked sites may be deleted, reordered, or miscompile.

## Legal

You must supply your own dump of a cartridge you own; this repo
contains no Nintendo-copyrighted data, and the build refuses dumps that
don't hash to the retail image. The decompiled sources document the
original work for interoperability and preservation.

[decompals/ido-static-recomp]: https://github.com/decompals/ido-static-recomp
[farisawan-2000/hey_you_pikachu]: https://github.com/farisawan-2000/hey_you_pikachu
[docs/UB.md]: docs/UB.md
