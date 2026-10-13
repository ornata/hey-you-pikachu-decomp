#include "serial_queue.h"
#include "common.h"
#include "eeprom_io.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Private controller-thread transfer state and owned checksum-protected staging storage.
typedef struct EepromJob {
    s32 state; ///< EepromTransferState retained as a signed target word.
    s32 retries; ///< Accumulated transfer failures, reduced after successful blocks.
    u8 page; ///< Current eight-byte EEPROM page.
    u8 padding09;
    s16 backupPage; ///< Alternate starting page, or a negative backup-progress marker.
    void *payload; ///< Borrowed payload pointer; read destinations remain valid until completion.
    s32 payloadBytes; ///< Payload length excluding its four-byte checksum.
    u8 *cursor; ///< Current block within owned staging storage.
    s32 remainingBytes; ///< Payload and checksum bytes still pending.
    u8 staging[0x200]; ///< Owned transfer payload and checksum storage.
} EepromJob;

typedef char EepromJobSizeCheck[(sizeof(EepromJob) == 0x21C) ? 1 : -1];
typedef char EepromJobStagingOffsetCheck[((u32)&((EepromJob *)0)->staging == 0x1C) ? 1 : -1];
typedef char EepromJobPayloadOffsetCheck[((u32)&((EepromJob *)0)->payload == 0xC) ? 1 : -1];

/// Persistent EEPROM transfer state, payload staging buffer and backup-copy progress.
/// \note Original identifier: \c D_800AD1E0 (0x800AD1E0).
extern EepromJob gEepromTransferJob;
/// Cached EEPROM probe result used to enable supported transfers.
/// \note Original identifier: \c __osActiveQueue_2 (0x800AD3FC).
extern s32 gEepromDeviceType;
void _bzero(void *, u32);
s32 checksumCrc32(s32, void *);



/// Check whether the current EEPROM payload passes checksum validation.
/// \returns 1 when the checksums match, 0 otherwise.
/// \note Original decompilation identifier: \c eepromValidateStagedChecksum (0x800033D0).
s32 eepromValidateStagedChecksum(void) {
    s32 stored;
    s32 crc;

    crc = checksumCrc32(gEepromTransferJob.payloadBytes, gEepromTransferJob.staging);
    bcopy(gEepromTransferJob.staging + gEepromTransferJob.payloadBytes, &stored, 4);
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
        (osEepromRead(serialGetMessageQueue(), gEepromTransferJob.page, gEepromTransferJob.cursor) != 0)) {
        if (++gEepromTransferJob.retries >= 0xB) {
            return EEPROM_TRANSFER_READ_FAILED;
        }
        return EEPROM_TRANSFER_READING;
    }
    gEepromTransferJob.retries /= 2;
    gEepromTransferJob.remainingBytes -= 8;
    if (gEepromTransferJob.remainingBytes > 0) {
        gEepromTransferJob.page += 1;
        gEepromTransferJob.cursor += 8;
        return EEPROM_TRANSFER_READING;
    }
    if (eepromValidateStagedChecksum() != 0) {
        bcopy(gEepromTransferJob.staging, gEepromTransferJob.payload, gEepromTransferJob.payloadBytes);
        return EEPROM_TRANSFER_IDLE;
    }
    return EEPROM_TRANSFER_CHECKSUM_FAILED;
}

/// Write one eight-byte EEPROM block and advance the current transfer.
/// \returns Transfer state: 0 complete, 2 pending, or 5 terminal write failure.
/// \note Original decompilation identifier: \c eepromWriteNextBlock (0x80003540).
s32 eepromWriteNextBlock(void) {
    if ((osEepromProbe(serialGetMessageQueue()) != 1) ||
        (osEepromWrite(serialGetMessageQueue(), gEepromTransferJob.page, gEepromTransferJob.cursor) != 0)) {
        if (++gEepromTransferJob.retries >= 0xB) {
            return EEPROM_TRANSFER_WRITE_FAILED;
        }
        return EEPROM_TRANSFER_WRITING;
    }
    gEepromTransferJob.retries /= 2;
    gEepromTransferJob.remainingBytes -= 8;
    if (gEepromTransferJob.remainingBytes > 0) {
        gEepromTransferJob.page += 1;
        gEepromTransferJob.cursor += 8;
        return EEPROM_TRANSFER_WRITING;
    }
    return EEPROM_TRANSFER_IDLE;
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
        return EEPROM_TRANSFER_READ_FAILED;
    }
    if (eepromIsTransferBusy()) {
        return gEepromTransferJob.state;
    }
    gEepromTransferJob.page = page;
    gEepromTransferJob.backupPage = backupPage;
    gEepromTransferJob.payload = destination;
    gEepromTransferJob.payloadBytes = byteCount;
    gEepromTransferJob.cursor = gEepromTransferJob.staging;
    gEepromTransferJob.remainingBytes = gEepromTransferJob.payloadBytes + 4;
    gEepromTransferJob.state = EEPROM_TRANSFER_READING;
    gEepromTransferJob.retries = 0;
    return EEPROM_TRANSFER_IDLE;
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
        return EEPROM_TRANSFER_WRITE_FAILED;
    }
    if (eepromIsTransferBusy()) {
        return gEepromTransferJob.state;
    }
    bcopy(source, gEepromTransferJob.staging, byteCount);
    crc = checksumCrc32(byteCount, source);
    TRACE("calcCRC:%08x\n", crc);
    bcopy(&crc, gEepromTransferJob.staging + byteCount, 4);
    gEepromTransferJob.page = page;
    gEepromTransferJob.backupPage = backupPage;
    gEepromTransferJob.payload = source;
    gEepromTransferJob.payloadBytes = byteCount;
    gEepromTransferJob.cursor = gEepromTransferJob.staging;
    gEepromTransferJob.remainingBytes = gEepromTransferJob.payloadBytes + 4;
    gEepromTransferJob.state = EEPROM_TRANSFER_WRITING;
    gEepromTransferJob.retries = 0;
    return EEPROM_TRANSFER_IDLE;
}

/// Probe the EEPROM device and reset the shared transfer state to idle.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c eepromInitializeTransfers (0x800037AC).
void eepromInitializeTransfers(void) {
    gEepromDeviceType = osEepromProbe(serialGetMessageQueue());
    _bzero(&gEepromTransferJob, 0x21C);
    gEepromTransferJob.state = EEPROM_TRANSFER_IDLE;
}

/// Advance the current EEPROM transfer by one block and handle backup copies.
/// Read failures retry the backup copy; writes attempt the second copy after
/// either completion or failure of the first copy. Only 4-Kbit EEPROM is used.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c eepromUpdateTransfer (0x800037F0).
void eepromUpdateTransfer(void) {
    if (gEepromDeviceType == 1) {
        switch (gEepromTransferJob.state) {
        case EEPROM_TRANSFER_READING:
            gEepromTransferJob.state = eepromReadNextBlock();
            if ((gEepromTransferJob.state == EEPROM_TRANSFER_READ_FAILED) || (gEepromTransferJob.state == EEPROM_TRANSFER_CHECKSUM_FAILED)) {
                if (gEepromTransferJob.backupPage >= 0) {
                    gEepromTransferJob.page = gEepromTransferJob.backupPage;
                    gEepromTransferJob.backupPage = -2;
                    gEepromTransferJob.cursor = gEepromTransferJob.staging;
                    gEepromTransferJob.remainingBytes = gEepromTransferJob.payloadBytes + 4;
                    gEepromTransferJob.state = EEPROM_TRANSFER_READING;
                    gEepromTransferJob.retries = 0;
                }
            }
            break;
        case EEPROM_TRANSFER_WRITING:
            gEepromTransferJob.state = eepromWriteNextBlock();
            if ((gEepromTransferJob.state == EEPROM_TRANSFER_WRITE_FAILED) || (gEepromTransferJob.state == EEPROM_TRANSFER_IDLE)) {
                if (gEepromTransferJob.backupPage < 0) {
                    if ((gEepromTransferJob.state == EEPROM_TRANSFER_WRITE_FAILED) && (gEepromTransferJob.backupPage == -2)) {
                        gEepromTransferJob.state = EEPROM_TRANSFER_WRITE_FAILED;
                    } else {
                        gEepromTransferJob.state = EEPROM_TRANSFER_IDLE;
                    }
                } else {
                    gEepromTransferJob.page = gEepromTransferJob.backupPage;
                    if (gEepromTransferJob.state == EEPROM_TRANSFER_WRITE_FAILED) {
                        gEepromTransferJob.backupPage = -2;
                    } else {
                        gEepromTransferJob.backupPage = -1;
                    }
                    gEepromTransferJob.cursor = gEepromTransferJob.staging;
                    gEepromTransferJob.remainingBytes = gEepromTransferJob.payloadBytes + 4;
                    gEepromTransferJob.state = EEPROM_TRANSFER_WRITING;
                    gEepromTransferJob.retries = 0;
                }
            }
            break;
        case EEPROM_TRANSFER_IDLE:
        case EEPROM_TRANSFER_READ_FAILED:
        case EEPROM_TRANSFER_CHECKSUM_FAILED:
        case EEPROM_TRANSFER_WRITE_FAILED:
            break;
        default:
            TRACE("Unknown eeprom state(%d)\n", gEepromTransferJob.state);
            break;
        }
    }
}

/// Dispatch a queued EEPROM read/write setup request on the controller thread.
/// \param req Request receiving the setup result. Its busy flag is cleared
/// after dispatch, not after completion of the underlying EEPROM transfer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c eepromDispatchRequest (0x80003938).
void eepromDispatchRequest(EepromRequest *req) {
    switch (req->command) {
    case 0:
        req->result = eepromStartRead(req->page, req->backupPage, req->buffer, req->byteCount);
        break;
    case 1:
        req->result = eepromStartWrite(req->page, req->backupPage, req->buffer, req->byteCount);
        break;
    default:
        TRACE("ERROR: Unknown command:%d\n", req->command);
        break;
    }
    req->pending = 0;
}

/// Query the current EEPROM transfer state.
/// \returns 0 when idle or a 4-Kbit EEPROM is unavailable, 1/2 for a pending
/// read/write, or 3/4/5 for read/checksum/write failure.
/// \note Original decompilation identifier: \c eepromGetTransferState (0x800039B4).
s32 eepromGetTransferState(void) {
    if (gEepromDeviceType != 1) {
        return 0;
    }
    return gEepromTransferJob.state;
}

/// Query the EEPROM type recorded by the initialization probe.
/// \returns SDK probe result: 0 absent, 1 for 4 Kbit, or 2 for 16 Kbit.
/// \note Original decompilation identifier: \c eepromGetDeviceType (0x800039E0).
s32 eepromGetDeviceType(void) {
    return gEepromDeviceType;
}


/// Test whether the EEPROM transfer state is nonterminal.
/// \returns 1 while a transfer is busy, 0 when idle, unavailable, or failed.
/// \note Original decompilation identifier: \c eepromIsTransferBusy (0x800039EC).
s32 eepromIsTransferBusy(void) {
    s32 transferState;

    transferState = eepromGetTransferState();
    if ((transferState != EEPROM_TRANSFER_IDLE) && (transferState != EEPROM_TRANSFER_READ_FAILED) && (transferState != EEPROM_TRANSFER_CHECKSUM_FAILED) && (transferState != EEPROM_TRANSFER_WRITE_FAILED)) {
        return 1;
    }
    return 0;
}
