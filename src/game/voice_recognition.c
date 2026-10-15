/// \file
/// Dictionary registration and recognition-result handling.
/// \ingroup VoiceInteraction
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/// \defgroup VoiceInteraction Voice interaction
/// Converts recognition results into messages for actor scripts.
///
/// Gameplay selects a vocabulary and listening mode. Recognition supplies
/// candidates and quality information, which gameplay translates into command
/// messages. Delivery and actor scripts determine whether and how an actor
/// responds. Scripts may request a different vocabulary as an activity changes.
///
/// Recognition, message delivery, and script execution advance independently.
/// Recognizing a word does not guarantee delivery or a particular response.
/// \see ActorScriptVm

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "voice_recognition.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Most recently recorded recognition command status; seven denotes a completed result.
/// \note Original identifier: \c D_800F8AC4 (0x800F8AC4).
extern volatile s32 gVoiceRecognitionCommandStatus;

/// Vocabulary descriptors listing global word identifiers for each dictionary.
/// \note Original identifier: \c D_80106D2C (0x80106D2C).
extern VoiceDictionary gVoiceDictionaries[];
/// Global voice word definitions linking encoded vocabulary to gameplay commands.
/// \note Original identifier: \c D_8010452C (0x8010452C).
extern VoiceWord gVoiceWordDefinitions[];
/// Selected dictionary identifier, or minus one when none is selected.
/// \note Original identifier: \c D_8009D534 (0x8009D534).
extern s32 gVoiceSelectedDictionary;

/// Resolve a selected-dictionary entry to its global word identifier.
/// \param index Valid entry index in the selected dictionary.
/// \returns The global word identifier, or zero when no dictionary is selected.
/// \note Original decompilation identifier: \c func_80040440 (0x80040440).
s32 voiceDictionaryGetWordId(u16 index) {
    if (gVoiceSelectedDictionary != VOICE_DICTIONARY_NONE) {
        return gVoiceDictionaries[gVoiceSelectedDictionary].wordIds[index];
    }
    return 0;
}

/// Whether the selected dictionary must begin a fresh upload.
/// \note Original identifier: \c D_8009D538 (0x8009D538).
extern s32 gVoiceDictionaryReloadRequested;
/// Recorded microphone amplifier state.
/// \note Original identifier: \c D_8009D53C (0x8009D53C).
extern s32 gVoiceAmplifierEnabled;
/// Next dictionary-local word index to register with the device.
/// \note Original identifier: \c D_800F8B08 (0x800F8B08).
extern s32 gVoiceDictionaryNextWordIndex;
/// Nonzero before dictionary setup or after upload completion; zero while registering words.
/// \note Original identifier: \c D_800F8B0C (0x800F8B0C).
extern s32 gVoiceDictionaryBetweenUploads;
/// Whether a recognition read has started and remains outstanding.
/// \note Original identifier: \c D_800F8B10 (0x800F8B10).
extern s32 gVoiceRecognitionReadActive;

/// Reset dictionary-upload and recognition-read progress.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80040484 (0x80040484).
void voiceRecognitionResetProgress(void) {
    gVoiceDictionaryReloadRequested = 1;
    gVoiceDictionaryNextWordIndex = 0;
    gVoiceDictionaryBetweenUploads = 1;
    gVoiceRecognitionReadActive = 0;
    gVoiceAmplifierEnabled = 1;
}

/// Clear recognition requests and reset dictionary and read progress.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800404B4 (0x800404B4).
void voiceRecognitionResetRequests(void) {
    gVoiceRecognitionReadRequested = 0;
    gVoiceRecognitionCommandStatus = 0;
    voiceRecognitionResetProgress();
    /* Diagnostics of the ROM/memory cache allocator: no code of it is
     * left in this unit, so its strings are kept here in .rodata order. */
    TRACE("Conflict using of ROM: %p(size 0x%x) and %p(size 0x%x)\n");
    TRACE("Out of cache_memory.\n");
    TRACE("Shortage of Cache_block_num.\n");
    TRACE("caRequestRomCopy(%p, %p, 0x%x, %p)\n");
    TRACE("Conflict using of cache: %p(size 0x%x) and 0x%x(size 0x%x)\n");
    TRACE("Out of cache_memory.\n");
    TRACE("Shortage of Cache_block_num.\n");
    TRACE("caRequestMemory(%p, 0x%x, 0x%x, %p)\n");
}

/// Controller port most recently selected for voice initialization or probing.
/// \note Original identifier: \c D_800F8AC0 (0x800F8AC0).
/// \note This address also marks the graphics thread stack top.
extern volatile s32 gVoiceRecognitionDevicePort;
/// Result code from the most recent voice device operation.
/// \note Original identifier: \c D_800F8ACC (0x800F8ACC).
extern volatile s32 gVoiceRecognitionLastDeviceError;
/// Persistent device handle used for dictionary registration and recognition.
/// \note Original identifier: \c D_800F8AF8 (0x800F8AF8).
extern OSVoiceHandle gVoiceRecognitionHandle;
/// Device availability: zero for disconnected, one for ready, and two after an error.
/// \note Original identifier: \c D_8009D530 (0x8009D530).
extern s32 gVoiceRecognitionDeviceState;
/// Probed voice-unit type; recognition initialization requires value one.
/// \note Original identifier: \c D_8009D990 (0x8009D990).
extern s32 osEngVoiceUnitType;
/// Whether the retained recognition result has not yet been consumed.
/// \note Original identifier: \c D_800F8AD4 (0x800F8AD4).
extern s32 gVoiceRecognitionResultPending;
/// RAM destination for the ROM segment containing voice vocabulary data.
/// \note Original identifier: \c D_80100680 (0x80100680).
extern u8 gVoiceVocabularyData[];
extern s32 D_A9800;
extern s32 D_B3C70;
#include "rom_dma.h"
s32 voiceDeviceInitialize(OSMesgQueue *, OSVoiceHandle *, s32);
s32 voiceDeviceRegisterWord(OSVoiceHandle *, u8 *);

/// Initialize the voice recognition device and load its resident dictionary data.
/// \param queue Initialized serial-interface message queue address; the queue
/// must remain valid while the voice device is used.
/// \param channel Controller port connected to the voice recognition device, from 0 through 3.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800404E8 (0x800404E8).
void voiceRecognitionInitialize(OSMesgQueue *queue, s32 channel) {
    s32 *start = &D_A9800;
    s32 i;

    gVoiceRecognitionDevicePort = channel;
    voiceRecognitionResetProgress();
    for (i = 0; i < 2; i++) {
        gVoiceRecognitionLastDeviceError = voiceDeviceInitialize(queue, &gVoiceRecognitionHandle, channel);
        if (osEngVoiceUnitType != 1) {
            gVoiceRecognitionLastDeviceError = CONT_ERR_INVALID;
        }
        TRACE("mic64Error:%d osEngVoiceUnitType:%d\n", gVoiceRecognitionLastDeviceError, osEngVoiceUnitType);
        if (gVoiceRecognitionLastDeviceError == 0) {
            gVoiceRecognitionDeviceState = VOICE_RECOGNITION_READY;
            break;
        }
        osYieldThread();
    }
    if (i >= 2) {
        TRACE("Initializing Mic failed.\n");
        gVoiceRecognitionDeviceState = VOICE_RECOGNITION_ERROR;
    }
    romDmaReadBlocking((s32)start, gVoiceVocabularyData, (s32)&D_B3C70 - (s32)start);
    gVoiceRecognitionResultPending = 0;
    TRACE("mic64Init() End\n");
}

/// Clear the device dictionary through the retained target call representation.
/// \param handle Initialized voice handle.
/// \param wordCount Number of replacement dictionary words.
/// \returns Zero on success, or a device error code.
/// \note Portability: The 32-bit count view preserves target call scheduling.
/// The canonical u16-count declaration changes voiceDictionaryClearDevice code;
/// see voice_device.h for the device contract.
/// \note Original decompilation identifier: \c func_8004CDA4 (0x8004CDA4).
s32 voiceDeviceClearDictionary(OSVoiceHandle *handle, s32 wordCount);

/// Clear the device dictionary and reserve space for its replacement.
/// \param wordCount Number of words in the replacement dictionary.
/// \returns Zero on success, -1 when the device is unavailable, or a device error code.
/// \note Portability: dictionary-upload debug strings remain in this function to preserve the original
/// IDO register allocation and read-only data order.
/// \note Original decompilation identifier: \c func_80040608 (0x80040608).
s32 voiceDictionaryClearDevice(s32 wordCount) {
    if (gVoiceRecognitionDeviceState != VOICE_RECOGNITION_READY) {
        return -1;
    }
    gVoiceRecognitionLastDeviceError = voiceDeviceClearDictionary(&gVoiceRecognitionHandle, wordCount & 0xFFFF);
    if (gVoiceRecognitionLastDeviceError == 0) {
        return 0;
    }
    if (gVoiceRecognitionLastDeviceError == CONT_ERR_NO_CONTROLLER) {
        gVoiceRecognitionDeviceState = VOICE_RECOGNITION_UNAVAILABLE;
        voiceRecognitionResetRequests();
    } else {
        gVoiceRecognitionDeviceState = VOICE_RECOGNITION_ERROR;
        voiceRecognitionResetRequests();
    }
    TRACE("%d:%s\n");
    TRACE("All Words Downloaded.\n");
    return gVoiceRecognitionLastDeviceError;
}

/// Number of words in the selected dictionary upload.
/// \note Original identifier: \c D_800F8AD0 (0x800F8AD0).
extern s32 gVoiceSelectedDictionaryWordCount;

/* Copy the encoded text of dictionary word `index` into `dst` as u16
 * characters, including the zero terminator. */
#define VOICE_COPY_WORD_TEXT(dst, index, src, i)                                                                       \
    do {                                                                                                               \
        (src) = gVoiceWordDefinitions[voiceDictionaryGetWordId(index)].text;                                           \
        for ((i) = 0;; (i)++) {                                                                                        \
            u16 c = (((src)[0] << 8) | (src)[1]) & 0xFFFF;                                                             \
            (dst)[i] = c;                                                                                              \
            (src) += 2;                                                                                                \
            if (c == 0) {                                                                                              \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/// Advance registration of the selected dictionary with the voice recognition device.
/// \param dictionary Valid dictionary index to select and upload.
/// \param batch Positive upload batch size, or zero to upload all remaining words.
/// \returns 1 when the dictionary is fully registered, or 0 while setup or uploading remains incomplete or a device error occurs.
/// \note Original decompilation identifier: \c func_800406B4 (0x800406B4).
/// \note Completion means registration is finished; it does not start a recognition read.
/// \see micSessionUpdateDictionary
s32 voiceDictionaryUpload(s32 dictionary, s32 batch) {
    s32 i;
    u8 *src;
    u16 word[72];

    if (gVoiceRecognitionDeviceState != VOICE_RECOGNITION_READY) {
        voiceRecognitionResetProgress();
        return 0;
    }
    if (dictionary != gVoiceSelectedDictionary || gVoiceDictionaryReloadRequested != 0) {
        gVoiceSelectedDictionary = dictionary;
        gVoiceDictionaryReloadRequested = 0;
        gVoiceDictionaryBetweenUploads = 1;
    } else if (gVoiceDictionaryBetweenUploads != 0) {
        return 1;
    }
    if (gVoiceDictionaryBetweenUploads != 0) {
        gVoiceSelectedDictionaryWordCount = gVoiceDictionaries[gVoiceSelectedDictionary].count;
        if (voiceDictionaryClearDevice(gVoiceSelectedDictionaryWordCount) == 0) {
            gVoiceDictionaryBetweenUploads = 0;
            gVoiceDictionaryNextWordIndex = 0;
        }
        return 0;
    }
    while (gVoiceDictionaryNextWordIndex < gVoiceSelectedDictionaryWordCount) {
        VOICE_COPY_WORD_TEXT(word, gVoiceDictionaryNextWordIndex, src, i);
        gVoiceRecognitionLastDeviceError = voiceDeviceRegisterWord(&gVoiceRecognitionHandle, (u8 *)word);
        if (gVoiceRecognitionLastDeviceError == 0) {
            gVoiceDictionaryNextWordIndex++;
        } else if (gVoiceRecognitionLastDeviceError == CONT_ERR_NO_CONTROLLER) {
            gVoiceRecognitionDeviceState = VOICE_RECOGNITION_UNAVAILABLE;
            voiceRecognitionResetRequests();
            return 0;
        } else {
            gVoiceRecognitionDeviceState = VOICE_RECOGNITION_ERROR;
            voiceRecognitionResetRequests();
            return 0;
        }
        if (batch != 0 && gVoiceDictionaryNextWordIndex % batch == 0) {
            break;
        }
    }
    if (gVoiceDictionaryNextWordIndex < gVoiceSelectedDictionaryWordCount) {
        return 0;
    }
    gVoiceDictionaryBetweenUploads = 1;
    return 1;
}

/// Start a recognition read when requested and the device is ready.
/// \returns 1 when a read starts, or 0 otherwise.
/// \note Original decompilation identifier: \c func_8004090C (0x8004090C).
s32 voiceRecognitionStartRequestedRead(void) {
    s32 ret = 0;

    if (gVoiceRecognitionDeviceState != VOICE_RECOGNITION_READY) {
        return 0;
    }
    if (gVoiceRecognitionReadRequested != 0) {
        gVoiceRecognitionLastDeviceError = osVoiceStartReadData(&gVoiceRecognitionHandle);
        if (gVoiceRecognitionLastDeviceError == 0) {
            ret = 1;
        } else if (gVoiceRecognitionLastDeviceError == CONT_ERR_NO_CONTROLLER) {
            gVoiceRecognitionDeviceState = VOICE_RECOGNITION_UNAVAILABLE;
            voiceRecognitionResetRequests();
        } else {
            gVoiceRecognitionDeviceState = VOICE_RECOGNITION_ERROR;
            voiceRecognitionResetRequests();
        }
    } else {
        gVoiceRecognitionCommandStatus = 0;
    }
    return ret;
}

#define VOICE_DEVICE_STATE_VOLATILE (*(volatile s32 *)&gVoiceRecognitionDeviceState)
/// Most recently completed recognition result, retained until replaced.
/// \note Original identifier: \c D_800F8AD8 (0x800F8AD8).
extern OSVoiceData gVoiceRecognitionResult;

/// Advance recognition reading and retain a newly completed result.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800409CC (0x800409CC).
void voiceRecognitionPoll(void) {
    s32 pad[2];
    OSVoiceData sp18;

    if (VOICE_DEVICE_STATE_VOLATILE == VOICE_RECOGNITION_READY) {
        if (gVoiceRecognitionReadActive == 0) {
            gVoiceRecognitionReadActive = voiceRecognitionStartRequestedRead();
            return;
        }
        gVoiceRecognitionLastDeviceError = osVoiceGetReadData(&gVoiceRecognitionHandle, &sp18);
        gVoiceRecognitionCommandStatus = gVoiceRecognitionHandle.cmd_status;
        if (gVoiceRecognitionLastDeviceError == 0) {
            gVoiceRecognitionResult = sp18;
            gVoiceRecognitionResultPending = 1;
            gVoiceRecognitionReadActive = 0;
            gVoiceRecognitionCommandStatus = 7;
        } else if (gVoiceRecognitionLastDeviceError == CONT_ERR_NO_CONTROLLER) {
            VOICE_DEVICE_STATE_VOLATILE = VOICE_RECOGNITION_UNAVAILABLE;
            voiceRecognitionResetRequests();
            return;
        } else if (gVoiceRecognitionLastDeviceError != CONT_ERR_NOT_READY) {
            VOICE_DEVICE_STATE_VOLATILE = VOICE_RECOGNITION_ERROR;
            voiceRecognitionResetRequests();
            return;
        }
        switch (gVoiceRecognitionCommandStatus) {
        case 1:
        case 3:
            if (gVoiceRecognitionReadRequested == 0) {
                gVoiceRecognitionLastDeviceError = osVoiceStopReadData(&gVoiceRecognitionHandle);
                if (gVoiceRecognitionLastDeviceError == 0) {
                    gVoiceRecognitionReadActive = 0;
                    return;
                }
                if (gVoiceRecognitionLastDeviceError == CONT_ERR_NO_CONTROLLER) {
                    VOICE_DEVICE_STATE_VOLATILE = VOICE_RECOGNITION_UNAVAILABLE;
                    voiceRecognitionResetRequests();
                    return;
                }
                VOICE_DEVICE_STATE_VOLATILE = VOICE_RECOGNITION_ERROR;
                voiceRecognitionResetRequests();
            }
            break;
        case 0:
        case 2:
        case 4:
        case 5:
        case 6:
        case 7:
            break;
        }
    }
}

/// Consume the pending recognition result.
/// \param out Optional destination for the complete result; unchanged when none is pending.
/// \returns The first device answer as a dictionary-local index, or VOICE_RECOGNITION_NO_RESULT when no
/// result is pending. Check the returned result's answer count before using the index.
/// \note Original decompilation identifier: \c func_80040BA0 (0x80040BA0).
/// \note Consumes the pending-result notification even when out is null. Results
/// are dictionary-local candidates, not gameplay command identifiers.
/// \see voiceDictionaryGetWordId
/// \see voiceCommandTakeMessage
s32 voiceRecognitionTakeResult(OSVoiceData *out) {
    s32 pad; /* unused, sizes the frame */
    s32 r;
    u32 mask;

    mask = osGetIntMask();
    osSetIntMask(OS_IM_NONE);
    if (gVoiceRecognitionResultPending == 1) {
        if (out != NULL) {
            *out = gVoiceRecognitionResult;
        }
        r = gVoiceRecognitionResult.answer[0];
    } else {
        r = VOICE_RECOGNITION_NO_RESULT;
    }
    gVoiceRecognitionResultPending = 0;
    osSetIntMask(mask);
    return r;
}

/// Get the current device recognition command status.
/// \returns The most recently recorded recognition command status.
/// \note Original decompilation identifier: \c func_80040C70 (0x80040C70).
s32 voiceRecognitionGetCommandStatus(void) {
    return gVoiceRecognitionCommandStatus;
}

/// Check whether a recognition read is active.
/// \returns Nonzero while a read is active, or zero otherwise.
/// \note Original decompilation identifier: \c func_80040C80 (0x80040C80).
s32 voiceRecognitionIsReadActive(void) {
    return gVoiceRecognitionReadActive;
}

/// Get the display name of a selected-dictionary entry.
/// \param index Dictionary-local word index.
/// \returns A borrowed word-name pointer, or an empty string when the index is
/// out of range or no dictionary is selected.
/// \note Original decompilation identifier: \c func_80040C8C (0x80040C8C).
char *voiceDictionaryGetWordName(u16 index) {
    if (index < gVoiceSelectedDictionaryWordCount && gVoiceSelectedDictionary != VOICE_DICTIONARY_NONE) {
        return gVoiceWordDefinitions[voiceDictionaryGetWordId(index)].name;
    }
    return "";
}

/// Get the game command type associated with a selected-dictionary word.
/// \param index Dictionary-local word index.
/// \returns The command type, or -1 when the index is out of range or no
/// dictionary is selected.
/// \note Original decompilation identifier: \c func_80040CF8 (0x80040CF8).
s32 voiceDictionaryGetCommandType(u16 index) {
    if (index < gVoiceSelectedDictionaryWordCount && gVoiceSelectedDictionary != VOICE_DICTIONARY_NONE) {
        return gVoiceWordDefinitions[voiceDictionaryGetWordId(index)].commandType;
    }
    return -1;
}

/// Get the game command value associated with a selected-dictionary word.
/// \param index Dictionary-local word index.
/// \returns The command value, or -1 when the index is out of range or no
/// dictionary is selected.
/// \note Original decompilation identifier: \c func_80040D60 (0x80040D60).
s32 voiceDictionaryGetCommandValue(u16 index) {
    if (index < gVoiceSelectedDictionaryWordCount && gVoiceSelectedDictionary != VOICE_DICTIONARY_NONE) {
        return gVoiceWordDefinitions[voiceDictionaryGetWordId(index)].commandValue;
    }
    return -1;
}

/// Get the voice recognition device availability state.
/// \returns 1 when ready, 0 when disconnected or unconfirmed, or 2 after an initialization or device error.
/// \note Original decompilation identifier: \c func_80040DC8 (0x80040DC8).
s32 voiceRecognitionGetDeviceState(void) {
    return gVoiceRecognitionDeviceState;
}

/// Check whether a voice recognition device is connected to a controller port.
/// \param queue Serial-interface message queue used for the controller query.
/// \param channel Controller port to inspect, from 0 through 3.
/// \returns 1 when the expected device is connected, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80040DD4 (0x80040DD4).
s32 voiceRecognitionProbePort(OSMesgQueue *queue, s32 channel) {
    OSContStatus sp20[4];
    OSContStatus *p;

    gVoiceRecognitionDevicePort = channel;
    osContStartQuery(queue);
    osRecvMesg(queue, NULL, OS_MESG_BLOCK);
    osContGetQuery(sp20);
    p = &sp20[channel];
    if (p->errno == 0 && (p->type & CONT_TYPE_MASK) == CONT_TYPE_VOICE) {
        return 1;
    }
    gVoiceRecognitionDeviceState = VOICE_RECOGNITION_UNAVAILABLE;
    return 0;
}

s32 __osVoiceSetADConverter(s32, s32, u8);

/// Set the microphone amplifier state on the ready voice recognition device.
/// \param flag VOICE_AMPLIFIER_ON to enable amplification, or VOICE_AMPLIFIER_OFF to disable it.
/// \returns Zero on success, -1 when the device is unavailable, or a device error code.
/// \note Original decompilation identifier: \c func_80040E64 (0x80040E64).
s32 voiceRecognitionSetAmplifier(u8 flag) {
    if (gVoiceRecognitionDeviceState != VOICE_RECOGNITION_READY) {
        gVoiceAmplifierEnabled = 0;
        return -1;
    }
    gVoiceRecognitionLastDeviceError =
        __osVoiceSetADConverter((s32)gVoiceRecognitionHandle.__mq, gVoiceRecognitionHandle.__channel, flag);
    if (gVoiceRecognitionLastDeviceError != 0) {
        voiceRecognitionResetRequests();
    } else if (flag != 0) {
        if (flag == VOICE_AMPLIFIER_ON) {
            gVoiceAmplifierEnabled = 1;
        } else {
            TRACE("MIC64_ERROR: Invalid flag: %d\n", flag);
        }
    } else {
        gVoiceAmplifierEnabled = 0;
    }
    return gVoiceRecognitionLastDeviceError;
}

/// Get the recorded microphone amplifier state.
/// \returns Nonzero when enabled, or zero when disabled.
/// \note Original decompilation identifier: \c func_80040F10 (0x80040F10).
s32 voiceRecognitionIsAmplifierEnabled(void) {
    return gVoiceAmplifierEnabled;
}
