#include "audio_control.h"
#include "player_chase.h"
#include "math_angles.h"
#include "random.h"
#include "weighted_choice.h"
#include "story_milestones.h"
#include "progress_field_types.h"
#include "progress_storage.h"
#include "progress_fields.h"
#include "progress_value.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "vector_direction.h"
#include "direction_animation.h"
#include "eeprom_io.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on
/* The same trace in a leaf function: a dead call would still make IDO give
 * the function an outgoing-argument frame, so the arguments become a
 * discarded comma expression that keeps only the format string. */
#define TRACE_LEAF (void)

/// Payload byte counts for three primary EEPROM records and their three backups.
/// \note Portability: A word-aligned record preserves the original data placement;
/// an array declaration would receive eight-byte alignment from IDO.
typedef struct EepromRecordSizeTable {
    s32 payloadBytes[6]; ///< Payload sizes excluding the transfer checksum and block padding.
} EepromRecordSizeTable;
typedef char EepromRecordSizeTableSizeCheck[(sizeof(EepromRecordSizeTable) == 0x18) ? 1 : -1];
s32 D_8009ADC0 = -1;
/// Payload byte counts used to place primary EEPROM records and their backup copies.
/// \note Original identifier: \c D_8009ADC4 (0x8009ADC4).
EepromRecordSizeTable gEepromRecordPayloadSizes = {{1, 0x74, 0x74, 1, 0x74, 0x74}};
/// Storage type identifiers indexed by resolved progress field ID.
/// \note Original identifier: \c D_8009A754 (0x8009A754).
extern s32 gProgressFieldTypes[];




/// Sets one direction in a pose from pitch and yaw angles.
/// \param pitch Direction pitch in degrees.
/// \param yaw Direction yaw in degrees.
/// \param pose Pose that receives the direction.
/// \param index Direction index within the pose, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80031F50 (0x80031F50).
void directionPoseSetAngles(f32 pitch, f32 yaw, u8 *pose, s32 index) {
    f32 sp1C[3];

    vector3FromPitchYawDegrees(pitch, yaw, sp1C);
    vector3PackNormalBytes(sp1C, pose + 0x10 + (index * 0x10));
}

/// Sets one direction in the next pose of a direction animation.
/// \param animation Direction animation whose next pose is updated.
/// \param pitch Direction pitch in degrees.
/// \param yaw Direction yaw in degrees.
/// \param index Direction index within the pose, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80031F94 (0x80031F94).
void directionAnimationSetNextPoseAngles(u8 *animation, f32 pitch, f32 yaw, s32 index) {
    u16 flag = ((DirectionAnimation *)animation)->poseIndex;
    u8 *d = animation;

    d += (flag ^ 1) * 0x28;
    directionPoseSetAngles(pitch, yaw, d, index);
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advances the animated directions and updates the selected pose for the current frame.
/// \param animation Direction animation state to advance.
/// \param poseIndex Pose to update, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80031FD0 (0x80031FD0).
void directionAnimationAdvancePose(u8 *animation, s32 poseIndex) {
    DirectionAngularMotion *s;
    s32 i;

    for (i = 0, s = ((DirectionAnimation *)animation)->directions; i < 2; i++, s++) {
        s->pitch += s->pitchRate * gRuntimeFrameSeconds;
        s->yaw += s->yawRate * gRuntimeFrameSeconds;
        while (s->pitch < 0.0f) {
            s->pitch += 360.0f;
        }
        while (s->pitch >= 360.0f) {
            s->pitch -= 360.0f;
        }
        while (s->yaw < 0.0f) {
            s->yaw += 360.0f;
        }
        while (s->yaw >= 360.0f) {
            s->yaw -= 360.0f;
        }
        directionPoseSetAngles(s->pitch, s->yaw, ((DirectionPose *)animation)[poseIndex].raw, i);
    }
}

/// Packed direction slot in the default pose; only the first three signed bytes are decoded.
typedef struct PackedDirectionSlot {
    s8 bytes[0x10]; ///< Packed slot storage, including uninterpreted trailing bytes.
} PackedDirectionSlot;
/// Interior view of the two packed directions in the default direction-animation pose.
/// \note Original identifier: \c D_8009A6C0 (0x8009A6C0).
extern PackedDirectionSlot gDirectionAnimationDefaultDirections[2];

/// Advances direction motion and periodically retargets it toward the default pose.
/// \param animation Direction animation state to update.
/// \param poseIndex Selected pose to update, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032148 (0x80032148).
void directionAnimationUpdateRetargetingMotion(u8 *animation, s32 poseIndex) {
    DirectionAngularMotion *s;
    s32 i;
    f32 direction[3];
    f32 a;
    f32 b;

    ((DirectionAnimation *)animation)->remaining -= gRuntimeFrameSeconds;
    if (((DirectionAnimation *)animation)->remaining < 0.0f) {
        ((DirectionAnimation *)animation)->mode = 2;
        ((DirectionAnimation *)animation)->remaining = 0.5f;
        for (i = 0, s = ((DirectionAnimation *)animation)->directions; i < 2; i++, s++) {
            vector3UnpackNormalBytes(gDirectionAnimationDefaultDirections[i].bytes, direction);
            vector3EvaluateOppositeLookAngles(direction, &a, &b);
            s->pitchRate = mathWrapSignedDegrees(a - s->pitch) * 2;
            s->yawRate = mathWrapSignedDegrees(b - s->yaw) * 2;
        }
    } else {
        directionAnimationAdvancePose(animation, poseIndex);
    }
}

/// Advances direction motion until its duration expires, then restores the default pose.
/// \param animation Direction animation state to update.
/// \param poseIndex Selected pose to update, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003225C (0x8003225C).
void directionAnimationUpdateTimedMotion(u8 *animation, s32 poseIndex) {
    ((DirectionAnimation *)animation)->remaining =
        (f32)(((DirectionAnimation *)animation)->remaining - gRuntimeFrameSeconds);
    if (((DirectionAnimation *)animation)->remaining < 0.0f) {
        directionAnimationReset(animation);
        return;
    }
    directionAnimationAdvancePose(animation, poseIndex);
}

/// Word-aligned view of a complete packed pose used by reset copies.
/// The shared byte view changes IDO copy code here; preserve this matching alignment.
typedef struct DirectionPoseCopy {
    s32 words[10]; ///< Complete packed pose storage.
} DirectionPoseCopy;
/// Default pose shared by both direction-animation pose slots.
/// \note Original identifier: \c D_8009A6B0 (0x8009A6B0).
extern DirectionPoseCopy gDirectionAnimationDefaultPose;

/// Restores the default direction poses and leaves the animation stopped.
/// \param animation Direction animation state to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800322B8 (0x800322B8).
void directionAnimationReset(u8 *animation) {
    DirectionPoseCopy t = gDirectionAnimationDefaultPose;

    *(DirectionPoseCopy *)(animation + 0x28) = t;
    *(DirectionPoseCopy *)animation = t;
    ((DirectionAnimation *)animation)->mode = 0;
    ((DirectionAnimation *)animation)->poseIndex = 0;
}

/// Stops direction animation while preserving its current pose.
/// \param animation Direction animation state to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003237C (0x8003237C).
void directionAnimationStop(u8 *animation) {
    ((DirectionAnimation *)animation)->mode = 0;
}


/// Starts timed direction motion from the current pose.
/// \param animation Direction animation state to configure.
/// \param duration Nonnegative duration of the motion in seconds.
/// \param rates Two pairs of pitch and yaw angular rates in degrees per second.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032384 (0x80032384).
void directionAnimationStartMotion(u8 *animation, f32 duration, DirectionAngularRates *rates) {
    s32 pad[2]; /* unused, sizes the frame */
    f32 direction[3];
    f32 *rec;
    s32 off;
    DirectionAngularRates *p;

    ((DirectionAnimation *)animation)->mode = 1;
    rec = (f32 *)((DirectionAnimation *)animation)->directions;
    off = 0;
    p = rates;
    ((DirectionAnimation *)animation)->duration = duration;
    ((DirectionAnimation *)animation)->remaining = duration;
    do {
        vector3UnpackNormalBytes((s8 *)&((DirectionAnimation *)animation)
                                     ->poses[((DirectionAnimation *)animation)->poseIndex]
                                     .raw[0x10 + off],
                                 direction);
        vector3EvaluateOppositeLookAngles(direction, rec, rec + 1);
        off += 0x10;
        rec[2] = p->pitch;
        rec[3] = p->yaw;
        p++;
        rec += 4;
    } while (off != 0x20);
}

/// Direction-animation update callbacks indexed by nonzero motion mode.
/// \note Original identifier: \c D_8009A6D8 (0x8009A6D8).
extern DirectionAnimationUpdateHandler gDirectionAnimationUpdateHandlers[];

/// Advances an active direction animation for the current frame.
/// \param animation Direction animation state to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032448 (0x80032448).
void directionAnimationUpdate(u8 *animation) {
    u16 motionKind;

    motionKind = (u16)((DirectionAnimation *)animation)->mode;
    if (motionKind != 0) {
        ((DirectionAnimation *)animation)->poseIndex ^= 1;
        gDirectionAnimationUpdateHandlers[motionKind](animation, ((DirectionAnimation *)animation)->poseIndex);
    }
}

/// Prepares a weighted selection table for repeated random choices.
/// \param t Mutable selection table with a zero-key terminator; prepared tables are left unchanged.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032490 (0x80032490).
void weightedChoicePrepareTable(WeightedChoiceTable *t) {
    s32 n = 0;
    WeightedChoiceEntry *e;

    if (t->lastChoiceIndex == 0) {
        e = t->entries + 1;
        while (1) {
            e->weight += e[-1].weight;
            e++;
            if (e[-1].key == 0)
                break;
            n++;
        }
        t->lastChoiceIndex = n;
    }
}


/// Chooses a key from a weighted selection table, preparing the table when needed.
/// \param t Mutable selection table with a zero-key terminator and a positive total weight.
/// \returns The selected nonzero key, or zero if the table yields no selection.
/// \note Original decompilation identifier: \c func_800324D8 (0x800324D8).
s32 weightedChoiceSelectKey(WeightedChoiceTable *t) {
    s32 pad[2]; /* unused: sizes the frame to 0x28 */
    WeightedChoiceEntry *e;
    s32 r;

    weightedChoicePrepareTable(t);
    e = t->entries;
    r = randomBelow(e[t->lastChoiceIndex].weight);
    for (; e->key != 0; e++) {
        if (e->weight >= r) {
            return e->key;
        }
    }
    TRACE("invalid array of itemFreqT:%d > %d\n", r, t->entries[t->lastChoiceIndex].weight);
    return 0;
}

/* Save record bytes followed by six cumulative EEPROM block offsets. The
 * two objects occupy 0x7A bytes, naturally rounded to 0x80 by the BSS section. */
/// Packed progress-field record transferred to and from EEPROM.
/// \note Original identifier: \c D_800F24D0 (0x800F24D0).
u8 gProgressSaveRecord[0x74];
/// EEPROM block offsets for the six primary and backup records.
/// \note Original identifier: \c D_800F2544 (0x800F2544).
u8 gEepromRecordBlockOffsets[6];
/// Prepares the EEPROM locations of save records and their backup copies.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032550 (0x80032550).
void progressInitializeEepromRecordLocations(void) {
    EepromRecordSizeTable recordSizes = gEepromRecordPayloadSizes;
    s32 i;
    u32 offset = 0;
    TRACE_LEAF("");
    TRACE_LEAF("");
    TRACE_LEAF("");
    for (i = 0; i < 6; i++) {
        gEepromRecordBlockOffsets[i] = offset;
        TRACE_LEAF(" _eepromIdx[%d] = %d\n", i, offset);
        offset += (u32)(recordSizes.payloadBytes[i] + 11) >> 3;
    }
    TRACE_LEAF("\npreserved data   size: %d\n", offset);
}

/// Gets the backup record associated with an EEPROM record.
/// \param record Record index from zero through five.
/// \returns The backup record index, or minus one for a record without a backup.
/// \note Original decompilation identifier: \c func_80032618 (0x80032618).
s32 progressGetBackupRecordIndex(s32 record) {
    s32 table[6] = {3, 4, 5, -1, -1, -1};

    return table[record];
}

/// Provide an inactive hook for beginning progress-storage initialization.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003266C (0x8003266C).
void progressStorageBeginInitializationHook(void) {
}

/// Provide an inactive hook for finishing progress-field layout setup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032674 (0x80032674).
void progressStorageFinishFieldLayoutHook(void) {
}

/// Location of one field within the owned packed progress-record buffer.
typedef struct ProgressFieldStorage {
    s32 byteOffset; ///< Byte offset from the progress-record start.
    s32 bitShift; ///< Low-bit position within a byte for packed sub-byte fields.
} ProgressFieldStorage;

typedef char ProgressFieldStorageSizeCheck[(sizeof(ProgressFieldStorage) == 8) ? 1 : -1];
typedef char ProgressFieldShiftOffsetCheck[(((u32)&((ProgressFieldStorage *)0)->bitShift) == 4) ? 1 : -1];
/// Byte offsets and bit shifts used to access packed progress fields.
/// \note Original identifier: \c D_800F1D30 (0x800F1D30).
extern ProgressFieldStorage gProgressFieldStorageLayout[];
/// Prepares the storage layout required to read and write saved progress fields.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003267C (0x8003267C).
void progressInitializeFieldStorageLayout(void) {
    s32 bit = 0;
    s32 offset = 0;
    s32 type;
    s32 i;
    s32 width;
    s32 bytes;
    for (type = 0; type < 8; type++) {
        width = gProgressFieldTypeLimits[type].bitWidth;
        if (bit != 0) {
            bit = 0;
            offset++;
        }
        bytes = (width + 7) / 8;
        offset = ((offset + bytes - 1) / bytes) * bytes;
        for (i = 0; i < 244; i++) {
            if (type == gProgressFieldTypes[i]) {
                gProgressFieldStorageLayout[i].bitShift = bit;
                bit += width;
                gProgressFieldStorageLayout[i].byteOffset = offset;
                if (bit >= 8) {
                    offset += bit / 8;
                    bit = 0;
                }
            }
        }
    }
}

/// Reads the saved value of a resolved progress field.
/// \param fieldId Valid resolved progress-field ID.
/// \param type Incoming value is ignored; callers may omit this legacy parameter.
/// \returns The saved field value, or zero for an unsupported field type.
///
/// \note UB: K&R definition; callers pass only idx, so type is a dead parameter reused as a local
/// (the ROM keeps it in a1)
/// \note Original decompilation identifier: \c func_80032830 (0x80032830).
s32 progressReadFieldValue(fieldId, type)
s32 fieldId;
s32 type;
{
    s32 v;
    ProgressFieldStorage *f;

    type = gProgressFieldTypes[fieldId];
    f = &gProgressFieldStorageLayout[fieldId];
    switch (type) {
    case 0:
        return *(s32 *)(gProgressSaveRecord + f->byteOffset);
    case 1:
        return *(s16 *)(gProgressSaveRecord + f->byteOffset);
    case 2:
        return *(u16 *)(gProgressSaveRecord + f->byteOffset);
    case 3:
        return *(s8 *)(gProgressSaveRecord + f->byteOffset);
    case 4:
        return *(u8 *)(gProgressSaveRecord + f->byteOffset);
    case 5:
    case 6:
    case 7:
        v = gProgressSaveRecord[f->byteOffset] >> f->bitShift;
        v &= ((1 << (gProgressFieldTypeLimits[type].bitWidth + 31)) << 1) - 1;
        return v;
    default:
        TRACE("Unknown type:%d\n", type);
        break;
    }
    return 0;
}

/// Stores a value in a resolved progress field within its permitted range.
/// \param fieldId Valid resolved progress-field ID.
/// \param value Requested field value.
/// \returns The value stored after applying the field’s limits.
/// \note Original decompilation identifier: \c func_80032968 (0x80032968).
s32 progressStoreFieldValue(s32 fieldId, s32 value) {
    ProgressFieldTypeLimits *limits = &gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]];
    u8 *save = gProgressSaveRecord;
    ProgressFieldStorage *field = &gProgressFieldStorageLayout[fieldId];
    u8 *byte;
    u8 mask;
    u8 bits;
    if (value < gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]].minimum)
        value = gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]].minimum;
    else if (value > limits->maximum)
        value = limits->maximum;
    switch (gProgressFieldTypes[fieldId]) {
    case 0:
        *(s32 *)(save + field->byteOffset) = value;
        break;
    case 1:
        *(s16 *)(save + field->byteOffset) = value;
        break;
    case 2:
        *(u16 *)(save + field->byteOffset) = value;
        break;
    case 3:
        *(s8 *)(save + field->byteOffset) = value;
        break;
    case 4:
        *(u8 *)(save + field->byteOffset) = value;
        break;
    case 5:
    case 6:
    case 7:
        mask = (((1 << (gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]].bitWidth + 31)) << 1) - 1) << field->bitShift;
        bits = value << field->bitShift;
        byte = &gProgressSaveRecord[field->byteOffset];
        *byte &= ~mask;
        byte = &gProgressSaveRecord[field->byteOffset];
        *byte |= bits;
        break;
    default:
        TRACE("Unknown type:%d\n", gProgressFieldTypes[fieldId]);
        break;
    }
    return value;
}

/// Gets the minimum permitted value for a progress field.
/// \param fieldId Valid progress-field ID.
/// \returns The field’s minimum permitted value.
/// \note Original decompilation identifier: \c func_80032B18 (0x80032B18).
s32 progressGetFieldMinimum(s32 fieldId) {
    ProgressFieldTypeLimits *p = &gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]];

    return p->minimum;
}

/// Gets the maximum permitted value for a progress field.
/// \param fieldId Valid progress-field ID.
/// \returns The field’s maximum permitted value.
/// \note Original decompilation identifier: \c func_80032B48 (0x80032B48).
s32 progressGetFieldMaximum(s32 fieldId) {
    ProgressFieldTypeLimits *p = &gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]];

    return p->maximum;
}

/// Area-indexed field IDs used to resolve area-relative progress selectors.
/// \note Original identifier: \c D_8009AB28 (0x8009AB28).
extern u8 gAreaProgressFieldSelectors[];
/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern s32 gRuntimeCurrentArea;

/// Resolves a progress-field selector for the current area.
/// \param field Progress-field ID or area-relative field selector to resolve for the current area.
/// \returns The resolved field ID, which may be minus one when the selector has no field in this area.
/// \note Original decompilation identifier: \c func_80032B78 (0x80032B78).
s32 progressResolveFieldId(s32 field) {
    if (field & 0x40000000) {
        return *(s32 *)(gAreaProgressFieldSelectors + ((field & 0x3FFFFFFF) * 0x60) + (gRuntimeCurrentArea * 4));
    }
    return field;
}

/// Provide an inactive hook for preparing progress-field layout setup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032BC4 (0x80032BC4).
void progressStoragePrepareFieldLayoutHook(void) {
}

/// Gets the minimum permitted value for a progress field.
/// \param fieldId Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \returns The field’s minimum permitted value.
/// \note Original decompilation identifier: \c func_80032BCC (0x80032BCC).
s32 progressGetMinimum(s32 fieldId) {
    return progressGetFieldMinimum(progressResolveFieldId(fieldId));
}

/// Gets the maximum permitted value for a progress field.
/// \param fieldId Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \returns The field’s maximum permitted value.
/// \note Original decompilation identifier: \c func_80032BF4 (0x80032BF4).
s32 progressGetMaximum(s32 fieldId) {
    return progressGetFieldMaximum(progressResolveFieldId(fieldId));
}

s32 progressReadFieldValue();

/// Gets the saved value of a progress field.
/// \param field Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \returns The current field value.
/// \note Original decompilation identifier: \c __osSpSetPc (0x80032C1C).
s32 progressGetValue(u32 field) {
    return progressReadFieldValue(progressResolveFieldId(field));
}

s32 progressStoreFieldValue(s32, s32);

/// Stores a progress-field value within its permitted range.
/// \param field Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \param value Requested field value.
/// \returns The value stored after applying the field’s limits.
/// \note Original decompilation identifier: \c func_80032C44 (0x80032C44).
s32 progressSetValue(s32 field, s32 value) {
    return progressStoreFieldValue(progressResolveFieldId(field), value);
}

/// Adjusts a progress field by the requested amount within its permitted range.
/// \param field Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \param amount Signed amount to add to the current value.
/// \returns The resulting stored value after applying the field’s limits.
/// \note Original decompilation identifier: \c func_80032C70 (0x80032C70).
s32 progressAddValue(s32 field, s32 amount) {
    return progressSetValue(field, progressGetValue(field) + amount);
}

/// Checks whether a progress-field selector identifies a field in the current area.
/// \param field Progress-field ID or area-relative field selector to resolve for the current area.
/// \returns One if the selector resolves to a field, or zero if it resolves to no field.
/// \note Original decompilation identifier: \c func_80032CA4 (0x80032CA4).
s32 progressHasFieldInCurrentArea(s32 field) {
    if (progressResolveFieldId(field) == -1) {
        return 0;
    }
    return 1;
}

/// Save-control byte transferred independently from the progress records.
/// \note Original identifier: \c D_8009A6F0 (0x8009A6F0).
extern u8 gEepromSaveControl;

/// Requests that save control data be written to EEPROM.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032CD4 (0x80032CD4).
void progressSaveControlDataToEeprom(void) {
    s32 ans = eepromRequestWrite(gEepromRecordBlockOffsets[0], gEepromRecordBlockOffsets[3], &gEepromSaveControl, 1);

    TRACE("pdSaveFCB_EEPROM: eepromWrite ans:%d\n", ans);
}


/// Requests save control data from EEPROM.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032D0C (0x80032D0C).
void progressLoadControlDataFromEeprom(void) {
    s32 ans = eepromRequestRead(gEepromRecordBlockOffsets[0], gEepromRecordBlockOffsets[3], &gEepromSaveControl, 1);

    TRACE("pdLoadFCB_EEPROM: eepromWrite ans:%d\n", ans);
}

/// Separate symbol view of the primary player profile points balance.
/// \note Original identifier: \c D_800EF790 (0x800EF790).
extern s32 gPlayerProfilePoints;
/// Separate symbol view of the primary player friendship-title level.
/// \note Original identifier: \c D_800EF796 (0x800EF796).
extern u8 gPlayerFriendshipLevel;
/// Player status voice count, incremented when a messenger delivers to Pikachu.
/// \note Original identifier: \c D_800EF79C (0x800EF79C).
extern s32 gPlayerVoiceMessageCount;

/// Captures current gameplay progress and requests that its save record be written to EEPROM.
/// \param record Progress record index, one or two, each with a backup record.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032D44 (0x80032D44).
void progressSaveRecordToEeprom(s32 record) {
    s32 ans;

    progressSetValue(1, gPlayerProfilePoints);
    progressSetValue(2, gPlayerFriendshipLevel);
    progressSetValue(3, gPlayerVoiceMessageCount);
    ans =
        eepromRequestWrite(gEepromRecordBlockOffsets[record],
                           gEepromRecordBlockOffsets[progressGetBackupRecordIndex(record)], &gProgressSaveRecord, 0x74);
    TRACE("pdSaveEEPROM: eepromWrite ans:%d\n", ans);
}

s32 progressGetBackupRecordIndex(s32); /* extern */

/// Requests a saved progress record from EEPROM into the progress buffer.
/// \param record Progress record index, one or two, each with a backup record.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032DC4 (0x80032DC4).
void progressLoadRecordFromEeprom(s32 record) {
    s32 ans =
        eepromRequestRead(gEepromRecordBlockOffsets[record],
                          gEepromRecordBlockOffsets[progressGetBackupRecordIndex(record)], gProgressSaveRecord, 0x74);

    TRACE("pdLoadEEPROM: eepromRead ans:%d\n", ans);
}

void audioApplySavedMusicVolume(f32);
void audioApplySavedSoundEffectVolume(f32);

/// Applies the loaded progress record to the active gameplay state and settings.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032E10 (0x80032E10).
void progressApplyLoadedRecord(void) {
    playerSetChaseStage(progressGetValue(0xC));
    gPlayerProfilePoints = progressGetValue(1);
    gPlayerFriendshipLevel = progressGetValue(2);
    gPlayerVoiceMessageCount = progressGetValue(3);
    audioSetOutputMode(2 - progressGetValue(9));
    audioApplySavedMusicVolume(1.0f);
    audioApplySavedSoundEffectVolume(1.0f);
    progressSetChapterAndUnlockMilestones(progressGetValue(0));
}

void _bzero(void *, s32); /* extern */

/// Resets the progress record to its initial gameplay values.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032EAC (0x80032EAC).
void progressResetRecordToDefaults(void) {
    _bzero(&gProgressSaveRecord, 0x74);
    progressSetValue(0x44, 1);
    progressSetValue(0x3B, 1);
    progressSetValue(0x56, 1);
    progressSetValue(0xA, progressGetMaximum(0xA));
    progressSetValue(0xB, progressGetMaximum(0xB) / 2);
}

void progressInitializeEepromRecordLocations(void);

/// Initializes progress storage and prepares a default save record.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032F30 (0x80032F30).
void progressInitializeSaveStorage(void) {
    _bzero(gProgressSaveRecord, 0x74);
    _bzero(gProgressFieldStorageLayout, 0x7A0);
    progressStorageBeginInitializationHook();
    progressStoragePrepareFieldLayoutHook();
    progressInitializeFieldStorageLayout();
    progressStorageFinishFieldLayoutHook();
    progressResetRecordToDefaults();
    progressInitializeEepromRecordLocations();
    gEepromSaveControl = 0;
}
