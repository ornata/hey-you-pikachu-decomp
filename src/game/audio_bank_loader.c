/* harvested: matched m2c drafts; pending style pass */
#include "common.h"

void audioBankFileRelocate(ALBankFile *, s32);
void romDmaReadBlocking(s32 romAddress, u8 *destination, s32 byteCount);

/// Load an audio bank file from ROM and prepare it for playback.
/// \param heap Audio heap receiving the loaded bank file.
/// \param romStart First ROM byte of the bank file.
/// \param romEnd ROM address immediately following the bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns The loaded bank-file buffer.
/// \note Original decompilation identifier: \c func_80020340 (0x80020340).
u8 *audioBankLoad(ALHeap *heap, s32 romStart, s32 romEnd, s32 sampleBase) {
    s32 len;
    u8 *buf;

    len = romEnd - romStart;
    buf = alHeapDBAlloc(NULL, 0, heap, 1, len);
    romDmaReadBlocking(romStart, buf, len);
    audioBankFileRelocate((ALBankFile *)buf, sampleBase);
    return buf;
}

/// Resolve sequence-file entries relative to their sequence data base.
/// \param file Sequence file whose entries still contain relative offsets.
/// \param base Base address of the sequence data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c alSeqFileNew (0x800203B4).
void alSeqFileNew(ALSeqFile *file, u8 *base) {
    s32 i;

    for (i = 0; i < file->seqCount; i++) {
        file->seqArray[i].offset = (u8 *)((s32)file->seqArray[i].offset + (s32)base);
    }
}

void audioBankRelocate(u8 *, s32, s32);

/// Prepare all banks in a newly loaded audio bank file for playback.
/// \param file Loaded bank file whose bank entries have not yet been relocated.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800203F4 (0x800203F4).
void audioBankFileRelocate(ALBankFile *file, s32 sampleBase) {
    s32 i;

    for (i = 0; i < file->bankCount; i++) {
        file->bankArray[i] = (ALBank *)((u8 *)file->bankArray[i] + (s32)file);
        audioBankRelocate((u8 *)file->bankArray[i], (s32)file, sampleBase);
    }
}

void audioInstrumentRelocate(s32, s32, s32);

/// Resolve a bank's instruments and their sound resources.
/// \param bank Bank within the loaded bank file.
/// \param bankBase Memory base address of the loaded bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Portability: instrumentCursor is a byte cursor biased by the bank-header
/// size. Only its instrument-array member is accessed after advancing; this view
/// preserves IDO's matching address calculation for the variable-length bank.
/// \note Original decompilation identifier: \c func_80020470 (0x80020470).
void audioBankRelocate(u8 *bank, s32 bankBase, s32 sampleBase) {
    s32 percussionAddress;
    s32 instrumentAddress;
    s32 percussionOffset;
    s32 instrumentOffset;
    s32 var_s1;
    u8 *instrumentCursor;

    percussionOffset = (s32)((ALBank *)bank)->percussion;
    if (((percussionOffset == 0) || !(percussionOffset & 0x80000000)) &&
        ((((ALBank *)bank)->instCount == 0) || !((s32)((ALBank *)bank)->instArray[0] & 0x80000000))) {
        percussionAddress = percussionOffset + bankBase;
        if (percussionOffset != 0) {
            ((ALBank *)bank)->percussion = (ALInstrument *)percussionAddress;
            audioInstrumentRelocate(percussionAddress, bankBase, sampleBase);
        }
        var_s1 = 0;
        if (((ALBank *)bank)->instCount > 0) {
            instrumentCursor = bank;
            do {
                instrumentOffset = (s32)((ALBank *)instrumentCursor)->instArray[0];
                instrumentAddress = instrumentOffset + bankBase;
                if (instrumentOffset != 0) {
                    ((ALBank *)instrumentCursor)->instArray[0] = (ALInstrument *)instrumentAddress;
                    audioInstrumentRelocate(instrumentAddress, bankBase, sampleBase);
                }
                var_s1 += 1;
                instrumentCursor += sizeof(ALInstrument *);
            } while (var_s1 < ((ALBank *)bank)->instCount);
        }
    }
}

typedef struct {
    u32 a, b, c;
} S205DC;
void audioSoundRelocate(S205DC *, u32, s32);

/// Resolve an instrument's sound definitions for playback.
/// \param instrument Address of the instrument within the loaded bank file.
/// \param bankBase Memory base address of the loaded bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80020540 (0x80020540).
void audioInstrumentRelocate(s32 instrument, s32 bankBase, s32 sampleBase) {
    s32 i;

    if (*(s16 *)(instrument + 0xE) == 0 || !(*(u32 *)(instrument + 0x10) & 0x80000000)) {
        for (i = 0; i < *(s16 *)(instrument + 0xE); i++) {
            ((s32 *)(instrument + 0x10))[i] += bankBase;
            audioSoundRelocate((S205DC *)((s32 *)(instrument + 0x10))[i], bankBase, sampleBase);
        }
    }
}

typedef struct Seg20628 Seg20628;
void audioWaveTableRelocate(Seg20628 *, s32, s32);
/// Resolve a sound's envelope, key map, and sample description.
/// \param sound Sound definition within the loaded bank file.
/// \param bankBase Memory base address of the loaded bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800205DC (0x800205DC).
void audioSoundRelocate(S205DC *sound, u32 bankBase, s32 sampleBase) {
    if (!(sound->a & 0x80000000)) {
        sound->a += bankBase;
        sound->b += bankBase;
        sound->c += bankBase;
        audioWaveTableRelocate((Seg20628 *)sound->c, (s32)bankBase, sampleBase);
    }
}

struct Seg20628 {
    s32 pos;
    s32 unk4;
    u8 mode;
    s32 end;
    s32 start;
};

/// Prepare an ADPCM or raw sample description for playback.
/// \param waveTable Sample description within the loaded bank file.
/// \param bankBase Memory base address of the loaded bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80020628 (0x80020628).
void audioWaveTableRelocate(Seg20628 *waveTable, s32 bankBase, s32 sampleBase) {
    if (waveTable->mode == 0) {
        if (!((u32)waveTable->start & 0x80000000)) {
            waveTable->pos += sampleBase;
            waveTable->start += bankBase;
            if (waveTable->end != 0) {
                waveTable->end += bankBase;
            }
        }
    } else if (waveTable->mode == 1) {
        if (!((u32)waveTable->end & 0x80000000)) {
            waveTable->pos += sampleBase;
            if (waveTable->end != 0) {
                waveTable->end += bankBase;
            }
        }
    }
}
