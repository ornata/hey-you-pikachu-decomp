/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
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

/* The unit's .data tail, ROM 0x9B9C0..0x9BA00. IDO places the globals first,
 * then the initializer image of progressGetBackupRecordIndex's local array (D_8009ADDC), so
 * the layout is: -1 word, progressInitializeEepromRecordLocations's image, progressGetBackupRecordIndex's image, pad.
 * D_8009ADC4 is a struct because IDO 8-aligns arrays and ROM has it at +4. */
typedef struct {
    s32 v[6];
} Unk2F760Image;
s32 D_8009ADC0 = -1;
/// Payload byte counts used to place primary EEPROM records and their backup copies.
/// \note Original identifier: \c D_8009ADC4 (0x8009ADC4).
Unk2F760Image gEepromRecordPayloadSizes = {{1, 0x74, 0x74, 1, 0x74, 0x74}};
typedef struct {
    s32 f0;
    s32 f4;
    s32 f8;
} Unk0C;

/// Storage widths and permitted value ranges indexed by progress field type.
/// \note Original identifier: \c D_8009A6F4 (0x8009A6F4).
extern Unk0C gProgressFieldTypeLimits[];
/// Storage type identifiers indexed by resolved progress field ID.
/// \note Original identifier: \c D_8009A754 (0x8009A754).
extern s32 gProgressFieldTypes[];
/// Owned toolbox display state, item rows, placeholders and special storage slots.
/// \note Original identifier: \c D_800F1BE0 (0x800F1BE0).
extern s32 gToolboxState;

typedef struct {
    u8 pad0[0x71];
    s8 f71;
    s16 f72;
} Unk74;
/// Per-kind item motion, rendering, response and description configuration.
/// \note Original identifier: \c D_80125A40 (0x80125A40).
extern Unk74 gItemKinds[];
s32 progressSetValue(s32, s32);

void toolboxScaleCurrentRow(s32 *);           /* extern */
u8 *toolboxTakeObject(s32, s32); /* extern */

void vector3FromPitchYawDegrees(f32, f32, f32 *);
void vector3PackNormalBytes(f32 *, u8 *);

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
    u16 flag = *(u16 *)(animation + 0x72);
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

    for (i = 0, s = (DirectionAngularMotion *)(animation + 0x50); i < 2; i++, s++) {
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

typedef struct {
    f32 x, y, z, w;
} Rec16;
typedef struct {
    f32 x, y, z;
} V3_32148;
/// Interior view of the two packed directions in the default direction-animation pose.
/// \note Original identifier: \c D_8009A6C0 (0x8009A6C0).
extern Rec16 gDirectionAnimationDefaultDirections[2];
void vector3UnpackNormalBytes();
void vector3EvaluateOppositeLookAngles();
f32 mathWrapSignedDegrees(f32);

/// Advances direction motion and periodically retargets it toward the default pose.
/// \param animation Direction animation state to update.
/// \param poseIndex Selected pose to update, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032148 (0x80032148).
void directionAnimationUpdateRetargetingMotion(u8 *animation, s32 poseIndex) {
    DirectionAngularMotion *s;
    s32 i;
    V3_32148 dir;
    f32 a;
    f32 b;

    *(f32 *)(animation + 0x78) -= gRuntimeFrameSeconds;
    if (*(f32 *)(animation + 0x78) < 0.0f) {
        *(s16 *)(animation + 0x70) = 2;
        *(f32 *)(animation + 0x78) = 0.5f;
        for (i = 0, s = (DirectionAngularMotion *)(animation + 0x50); i < 2; i++, s++) {
            vector3UnpackNormalBytes(&gDirectionAnimationDefaultDirections[i], &dir);
            vector3EvaluateOppositeLookAngles(&dir, &a, &b);
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

typedef struct Blk28 {
    s32 w[10];
} Blk28;
/// Default pose shared by both direction-animation pose slots.
/// \note Original identifier: \c D_8009A6B0 (0x8009A6B0).
extern Blk28 gDirectionAnimationDefaultPose;

/// Restores the default direction poses and leaves the animation stopped.
/// \param animation Direction animation state to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800322B8 (0x800322B8).
void directionAnimationReset(u8 *animation) {
    Blk28 t = gDirectionAnimationDefaultPose;

    *(Blk28 *)(animation + 0x28) = t;
    *(Blk28 *)animation = t;
    *(s16 *)(animation + 0x70) = 0;
    *(s16 *)(animation + 0x72) = 0;
}

/// Stops direction animation while preserving its current pose.
/// \param animation Direction animation state to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003237C (0x8003237C).
void directionAnimationStop(u8 *animation) {
    ((DirectionAnimation *)animation)->mode = 0;
}

typedef struct Vec3_32384 {
    f32 x, y, z;
} Vec3_32384;
typedef struct Frame28 {
    s8 data[0x28];
} Frame28;
void vector3UnpackNormalBytes(s8 *, Vec3_32384 *);
void vector3EvaluateOppositeLookAngles(Vec3_32384 *, f32 *, f32 *);

/// Starts timed direction motion from the current pose.
/// \param animation Direction animation state to configure.
/// \param duration Nonnegative duration of the motion in seconds.
/// \param rates Two pairs of pitch and yaw angular rates in degrees per second.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032384 (0x80032384).
void directionAnimationStartMotion(u8 *animation, f32 duration, DirectionAngularRates *rates) {
    s32 pad[2]; /* unused, sizes the frame */
    Vec3_32384 v;
    f32 *rec;
    s32 off;
    DirectionAngularRates *p;

    *(s16 *)(animation + 0x70) = 1;
    rec = (f32 *)(animation + 0x50);
    off = 0;
    p = rates;
    *(f32 *)(animation + 0x74) = duration;
    *(f32 *)(animation + 0x78) = duration;
    do {
        vector3UnpackNormalBytes(&((Frame28 *)(animation + 0x10))[*(u16 *)(animation + 0x72)].data[off], &v);
        vector3EvaluateOppositeLookAngles(&v, rec, rec + 1);
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

    motionKind = *(u16 *)(animation + 0x70);
    if (motionKind != 0) {
        *(u16 *)(animation + 0x72) ^= 1;
        gDirectionAnimationUpdateHandlers[motionKind](animation, *(u16 *)(animation + 0x72));
    }
}

typedef struct {
    s32 key;
    s32 off;
} OffEntry;
typedef struct {
    OffEntry *entries;
    s32 count;
} OffTable;
/// Prepares a weighted selection table for repeated random choices.
/// \param t Mutable selection table with a zero-key terminator; prepared tables are left unchanged.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032490 (0x80032490).
void weightedChoicePrepareTable(OffTable *t) {
    s32 n = 0;
    OffEntry *e;

    if (t->count == 0) {
        e = t->entries + 1;
        while (1) {
            e->off += e[-1].off;
            e++;
            if (e[-1].key == 0)
                break;
            n++;
        }
        t->count = n;
    }
}

u32 randomBelow(s32);

/// Chooses a key from a weighted selection table, preparing the table when needed.
/// \param t Mutable selection table with a zero-key terminator and a positive total weight.
/// \returns The selected nonzero key, or zero if the table yields no selection.
/// \note Original decompilation identifier: \c func_800324D8 (0x800324D8).
s32 weightedChoiceSelectKey(OffTable *t) {
    s32 pad[2]; /* unused: sizes the frame to 0x28 */
    OffEntry *e;
    s32 r;

    weightedChoicePrepareTable(t);
    e = t->entries;
    r = randomBelow(e[t->count].off);
    for (; e->key != 0; e++) {
        if (e->off >= r) {
            return e->key;
        }
    }
    TRACE("invalid array of itemFreqT:%d > %d\n", r, t->entries[t->count].off);
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
    Unk2F760Image image = gEepromRecordPayloadSizes;
    s32 i;
    u32 offset = 0;
    TRACE_LEAF("");
    TRACE_LEAF("");
    TRACE_LEAF("");
    for (i = 0; i < 6; i++) {
        gEepromRecordBlockOffsets[i] = offset;
        TRACE_LEAF(" _eepromIdx[%d] = %d\n", i, offset);
        offset += (u32)(image.v[i] + 11) >> 3;
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

typedef struct {
    s32 off;
    s32 shift;
} Unk8;
/// Byte offsets and bit shifts used to access packed progress fields.
/// \note Original identifier: \c D_800F1D30 (0x800F1D30).
extern Unk8 gProgressFieldStorageLayout[];
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
        width = gProgressFieldTypeLimits[type].f0;
        if (bit != 0) {
            bit = 0;
            offset++;
        }
        bytes = (width + 7) / 8;
        offset = ((offset + bytes - 1) / bytes) * bytes;
        for (i = 0; i < 244; i++) {
            if (type == gProgressFieldTypes[i]) {
                gProgressFieldStorageLayout[i].shift = bit;
                bit += width;
                gProgressFieldStorageLayout[i].off = offset;
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
    Unk8 *f;

    type = gProgressFieldTypes[fieldId];
    f = &gProgressFieldStorageLayout[fieldId];
    switch (type) {
    case 0:
        return *(s32 *)(gProgressSaveRecord + f->off);
    case 1:
        return *(s16 *)(gProgressSaveRecord + f->off);
    case 2:
        return *(u16 *)(gProgressSaveRecord + f->off);
    case 3:
        return *(s8 *)(gProgressSaveRecord + f->off);
    case 4:
        return *(u8 *)(gProgressSaveRecord + f->off);
    case 5:
    case 6:
    case 7:
        v = gProgressSaveRecord[f->off] >> f->shift;
        v &= ((1 << (gProgressFieldTypeLimits[type].f0 + 31)) << 1) - 1;
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
    Unk0C *limits = &gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]];
    u8 *save = gProgressSaveRecord;
    Unk8 *field = &gProgressFieldStorageLayout[fieldId];
    u8 *byte;
    u8 mask;
    u8 bits;
    if (value < gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]].f4)
        value = gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]].f4;
    else if (value > limits->f8)
        value = limits->f8;
    switch (gProgressFieldTypes[fieldId]) {
    case 0:
        *(s32 *)(save + field->off) = value;
        break;
    case 1:
        *(s16 *)(save + field->off) = value;
        break;
    case 2:
        *(u16 *)(save + field->off) = value;
        break;
    case 3:
        *(s8 *)(save + field->off) = value;
        break;
    case 4:
        *(u8 *)(save + field->off) = value;
        break;
    case 5:
    case 6:
    case 7:
        mask = (((1 << (gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]].f0 + 31)) << 1) - 1) << field->shift;
        bits = value << field->shift;
        byte = &gProgressSaveRecord[field->off];
        *byte &= ~mask;
        byte = &gProgressSaveRecord[field->off];
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
    Unk0C *p = &gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]];

    return p->f4;
}

/// Gets the maximum permitted value for a progress field.
/// \param fieldId Valid progress-field ID.
/// \returns The field’s maximum permitted value.
/// \note Original decompilation identifier: \c func_80032B48 (0x80032B48).
s32 progressGetFieldMaximum(s32 fieldId) {
    Unk0C *p = &gProgressFieldTypeLimits[gProgressFieldTypes[fieldId]];

    return p->f8;
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
/// \param record Primary EEPROM record index from zero through two, each with a backup record.
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
/// \param record Primary EEPROM record index from zero through two, each with a backup record.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032DC4 (0x80032DC4).
void progressLoadRecordFromEeprom(s32 record) {
    s32 ans =
        eepromRequestRead(gEepromRecordBlockOffsets[record],
                          gEepromRecordBlockOffsets[progressGetBackupRecordIndex(record)], gProgressSaveRecord, 0x74);

    TRACE("pdLoadEEPROM: eepromRead ans:%d\n", ans);
}

void playerSetChaseStage(s32);
void audioApplySavedMusicVolume(f32);
void audioApplySavedSoundEffectVolume(f32);
void audioSetOutputMode(s32);
void progressSetChapterAndUnlockMilestones(s32);

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
void progressResetRecordToDefaults(void);

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
