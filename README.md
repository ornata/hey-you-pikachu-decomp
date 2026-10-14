# Hey You, Pikachu! (USA) Decompilation

This is a decompilation of Hey You, Pikachu for the N64 to C code, and assembly code where it seemed impossible to legally generate a valid C program via the IDO toolchain.

This repository does not contain game assets. Individually, this repository cannot recreate Hey You Pikachu. In order to produce a game, you must provide your own ROM.

This decompilation was done using AI. It may be used as a reference for other decompilation work, or used as-is.

If you compile this repository with your provided ROM, the build system will check the built result's SHA-1 against the game's. On success, it will print `OK: ROM matches`.

I've tested this on my laptop.

I put this code up solely for educational purposes. Recently, decompilation projects have been popular amongst nonexpert engineers using AI.
I wanted to understand the strengths and limitations of using AI for decompilation projects. Additionally, this game is interesting
because of its use of the Nintendo VRU. I thought it might be interesting to see how the voice system works, and see if it would be
possible to extend it.

## Requirements
- nix
- A Hey You Pikachu ROM

## Quick start

```sh
# Supply your legally dumped ROM in rom/ (.z64/.v64/.n64).
; mkdir -p rom
; cp /path/to/your/dump.z64 rom/
# If flakes are enabled, you can skip the --extra-experimental-features flag.
; nix --extra-experimental-features 'nix-command flakes' develop --command bash
; python3 tools/rom.py rom/dump.z64 rom/baserom.us.z64
; python3 tools/materialize_ultralib.py --from /path/to/local-sdk-inputs
; uv run --locked make -j8  # Produces build/pikachu.z64 + SHA-1 check
; uv run --locked make verify-clean
```

Supports macOS and x86-64 Linux; tested on Apple Silicon. If flakes are already
enabled, the shell command is just `nix develop --command bash`.
Nix supplies IDO 5.3 and uv.

With direnv installed and hooked into your shell, run `direnv allow` once in the
repo. Entering the directory then loads Nix and the locked Python environment
automatically; you can skip `nix develop` and run `make -j8` directly.

## Overview

Effectively, the situation is like this.

```text
rom/baserom.us.z64 --(splat)--> build/asm/, build/assets/, build/pikachu.ld
build/asm/*.s     --(GNU as)--> build/asm/*.o
build/assets/*   --(GNU ld)--> build/assets/*.o
src/**/*.c       --(IDO 5.3 + asm-processor)--> build/src/**/*.o

build/**/*.o + linker/*.ld + build/pikachu.ld --(GNU ld)--> build/pikachu.elf
build/pikachu.elf --(objcopy)--> build/pikachu.z64 --(SHA-1)--> retail match
```

* **IDO 5.3** is the original 1998 SGI IRIX compiler, statically
  recompiled by [decompals/ido-static-recomp] to run natively on a modern machine.
* **asm-processor** lets one `.c` file mix matched C with
  `#pragma GLOBAL_ASM(...)` blocks, so code is decompiled one function
  at a time while the ROM keeps matching.
* **splat** produced the original split (`config/pikachu.yaml`,
  `config/symbol_addrs.txt`, `build/pikachu.ld`); re-run it only when re-splitting
  segments. Make regenerates the split when its inputs change.

Base split (yaml/symbols/asm layout) derives from
[farisawan-2000/hey_you_pikachu].

SDK-related headers in `include/` are local inputs excluded from version control;
SDK implementations are generated under ignored `build/asm/` from the supplied ROM.

## Status

The build reproduces the retail ROM byte for byte. Game functions remain in C,
with one game assembly fallback under `src/nonmatching/`

SDK implementations are excluded from the source tree and regenerated locally.

Run `make progress` for current game-source counts; generated SDK functions are excluded from those counts.

| region | state |
| ------ | ----- |
| header / IPL3 | extracted binary |
| boot segment, ROM 0x1000–0xA9800 | game C; SDK assembly generated locally |
| resident overlay, ROM 0xB3C70–0xDCF60 | split into 15 C units with owned rodata and bss |
| 23 scene overlays, ROM 0xDCF60–0x4B4990 | all text split; 11 scenes fully matched, all spatial lookups matched; scene data retained as binary |
| remaining assets | extracted binary; inventory research continues |

Agents should start with [docs/agents/AGENT_WORKFLOW.md](docs/agents/AGENT_WORKFLOW.md) for tool selection,
bounded searches, verification, and interruption recovery.

Every default build prints an assembly fallback summary. Run `make asm-fallbacks`
for function names and source paths. Machine-readable manifests remain in `docs/`.

## Reading the code

[Voice interaction and actor scripts](docs/VOICE_AND_SCRIPT_VM.md) traces recognition,
message delivery, cooperative VM execution, and the lower-level action sequencer.

## Undefined behaviour

Although it's easy to run UBSan to find UB with modern compilers, UB known/understood
by agents that did the porting work is documented in **[docs/UB.md]**.

## Notice

This project claims no rights to the original game, SDK, or compiler; those rights
remain with their respective owners. It provides reconstructed source code solely
for educational purposes. You must supply your own ROM and local SDK inputs.

[decompals/ido-static-recomp]: https://github.com/decompals/ido-static-recomp
[farisawan-2000/hey_you_pikachu]: https://github.com/farisawan-2000/hey_you_pikachu
[docs/UB.md]: docs/UB.md
