# Undefined Behavior & Portability Ledger

```
      !  ---------------------------------------------------  !
      !  THIS CODEBASE TARGETS EXACTLY ONE COMPILER: IDO 5.3  !
      !  Matched C is a *witness* for the ROM's machine code, !
      !  not portable ISO C. Retarget at your own risk.       !
      !  ---------------------------------------------------  !
```

A matching decompilation inverts the usual contract of C. Normally you
write standard C and any conforming compiler may run it; here the
machine code is fixed (the retail ROM) and the C exists to make one
specific compiler — SGI IDO 5.3, `-O2 -mips2 -32 -G0` — emit those
exact bytes. Where the original programmers (or the SDK, or the
compiler's own idioms) relied on undefined, unspecified, or
implementation-defined behavior, we must keep that reliance to match.

## Annotation convention

Markers appear in source at the site of the hazard:

| marker          | meaning                                                      |
| --------------- | ------------------------------------------------------------ |
| `UB:`           | undefined behavior per ISO C; a different compiler/optimizer may do anything, including deleting the code |
| `NONPORTABLE:`  | implementation-defined or ABI/hardware-specific behavior; well-defined on IDO + MIPS o32 + N64, different elsewhere |

ASan/UBSan findings, if the code is ever compiled for a modern host,
should be recorded here and at the site with the sanitizer name.

## Project-wide hazards (apply everywhere, not re-annotated per site)

1. **Hardcoded address constants.** BSS, stacks, hardware registers
   (`0x800A9000`, `0xA4600010`, ...) are produced by casting integers
   to pointers. Implementation-defined (C90 6.3.4), and on any hosted
   platform those dereferences are wild pointers. Under ASan every one
   of these is an invalid access; on N64 they are the memory map.

2. **Signed integer overflow.** IDO compiles with 32-bit two's
   complement wraparound and the game logic assumes it. ISO C calls
   signed overflow undefined; clang/gcc at `-O2` will optimize on the
   assumption it cannot happen (e.g. delete `x + 1 < x` checks).

3. **Type punning through unions and pointer casts.** Locally supplied
   SDK headers and game code read float bits as integers, treat message queues
   as untyped word stores, and alias structs. Violates C99+ strict aliasing; IDO has no strict-aliasing
   optimizer so it is safe *there*. A modern compiler needs
   `-fno-strict-aliasing` at minimum.

4. **Intentional infinite loops** (`for (;;);` in idle threads,
   `while (TRUE);` hangs). Defined in C90, and *also* defined in C11:
   6.8.5p6 only lets an implementation assume termination when the
   controlling expression is not a constant expression, and `for (;;)`
   / `while (1)` are exempt. What is nonportable is the codegen: IDO
   emits the loop plus a dead function epilogue after it, and those
   unreachable bytes are in the retail ROM. See `IdleMain` in
   `src/main.c`.

5. **No C runtime.** `main` returns `void`, is called from hand-written
   assembly (`build/asm/entry.s`) with BSS cleared manually and a stack
   pointer picked from a linker symbol. Freestanding, not hosted.

6. **Stores below the stack pointer.** IDO "homes" unused incoming
   arguments to the caller's argument-save area even in leaf functions
   with no frame (`sw a0, 0(sp)` — see `n_alSynFreeFX`). Legal only
   because the o32 ABI reserves that area and nothing on the N64
   asynchronously clobbers it (interrupt handlers switch stacks).

7. **Uninitialized reads that match.** Some functions only match when a
   variable is declared but not initialized on every path, because that
   is what the original code did. These are genuine latent bugs in the
   shipped game, preserved intentionally; each gets a site `UB:` marker.

## Site-specific constraints

Current function Doxygen comments record local portability and matching notes.
Reviewed interface exceptions are maintained in `INTERFACE_CONTRACTS.json`;
layout and expression constraints are maintained in `TYPE_RECOVERY_NOTES.json`.
Use those source locations and ledgers rather than historical matching-wave lists.
