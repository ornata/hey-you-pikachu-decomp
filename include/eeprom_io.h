/// \file
/// Defines controller-thread EEPROM requests and asynchronous transfer contracts.

#ifndef EEPROM_IO_H
#define EEPROM_IO_H

#include "game_message.h"
#include <ultra64.h>

/// Asynchronous transfer states reported by the EEPROM interface.
typedef enum EepromTransferState {
    EEPROM_TRANSFER_IDLE = 0,            ///< No transfer is active, or EEPROM is unavailable to a state query.
    EEPROM_TRANSFER_READING = 1,         ///< A checksum-protected read is pending.
    EEPROM_TRANSFER_WRITING = 2,         ///< A checksum-protected write is pending.
    EEPROM_TRANSFER_READ_FAILED = 3,     ///< Reading failed or a read could not start without supported EEPROM.
    EEPROM_TRANSFER_CHECKSUM_FAILED = 4, ///< Read payload checksum did not match.
    EEPROM_TRANSFER_WRITE_FAILED = 5     ///< Writing failed or a write could not start without supported EEPROM.
} EepromTransferState;

/// Request borrowed by the controller thread until pending becomes zero.
/// Only one queued request and one transfer may be outstanding. Acknowledgment
/// reports setup, while transfer completion is queried separately.
typedef struct EepromRequest {
    s16 type; ///< GameMessageKind selector; GAME_MESSAGE_EEPROM.
    u8 padding02[2];
    s32 command;    ///< Zero requests a read; one requests a write.
    s32 pending;    ///< Nonzero until the controller thread acknowledges setup.
    s32 result;     ///< Transfer setup result, valid after acknowledgment.
    s32 page;       ///< First eight-byte EEPROM page.
    s32 backupPage; ///< Alternate starting page, or a negative value to disable it.
    void *buffer;   ///< Borrowed payload storage; reads retain it until completion.
    s32 byteCount;  ///< Payload size excluding the four-byte checksum.
} EepromRequest;

typedef char EepromRequestSizeCheck[(sizeof(EepromRequest) == 0x20) ? 1 : -1];
typedef char EepromRequestBufferCheck[(((u32) & ((EepromRequest *)0)->buffer) == 0x18) ? 1 : -1];
typedef char EepromRequestPendingCheck[(((u32) & ((EepromRequest *)0)->pending) == 8) ? 1 : -1];

/// Start a checksum-protected read without waiting for completion.
/// \param page First eight-byte EEPROM page.
/// \param backupPage Fallback starting page, or negative to disable; must fit s16.
/// \param destination Writable payload storage retained until transfer completion.
/// \param byteCount Payload size from zero through 508; rounded payload plus checksum must fit the EEPROM page range.
/// \returns Zero if started, three if unavailable, or the current busy transfer state.
s32 eepromStartRead(s32 page, s32 backupPage, void *destination, s32 byteCount);

/// Start a checksum-protected write without waiting for completion.
/// \param page First eight-byte EEPROM page.
/// \param backupPage Second copy's starting page, or negative to disable; must fit s16.
/// \param source Borrowed payload storage; reusable after this call returns.
/// \param byteCount Payload size from zero through 508; rounded payload plus checksum must fit the EEPROM page range.
/// \returns Zero if started, five if unavailable, or the current busy transfer state.
s32 eepromStartWrite(s32 page, s32 backupPage, void *source, s32 byteCount);

/// Submit a read through the controller thread and wait for acknowledgment.
/// \param page First eight-byte EEPROM page.
/// \param backupPage Fallback starting page, or negative to disable; must fit s16.
/// \param destination Writable payload storage retained until transfer completion.
/// \param byteCount Payload size from zero through 508; rounded payload plus checksum must fit the EEPROM page range.
/// \returns Setup result as for eepromStartRead; the transfer may remain in progress.
s32 eepromRequestRead(s32 page, s32 backupPage, void *destination, s32 byteCount);

/// Submit a write through the controller thread and wait for acknowledgment.
/// \param page First eight-byte EEPROM page.
/// \param backupPage Second copy's starting page, or negative to disable; must fit s16.
/// \param source Borrowed payload storage; reusable after this call returns.
/// \param byteCount Payload size from zero through 508; rounded payload plus checksum must fit the EEPROM page range.
/// \returns Setup result as for eepromStartWrite; the transfer may remain in progress.
s32 eepromRequestWrite(s32 page, s32 backupPage, void *source, s32 byteCount);

/// Dispatch transfer setup on the controller thread.
/// \param request Borrowed request whose result and pending fields receive acknowledgment.
/// \returns Nothing; clearing pending does not imply transfer completion.
void eepromDispatchRequest(EepromRequest *request);

/// Probe the EEPROM and reset transfer state.
/// \returns Nothing.
void eepromInitializeTransfers(void);

/// Advance an outstanding transfer on the controller thread.
/// \returns Nothing.
void eepromUpdateTransfer(void);

/// Get the cached EEPROM probe result.
/// \returns The device type reported by the SDK probe.
s32 eepromGetDeviceType(void);

/// Query the current asynchronous transfer state.
/// \returns Zero when idle or unavailable, one/two for read/write, or three/four/five for read/checksum/write failure.
s32 eepromGetTransferState(void);

/// Query transfer activity.
/// \returns One during read or write, otherwise zero.
s32 eepromIsTransferBusy(void);

/// Query queued request and transfer activity.
/// \returns One while either is outstanding, otherwise zero.
/// \note The int spelling preserves the original IDO register allocation.
int eepromIsRequestOrTransferBusy(void);

/// Yield until the queued request and transfer finish.
/// \returns Nothing.
void eepromWaitForIdle(void);

/// Query transfer state through the queued-request interface.
/// \returns The state reported by eepromGetTransferState.
s32 eepromGetQueuedTransferState(void);

#endif
