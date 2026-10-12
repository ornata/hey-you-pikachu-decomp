/// \file
/// Provides common array and compiler-attribute helpers for game code.

#ifndef __MACROS_H__
#define __MACROS_H__

/* IDO's cfe has no __attribute__; alignment is advisory there and the
 * linker script is what actually places aligned objects. */
#ifndef __GNUC__
#define __attribute__(x)
#endif

#define ALIGNED(x) __attribute__((aligned(x)))

#define ARRLEN(x) ((s32)(sizeof(x) / sizeof(x[0])))

#endif
