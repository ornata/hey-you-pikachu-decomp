/* Recovered from the ROM image by disassembly; unit game/unused_absolute_value. */
/* The unreachable remainder (src/nonmatching/) is the ROM's own code, unmodified. */

#include "common.h"

/* Hand-written fabs assembled in reorder mode: the assembler filled the JR
 * delay slot with a NOP, so the entry returns at once. The ABS.D that follows
 * is never reached (no caller jumps to +8). */
void func_80048F60(void) {
}

#pragma GLOBAL_ASM("src/nonmatching/game/unused_absolute_value/unreachable_abs.s")
