/// \file
/// Declares loading and cache preparation of prelinked game overlays.

#ifndef OVERLAY_LOADER_H
#define OVERLAY_LOADER_H

#include <ultra64.h>

/// Load a prelinked overlay from cartridge ROM and prepare it for execution.
/// Blocks until the entire overlay has been read. All end addresses are exclusive.
/// \param romStart Cartridge ROM address of the overlay's first byte.
/// \param romEnd Cartridge ROM address immediately after the overlay image.
/// \param destination RAM load address with room for the overlay image.
/// \param textStart First byte of the overlay executable RAM range.
/// \param textEnd End of the executable text range.
/// \param dataStart First byte of the overlay initialized-data RAM range.
/// \param dataEnd End of the data range.
/// \param bssStart First byte of the overlay zero-initialized RAM range.
/// \param bssEnd End of the zero-initialized range.
/// \returns Address after BSS rounded up to a 16-byte boundary.
///
/// \note NONPORTABLE: ROM offsets and aligned RAM addresses use the N64 32-bit address space.
/// \note Original decompilation identifier: \c overlayLoadFromRom (0x800135C0).
void *overlayLoadFromRom(u32 romStart, u32 romEnd, u8 *destination, u8 *textStart, u8 *textEnd, u8 *dataStart,
                         u8 *dataEnd, u8 *bssStart, u8 *bssEnd);

#endif
