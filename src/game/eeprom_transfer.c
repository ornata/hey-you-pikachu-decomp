#include "common.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/* EEPROM transfer job (cmd/state machine driven by eepromUpdateTransfer) */
typedef struct EepromJob {
    s32 mode; /* 0 idle, 1 read, 2 write, 3..5 failure states */
    s32 retries;
    u8 page;
    u8 pad9;
    s16 nextPage;
    void *dst;
    s32 size;
    u8 *cur;
    s32 remaining;
    u8 buf[0x200];
} EepromJob;

/// Persistent EEPROM transfer state, payload staging buffer and backup-copy progress.
/// \note Original identifier: \c D_800AD1E0 (0x800AD1E0).
extern EepromJob gEepromTransferJob;
/// Cached EEPROM probe result used to enable supported transfers.
/// \note Original identifier: \c __osActiveQueue_2 (0x800AD3FC).
extern s32 gEepromDeviceType;
void _bzero(void *, u32);
s32 checksumCrc32(s32, void *);

/* queued request: dispatched by eepromDispatchRequest */
typedef struct EepromReq {
    s32 unk0;
    s32 cmd; /* 0 read, 1 write */
    s32 busy;
    s32 result;
    s32 page;
    s32 pageNext;
    void *buf;
    s32 size;
} EepromReq;

OSMesgQueue *serialGetMessageQueue();
s32 eepromIsTransferBusy();
typedef struct EepromArgs {
    s32 page;
    s32 pageNext;
    void *buf;
    s32 size;
} EepromArgs;
s32 eepromStartRead();
s32 eepromStartWrite();

/// Check whether the current EEPROM payload passes checksum validation.
/// \returns 1 when the checksums match, 0 otherwise.
/// \note Original decompilation identifier: \c eepromValidateStagedChecksum (0x800033D0).
s32 eepromValidateStagedChecksum(void) {
    s32 stored;
    s32 crc;

    crc = checksumCrc32(gEepromTransferJob.size, gEepromTransferJob.buf);
    bcopy(gEepromTransferJob.buf + gEepromTransferJob.size, &stored, 4);
    TRACE("loadCRC:%08x\nrealCRC:%08x\n\n", stored, crc);
    if (stored == crc) {
        return 1;
    }
    return 0;
}

/// Read one eight-byte EEPROM block and advance the current transfer.
/// Makes the payload available to the caller only after checksum validation.
/// \returns Transfer state: 0 complete, 1 pending, 3 terminal read failure,
/// or 4 checksum mismatch.
/// \note Original decompilation identifier: \c eepromReadNextBlock (0x80003438).
s32 eepromReadNextBlock(void) {
    if ((osEepromProbe(serialGetMessageQueue()) != 1) ||
        (osEepromRead(serialGetMessageQueue(), gEepromTransferJob.page, gEepromTransferJob.cur) != 0)) {
        if (++gEepromTransferJob.retries >= 0xB) {
            return 3;
        }
        return 1;
    }
    gEepromTransferJob.retries /= 2;
    gEepromTransferJob.remaining -= 8;
    if (gEepromTransferJob.remaining > 0) {
        gEepromTransferJob.page += 1;
        gEepromTransferJob.cur += 8;
        return 1;
    }
    if (eepromValidateStagedChecksum() != 0) {
        bcopy(gEepromTransferJob.buf, gEepromTransferJob.dst, gEepromTransferJob.size);
        return 0;
    }
    return 4;
}

/// Write one eight-byte EEPROM block and advance the current transfer.
/// \returns Transfer state: 0 complete, 2 pending, or 5 terminal write failure.
/// \note Original decompilation identifier: \c eepromWriteNextBlock (0x80003540).
s32 eepromWriteNextBlock(void) {
    if ((osEepromProbe(serialGetMessageQueue()) != 1) ||
        (osEepromWrite(serialGetMessageQueue(), gEepromTransferJob.page, gEepromTransferJob.cur) != 0)) {
        if (++gEepromTransferJob.retries >= 0xB) {
            return 5;
        }
        return 2;
    }
    gEepromTransferJob.retries /= 2;
    gEepromTransferJob.remaining -= 8;
    if (gEepromTransferJob.remaining > 0) {
        gEepromTransferJob.page += 1;
        gEepromTransferJob.cur += 8;
        return 2;
    }
    return 0;
}

/// Start an asynchronous EEPROM read with optional fallback-copy recovery.
/// \param page First eight-byte EEPROM page to read.
/// \param backupPage Fallback starting page on read/checksum failure, or a
/// negative signed 16-bit value to disable fallback.
/// \param destination Output buffer, kept valid until the transfer finishes.
/// \param byteCount Payload size, 0 through 508 bytes, excluding its checksum.
/// The rounded-up payload plus checksum must fit within the EEPROM page range.
/// \returns 0 if started, 3 if a 4-Kbit EEPROM is unavailable, or the current
/// transfer state if another transfer is busy. This does not wait for completion.
/// \note Original decompilation identifier: \c eepromStartRead (0x80003618).
s32 eepromStartRead(s32 page, s32 backupPage, void *destination, s32 byteCount) {
    TRACE("eepromRead(%d, %d, %p, %d)\n", page, backupPage, destination, byteCount);
    if (gEepromDeviceType != 1) {
        return 3;
    }
    if (eepromIsTransferBusy()) {
        return gEepromTransferJob.mode;
    }
    gEepromTransferJob.page = page;
    gEepromTransferJob.nextPage = backupPage;
    gEepromTransferJob.dst = destination;
    gEepromTransferJob.size = byteCount;
    gEepromTransferJob.cur = gEepromTransferJob.buf;
    gEepromTransferJob.remaining = gEepromTransferJob.size + 4;
    gEepromTransferJob.mode = 1;
    gEepromTransferJob.retries = 0;
    return 0;
}

/// Start an asynchronous EEPROM write with checksum protection.
/// \param page First eight-byte EEPROM page to write.
/// \param backupPage Second copy's starting page, or a negative value to disable
/// the second copy; the value must fit in a signed 16-bit integer.
/// \param source Payload to write; may be reused after this call returns.
/// \param byteCount Payload size, 0 through 508 bytes, excluding its checksum.
/// The rounded-up payload plus checksum must fit within the EEPROM page range.
/// \returns 0 if started, 5 if a 4-Kbit EEPROM is unavailable, or the current
/// transfer state if another transfer is busy. This does not wait for completion.
/// \note Original decompilation identifier: \c eepromStartWrite (0x800036C0).
s32 eepromStartWrite(s32 page, s32 backupPage, void *source, s32 byteCount) {
    s32 crc;
    TRACE("eepromWrite(%d, %d, %p, %d)\n", page, backupPage, source, byteCount);
    if (gEepromDeviceType != 1) {
        return 5;
    }
    if (eepromIsTransferBusy()) {
        return gEepromTransferJob.mode;
    }
    bcopy(source, gEepromTransferJob.buf, byteCount);
    crc = checksumCrc32(byteCount, source);
    TRACE("calcCRC:%08x\n", crc);
    bcopy(&crc, gEepromTransferJob.buf + byteCount, 4);
    gEepromTransferJob.page = page;
    gEepromTransferJob.nextPage = backupPage;
    gEepromTransferJob.dst = source;
    gEepromTransferJob.size = byteCount;
    gEepromTransferJob.cur = gEepromTransferJob.buf;
    gEepromTransferJob.remaining = gEepromTransferJob.size + 4;
    gEepromTransferJob.mode = 2;
    gEepromTransferJob.retries = 0;
    return 0;
}

/// Probe the EEPROM device and reset the shared transfer state to idle.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c eepromInitializeTransfers (0x800037AC).
void eepromInitializeTransfers(void) {
    gEepromDeviceType = osEepromProbe(serialGetMessageQueue());
    _bzero(&gEepromTransferJob, 0x21C);
    gEepromTransferJob.mode = 0;
}

/// Advance the current EEPROM transfer by one block and handle backup copies.
/// Read failures retry the backup copy; writes attempt the second copy after
/// either completion or failure of the first copy. Only 4-Kbit EEPROM is used.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c eepromUpdateTransfer (0x800037F0).
void eepromUpdateTransfer(void) {
    if (gEepromDeviceType == 1) {
        switch (gEepromTransferJob.mode) {
        case 1:
            gEepromTransferJob.mode = eepromReadNextBlock();
            if ((gEepromTransferJob.mode == 3) || (gEepromTransferJob.mode == 4)) {
                if (gEepromTransferJob.nextPage >= 0) {
                    gEepromTransferJob.page = gEepromTransferJob.nextPage;
                    gEepromTransferJob.nextPage = -2;
                    gEepromTransferJob.cur = gEepromTransferJob.buf;
                    gEepromTransferJob.remaining = gEepromTransferJob.size + 4;
                    gEepromTransferJob.mode = 1;
                    gEepromTransferJob.retries = 0;
                }
            }
            break;
        case 2:
            gEepromTransferJob.mode = eepromWriteNextBlock();
            if ((gEepromTransferJob.mode == 5) || (gEepromTransferJob.mode == 0)) {
                if (gEepromTransferJob.nextPage < 0) {
                    if ((gEepromTransferJob.mode == 5) && (gEepromTransferJob.nextPage == -2)) {
                        gEepromTransferJob.mode = 5;
                    } else {
                        gEepromTransferJob.mode = 0;
                    }
                } else {
                    gEepromTransferJob.page = gEepromTransferJob.nextPage;
                    if (gEepromTransferJob.mode == 5) {
                        gEepromTransferJob.nextPage = -2;
                    } else {
                        gEepromTransferJob.nextPage = -1;
                    }
                    gEepromTransferJob.cur = gEepromTransferJob.buf;
                    gEepromTransferJob.remaining = gEepromTransferJob.size + 4;
                    gEepromTransferJob.mode = 2;
                    gEepromTransferJob.retries = 0;
                }
            }
            break;
        case 0:
        case 3:
        case 4:
        case 5:
            break;
        default:
            TRACE("Unknown eeprom state(%d)\n", gEepromTransferJob.mode);
            break;
        }
    }
}

/// Dispatch a queued EEPROM read/write setup request on the controller thread.
/// \param req Request receiving the setup result. Its busy flag is cleared
/// after dispatch, not after completion of the underlying EEPROM transfer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c eepromDispatchRequest (0x80003938).
void eepromDispatchRequest(EepromReq *req) {
    switch (req->cmd) {
    case 0:
        req->result = eepromStartRead(req->page, req->pageNext, req->buf, req->size);
        break;
    case 1:
        req->result = eepromStartWrite(req->page, req->pageNext, req->buf, req->size);
        break;
    default:
        TRACE("ERROR: Unknown command:%d\n", req->cmd);
        break;
    }
    req->busy = 0;
}

/// Query the current EEPROM transfer state.
/// \returns 0 when idle or a 4-Kbit EEPROM is unavailable, 1/2 for a pending
/// read/write, or 3/4/5 for read/checksum/write failure.
/// \note Original decompilation identifier: \c eepromGetTransferState (0x800039B4).
s32 eepromGetTransferState(void) {
    if (gEepromDeviceType != 1) {
        return 0;
    }
    return gEepromTransferJob.mode;
}

/// Query the EEPROM type recorded by the initialization probe.
/// \returns SDK probe result: 0 absent, 1 for 4 Kbit, or 2 for 16 Kbit.
/// \note Original decompilation identifier: \c eepromGetDeviceType (0x800039E0).
s32 eepromGetDeviceType(void) {
    return gEepromDeviceType;
}

s32 eepromGetTransferState(); /* extern */

/// Test whether the EEPROM transfer state is nonterminal.
/// \returns 1 while a transfer is busy, 0 when idle, unavailable, or failed.
/// \note Original decompilation identifier: \c eepromIsTransferBusy (0x800039EC).
s32 eepromIsTransferBusy(void) {
    s32 transferState;

    transferState = eepromGetTransferState();
    if ((transferState != 0) && (transferState != 3) && (transferState != 4) && (transferState != 5)) {
        return 1;
    }
    return 0;
}
