/// \file
/// Provides owned ROM-loaded audio banks with resolved playback references.

#ifndef AUDIO_BANK_H
#define AUDIO_BANK_H

#include <ultra64.h>

/// Load a bank file and resolve its references for playback.
/// \param heap Audio heap owning the returned bank file and its metadata.
/// \param romStart First cartridge byte of the bank file.
/// \param romEnd Cartridge address immediately following the bank file.
/// \param sampleBase Cartridge base of the associated sample data, which remains in ROM.
/// \returns Loaded bank file, valid until its audio heap is reset or released.
/// \note Portability: Bank references use the original 32-bit target address representation.
/// \note Original decompilation identifier: \c func_80020340 (0x80020340).
ALBankFile *audioBankLoad(ALHeap *heap, s32 romStart, s32 romEnd, s32 sampleBase);

#endif
