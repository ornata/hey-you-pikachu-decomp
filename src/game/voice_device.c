#include "voice_device.h"
#include "common.h"
#include "PRinternal/controller_voice.h"
void _bzero(void *, s32);

/// Probed voice-unit type; recognition initialization requires value one.
/// \note Original identifier: \c D_8009D990 (0x8009D990).
extern s32 osEngVoiceUnitType;
/// Five A/D-converter initialization commands; individual command meanings remain unresolved.
/// \note Original identifier: \c D_8009D994 (0x8009D994).
extern u8 gVoiceDeviceAdConverterCommands[5];
/// Initialize a voice recognition handle and its connected device.
/// \param mq Serial-interface message queue.
/// \param hd Handle receiving the device connection state.
/// \param channel Controller port connected to the device, from 0 through 3.
/// \returns Zero on success, or a controller or voice-device error code.
/// \note Original decompilation identifier: \c func_8004C8A0 (0x8004C8A0).
s32 voiceDeviceInitialize(OSMesgQueue *mq, OSVoiceHandle *hd, s32 channel) {
    OSTime start;
    s32 ret;
    s32 i;
    u8 data[2];
    u8 status = 0;
    u8 command[4];
    hd->__channel = channel;
    hd->__mq = mq;
    hd->__mode = 0;
    ret = __osVoiceGetStatus(mq, channel, &status);
    if (ret != 0)
        return ret;
    ret = __osContChannelReset(mq, channel);
    if (ret != 0)
        return CONT_ERR_CONTRFAIL;
    start = osGetCount();
    while ((u64)osGetCount() - start < 2343750) {
    }
    ret = __osVoiceGetStatus(mq, channel, &status);
    if (ret != 0)
        return ret;
    __osVoiceContRead2(hd->__mq, hd->__channel, 0, data);
    osEngVoiceUnitType = data[0] | (data[1] << 8);
    for (i = 0; i < 5; i++) {
        ret = __osVoiceSetADConverter(mq, channel, gVoiceDeviceAdConverterCommands[i]);
        if (ret != 0)
            return ret;
    }
    ret = __osVoiceGetStatus(mq, channel, &status);
    if (ret != 0)
        return ret;
    if (status & 2)
        return CONT_ERR_VOICE_NO_RESPONSE;
    *(u32 *)command = 0x100;
    ret = __osVoiceContWrite4(mq, channel, 0, command);
    if (ret != 0)
        return ret;
    ret = __osVoiceCheckResult(hd, &status);
    if (ret & 0xFF00)
        ret = CONT_ERR_INVALID;
    return ret;
}

/// Register an encoded word in the device dictionary.
/// \param hd Initialized voice recognition handle.
/// \param word Zero-terminated target-encoded 16-bit text, with at most 37 nonzero code units.
/// \returns Zero on success, or a controller or word-registration error code.
/// \note Original decompilation identifier: \c func_8004CAAC (0x8004CAAC).
s32 voiceDeviceRegisterWord(OSVoiceHandle *hd, u8 *word) {
    s32 ret;
    s32 length;
    s32 i;
    u8 status;
    u8 command[80];
    u8 *cursor;
    ret = __osVoiceGetStatus(hd->__mq, hd->__channel, &status);
    if (ret != 0)
        return ret;
    if (status & 2)
        return CONT_ERR_VOICE_NO_RESPONSE;
    length = 0;
    cursor = word;
    for (;;) {
        cursor += 2;
        if ((((cursor[-1] << 8) | cursor[-2]) & 0xFFFF) == 0)
            break;
        length += 2;
    }
    _bzero(command, 80);
    for (i = 0; i < length; i += 2) {
        command[79 - length + i] = word[i];
        command[79 - length + i - 1] = word[i + 1];
    }
    command[74 - i] = 3;
    ret = __osVoiceContWrite20(hd->__mq, hd->__channel, 0, command);
    if (ret != 0)
        return ret;
    ret = __osVoiceContWrite20(hd->__mq, hd->__channel, 0, command + 20);
    if (ret != 0)
        return ret;
    ret = __osVoiceContWrite20(hd->__mq, hd->__channel, 0, command + 40);
    if (ret != 0)
        return ret;
    ret = __osVoiceContWrite20(hd->__mq, hd->__channel, 0, command + 60);
    if (ret != 0)
        return ret;
    ret = __osVoiceCheckResult(hd, &status);
    if (ret != 0) {
        if (ret & 0x100)
            ret = 0xD;
        else if (ret & 0x200)
            ret = 0xE;
        else if (ret & 0xFF00)
            ret = CONT_ERR_INVALID;
    }
    return ret;
}

/// Set the device dictionary word mask.
/// \param hd Initialized voice recognition handle.
/// \param mask Mask buffer with room for bytes rounded up to an even length.
/// \param bytes Mask length in bytes, from 0 through 38.
/// \returns The final device operation status: zero on success, or an error code.
/// \note Original decompilation identifier: \c func_8004CC5C (0x8004CC5C).
s32 voiceDeviceSetDictionaryMask(OSVoiceHandle *hd, u8 *mask, s32 bytes) {
    s32 ret;
    s32 i;
    s32 length;
    u8 status;
    u8 command[40];
    ret = __osVoiceGetStatus(hd->__mq, hd->__channel, &status);
    if (ret != 0)
        return ret;
    if (status & 2)
        return CONT_ERR_VOICE_NO_RESPONSE;
    length = (bytes & 1) ? bytes + 1 : bytes;
    _bzero(command, 40);
    command[38 - length] = 4;
    for (i = 0; i < length; i += 2) {
        command[i - length + 40] = mask[i];
        command[i - length + 41] = mask[i + 1];
    }
    if (bytes & 1)
        command[39] = 0;
    ret = __osVoiceContWrite20(hd->__mq, hd->__channel, 0, command);
    if (ret == 0)
        ret = __osVoiceCheckResult(hd, &status);
    ret = __osVoiceContWrite20(hd->__mq, hd->__channel, 0, command + 20);
    if (ret == 0) {
        ret = __osVoiceCheckResult(hd, &status);
        if (ret & 0xFF00)
            ret = CONT_ERR_INVALID;
    }
    return ret;
}

/// Clear the device dictionary and set its word capacity.
/// \param hd Initialized voice recognition handle.
/// \param word Number of words in the replacement dictionary.
/// \returns Zero on success, or a controller or voice-device error code.
/// \note Original decompilation identifier: \c func_8004CDA4 (0x8004CDA4).
s32 voiceDeviceClearDictionary(OSVoiceHandle *hd, u16 word) {
    s32 ret;
    u8 status;
    u8 command[4];
    ret = __osVoiceGetStatus(hd->__mq, hd->__channel, &status);
    if (ret != 0)
        return ret;
    if (status & 2)
        return CONT_ERR_VOICE_NO_RESPONSE;
    *(u32 *)command = 0x02000000;
    command[2] = word & 0xFF;
    command[3] = (word & 0xFF00) >> 8;
    ret = __osVoiceContWrite4(hd->__mq, hd->__channel, 0, command);
    if (ret != 0)
        return ret;
    ret = __osVoiceCheckResult(hd, &status);
    if (ret & 0xFF00)
        ret = CONT_ERR_INVALID;
    return ret;
}
