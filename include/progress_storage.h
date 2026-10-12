/// \file
/// Declares progress-record initialization, EEPROM requests and gameplay restoration.
/// EEPROM requests acknowledge setup separately from transfer completion.

#ifndef PROGRESS_STORAGE_H
#define PROGRESS_STORAGE_H

#include "ultra64.h"

/// Requests that save control data be written to EEPROM.
/// \returns Nothing; transfer completion and errors are queried through the EEPROM interface.
/// \pre Progress storage is initialized and no EEPROM request or transfer is outstanding.
/// \note Original decompilation identifier: \c func_80032CD4 (0x80032CD4).
void progressSaveControlDataToEeprom(void);

/// Requests save control data from EEPROM.
/// \returns Nothing; transfer completion and errors are queried through the EEPROM interface.
/// \pre Progress storage is initialized and no EEPROM request or transfer is outstanding.
/// \note Original decompilation identifier: \c func_80032D0C (0x80032D0C).
void progressLoadControlDataFromEeprom(void);

/// Captures current gameplay progress and requests that its save record be written to EEPROM.
/// \param record Progress record index, one or two, each with a backup record.
/// \returns Nothing; transfer completion and errors are queried through the EEPROM interface.
/// \pre Progress storage is initialized and no EEPROM request or transfer is outstanding.
/// \note Original decompilation identifier: \c func_80032D44 (0x80032D44).
void progressSaveRecordToEeprom(s32 record);

/// Requests a saved progress record from EEPROM into the progress buffer.
/// \param record Progress record index, one or two, each with a backup record.
/// \returns Nothing; transfer completion and errors are queried through the EEPROM interface.
/// \pre Progress storage is initialized and no EEPROM request or transfer is outstanding.
/// \note Original decompilation identifier: \c func_80032DC4 (0x80032DC4).
void progressLoadRecordFromEeprom(s32 record);

/// Applies the loaded progress record to the active gameplay state and settings.
/// \pre A valid progress record is available and any preceding EEPROM read has completed successfully.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032E10 (0x80032E10).
void progressApplyLoadedRecord(void);

/// Resets the progress record to its initial gameplay values.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032EAC (0x80032EAC).
void progressResetRecordToDefaults(void);

/// Initializes progress storage and prepares a default save record.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032F30 (0x80032F30).
void progressInitializeSaveStorage(void);

#endif
