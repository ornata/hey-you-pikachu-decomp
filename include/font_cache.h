/// \file
/// Provides synchronization and retention boundaries for cached font glyph data.

#ifndef FONT_CACHE_H
#define FONT_CACHE_H

/// Finish outstanding font reads before consuming cached glyph pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F6B8 (0x8003F6B8).
void fontCacheWaitForReads(void);

/// Finish font reads and release current glyph-cache uses.
/// \pre Drawing commands that reference the released glyphs have finished.
/// \returns Nothing; released glyph data may be reused by later cache requests.
/// \note Original decompilation identifier: \c func_8003F694 (0x8003F694).
void fontCacheReleaseUses(void);

#endif
