/* Rodata strings referenced from asm .data that now live inside a C string
 * literal (no compiler-visible symbol of their own). */
/* "\xA1\xA9" x3 placeholder name, emitted after playerPrepareInteractionDescription's 0x87
 * out-of-reach tail; referenced by D_80123EA0. */
D_8012C50C = 0x8012C50C;

/* SDK data labels retained when their implementations are generated locally. */
data_getRate = 0x8009FAB0;
jtbl_envPull = 0x800A8040;
rodata_getRate = 0x800A8088;

/* Original: D_800A01C0. Controller input diagnostic string. */
gControllerInputTraceFormat = 0x800A01C0;
