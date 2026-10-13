#include "audio_bank.h"
#include "rom_dma.h"
#include "common.h"

/// Target offsets of the variable-length audio records consumed by relocation.
#define AUDIO_BANK_OFFSET_CHECK(type, field, offset) \
    typedef char type##_##field##OffsetCheck[(((u32) & ((type *)0)->field) == (offset)) ? 1 : -1]
AUDIO_BANK_OFFSET_CHECK(ALBank, instArray, 0xC);
AUDIO_BANK_OFFSET_CHECK(ALInstrument, soundCount, 0xE);
AUDIO_BANK_OFFSET_CHECK(ALInstrument, soundArray, 0x10);
AUDIO_BANK_OFFSET_CHECK(ALSound, envelope, 0);
AUDIO_BANK_OFFSET_CHECK(ALSound, keyMap, 4);
AUDIO_BANK_OFFSET_CHECK(ALSound, wavetable, 8);
AUDIO_BANK_OFFSET_CHECK(ALWaveTable, base, 0);
AUDIO_BANK_OFFSET_CHECK(ALWaveTable, type, 8);
AUDIO_BANK_OFFSET_CHECK(ALWaveTable, waveInfo, 0xC);
#undef AUDIO_BANK_OFFSET_CHECK

void audioBankFileRelocate(ALBankFile *, s32);

/// Load an audio bank file from ROM and prepare it for playback.
/// \param heap Audio heap receiving the loaded bank file.
/// \param romStart First ROM byte of the bank file.
/// \param romEnd ROM address immediately following the bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns The loaded bank-file buffer.
/// \note Original decompilation identifier: \c func_80020340 (0x80020340).
ALBankFile *audioBankLoad(ALHeap *heap, s32 romStart, s32 romEnd, s32 sampleBase) {
    s32 len;
    u8 *buf;

    len = romEnd - romStart;
    buf = alHeapDBAlloc(NULL, 0, heap, 1, len);
    romDmaReadBlocking(romStart, buf, len);
    audioBankFileRelocate((ALBankFile *)buf, sampleBase);
    return (ALBankFile *)buf;
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

void audioBankRelocate(ALBank *, s32, s32);

/// Prepare all banks in a newly loaded audio bank file for playback.
/// \param file Loaded bank file whose bank entries have not yet been relocated.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800203F4 (0x800203F4).
void audioBankFileRelocate(ALBankFile *file, s32 sampleBase) {
    s32 i;

    for (i = 0; i < file->bankCount; i++) {
        file->bankArray[i] = (ALBank *)((u8 *)file->bankArray[i] + (s32)file);
        audioBankRelocate(file->bankArray[i], (s32)file, sampleBase);
    }
}

void audioInstrumentRelocate(ALInstrument *, s32, s32);

/// Resolve a bank's instruments and their sound resources.
/// \param bank Bank within the loaded bank file.
/// \param bankBase Memory base address of the loaded bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Portability: instrumentCursor is a byte cursor biased by the bank-header
/// size. Only its instrument-array member is accessed after advancing; this view
/// preserves IDO's matching address calculation for the variable-length bank.
/// \note Original decompilation identifier: \c func_80020470 (0x80020470).
void audioBankRelocate(ALBank *bank, s32 bankBase, s32 sampleBase) {
    s32 percussionAddress;
    s32 instrumentAddress;
    s32 percussionOffset;
    s32 instrumentOffset;
    s32 instrumentIndex;
    u8 *instrumentCursor;

    percussionOffset = (s32)bank->percussion;
    if (((percussionOffset == 0) || !(percussionOffset & 0x80000000)) &&
        ((bank->instCount == 0) || !((s32)bank->instArray[0] & 0x80000000))) {
        percussionAddress = percussionOffset + bankBase;
        if (percussionOffset != 0) {
            bank->percussion = (ALInstrument *)percussionAddress;
            audioInstrumentRelocate((ALInstrument *)percussionAddress, bankBase, sampleBase);
        }
        instrumentIndex = 0;
        if (bank->instCount > 0) {
            instrumentCursor = (u8 *)bank;
            do {
                instrumentOffset = (s32)((ALBank *)instrumentCursor)->instArray[0];
                instrumentAddress = instrumentOffset + bankBase;
                if (instrumentOffset != 0) {
                    ((ALBank *)instrumentCursor)->instArray[0] = (ALInstrument *)instrumentAddress;
                    audioInstrumentRelocate((ALInstrument *)instrumentAddress, bankBase, sampleBase);
                }
                instrumentIndex += 1;
                instrumentCursor += sizeof(ALInstrument *);
            } while (instrumentIndex < bank->instCount);
        }
    }
}

void audioSoundRelocate(ALSound *, u32, s32);

/// Resolve an instrument's sound definitions for playback.
/// \param instrument Instrument within the loaded bank file.
/// \param bankBase Memory base address of the loaded bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80020540 (0x80020540).
void audioInstrumentRelocate(ALInstrument *instrument, s32 bankBase, s32 sampleBase) {
    s32 i;

    if (instrument->soundCount == 0 || !((u32)instrument->soundArray[0] & 0x80000000)) {
        for (i = 0; i < instrument->soundCount; i++) {
            instrument->soundArray[i] = (ALSound *)((s32)instrument->soundArray[i] + bankBase);
            audioSoundRelocate(instrument->soundArray[i], bankBase, sampleBase);
        }
    }
}

void audioWaveTableRelocate(ALWaveTable *, s32, s32);
/// Resolve a sound's envelope, key map, and sample description.
/// \param sound Sound definition within the loaded bank file.
/// \param bankBase Memory base address of the loaded bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800205DC (0x800205DC).
void audioSoundRelocate(ALSound *sound, u32 bankBase, s32 sampleBase) {
    if (!((u32)sound->envelope & 0x80000000)) {
        sound->envelope = (ALEnvelope *)((u32)sound->envelope + bankBase);
        sound->keyMap = (ALKeyMap *)((u32)sound->keyMap + bankBase);
        sound->wavetable = (ALWaveTable *)((u32)sound->wavetable + bankBase);
        audioWaveTableRelocate(sound->wavetable, (s32)bankBase, sampleBase);
    }
}

/// Prepare an ADPCM or raw sample description for playback.
/// \param waveTable Sample description within the loaded bank file.
/// \param bankBase Memory base address of the loaded bank file.
/// \param sampleBase ROM base address of the associated sample data.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80020628 (0x80020628).
void audioWaveTableRelocate(ALWaveTable *waveTable, s32 bankBase, s32 sampleBase) {
    if (waveTable->type == AL_ADPCM_WAVE) {
        if (!((u32)waveTable->waveInfo.adpcmWave.book & 0x80000000)) {
            waveTable->base = (u8 *)((s32)waveTable->base + sampleBase);
            waveTable->waveInfo.adpcmWave.book = (ALADPCMBook *)((s32)waveTable->waveInfo.adpcmWave.book + bankBase);
            if (waveTable->waveInfo.adpcmWave.loop != 0) {
                waveTable->waveInfo.adpcmWave.loop = (ALADPCMloop *)((s32)waveTable->waveInfo.adpcmWave.loop + bankBase);
            }
        }
    } else if (waveTable->type == AL_RAW16_WAVE) {
        if (!((u32)waveTable->waveInfo.rawWave.loop & 0x80000000)) {
            waveTable->base = (u8 *)((s32)waveTable->base + sampleBase);
            if (waveTable->waveInfo.rawWave.loop != 0) {
                waveTable->waveInfo.rawWave.loop = (ALRawLoop *)((s32)waveTable->waveInfo.rawWave.loop + bankBase);
            }
        }
    }
}
