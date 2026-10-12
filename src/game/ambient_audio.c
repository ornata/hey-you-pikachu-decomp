#include "camera_types.h"
#include "audio_spatial.h"
#include "object_create.h"
#include "object_lifecycle.h"
#include "heap_free.h"
/* Spatial audio emitters and effects. */
#include "common.h"
#include "audio_cue.h"
#include "heap_alloc.h"
#include "object_types.h"
s32 audioStopPlayback(s32, f64);
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/* One source line on purpose: IDO schedules per line, and the ROM's
 * register order only comes out when the three copies share one. */
#define VEC3F_COPY(dst, src)                                                                                           \
    (dst).x = (src).x;                                                                                                 \
    (dst).y = (src).y;                                                                                                 \
    (dst).z = (src).z
typedef struct {
    f32 x, y, z;
} Vec3fS;
typedef struct {
    s32 unk0;
    s32 voice;
    f32 delay;
    f32 delayRange;
    s32 first;
    s32 second;
    f32 probability;
    s32 count;
    void *src;
    f32 timer;
} Ext347D0;
typedef struct {
    u8 pad0[0x18];
    Vec3fS pos;
    u8 pad24[0x20];
    Vec3fS prev;
    u8 pad50[4];
    Ext347D0 *ext;
} Obj347D0;
s32 audioCountPlayback(s32);
void audioSetSoundEffectVolume(s32, s32);
void audioSetSoundEffectPan(s32, s32);
u32 randomNext(void);
s32 audioPlaySoundEffect(s32);
/// Start the next ambient sound and schedule the next playback.
/// \param ext Ambient emitter state to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033BD0 (0x80033BD0).
void ambientAudioPlayNextSound(Ext347D0 *ext) {
    s32 sound;
    ext->timer = randomNext() * (1.0 / 4294967296.0) * ext->delayRange + ext->delay;
    sound = randomNext() * (1.0 / 4294967296.0) < ext->probability ? ext->first : ext->second;
    switch (ext->unk0) {
    case 0:
        audioStopPlayback(ext->voice, 0.0);
        ext->voice = audioPlaySoundEffect(sound);
        break;
    case 1:
        ext->voice = audioPlayCue(sound);
        break;
    case 2:
        ext->voice = audioChangeCurrentCue(sound, 1.0, 1.0);
        break;
    }
}
/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern MatrixCamera gRenderCameras[];
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
/// Gain applied to the combined spatial volume of an ambient emitter.
/// \note Original identifier: \c D_800A61E8 (0x800A61E8).
extern f32 gAmbientAudioMixGain;

/// Determine the volume and stereo pan for an ambient emitter.
/// \param ext Emitter state with a positive source count and valid source positions.
/// \param volume Receives the combined source volume, capped at 256.
/// \param pan Receives the stereo pan value, capped at 256.
/// \returns Nothing.
/// \note A zero total source volume produces an undefined pan conversion;
/// callers must ensure that the sources have a nonzero combined volume.
/// \note Original decompilation identifier: \c func_80033D20 (0x80033D20).
void ambientAudioGetSpatialMix(Ext347D0 *ext, s32 *volume, s32 *pan) {
    s32 left;
    s32 right;
    Vec3fS *pos;
    s32 i;
    s32 sampleVolume;
    s32 samplePan;
    left = 0;
    right = 0;
    /* IDO schedules the index and source cursor initialization together. */
    // clang-format off
    i = 0; pos = ext->src;
    // clang-format on
    for (; i < ext->count; i++, pos++) {
        audioGetPositionVolumeAndPan(&pos->x, *gRenderCameras[gRenderCameraIndex].matrices, &sampleVolume, &samplePan,
                                     2.0f, 100.0f);
        left += sampleVolume * samplePan / 256;
        right += sampleVolume - sampleVolume * samplePan / 256;
    }
    *volume = (left + right) / sqrtf(ext->count) * gAmbientAudioMixGain;
    *pan = (f32)left / (left + right) * 256.0f;
    if (*volume > 256)
        *volume = 256;
    if (*pan > 256)
        *pan = 256;
}

void ambientAudioGetSpatialMix(Ext347D0 *, s32 *, s32 *);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance an ambient emitter and update its active playback.
/// \param obj Ambient emitter object to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033F20 (0x80033F20).
void ambientAudioUpdate(Obj347D0 *obj) {
    Ext347D0 *ext;
    s32 sp28;
    s32 sp24;

    ext = obj->ext;
    VEC3F_COPY(obj->prev, obj->pos);
    ext->timer -= gRuntimeFrameSeconds;
    if (ext->timer <= 0.0f) {
        ambientAudioPlayNextSound(ext);
    }
    if (ext->voice != 0) {
        if (audioCountPlayback(ext->voice) == 0) {
            ext->voice = 0;
            return;
        }
        if (ext->count > 0 && ext->src != NULL) {
            ambientAudioGetSpatialMix(ext, &sp28, &sp24);
            audioSetSoundEffectPan(ext->voice, sp24);
            audioSetSoundEffectVolume(ext->voice, sp28);
        }
    }
}

void _bzero(u8 *, s32);

/// Initialize the ambient state of an emitter object.
/// \param object Ambient emitter object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033FF4 (0x80033FF4).
void ambientAudioInitialize(u8 *object) {
    u8 *sp1C;

    sp1C = heapAllocLocal(&gGameHeap, 0x28, 0x8E);
    _bzero(sp1C, 0x28);
    ((Ext347D0 *)sp1C)->count = 1;
    ((Ext347D0 *)sp1C)->src = object + 0x18;
    ((RuntimeObjectPrefix *)object)->state.bytes = sp1C;
}

/// Stop an ambient emitter and release its state.
/// \param object Ambient emitter object whose state is released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80034050 (0x80034050).
void ambientAudioDestroy(u8 *object) {
    u8 *soundState = *(u8 **)(object + 0x54);
    audioStopPlayback(*(s32 *)(soundState + 4), 0.0);
    heapFreeLocal(&gGameHeap, soundState);
}

/// Object template used to create ambient audio emitters.
/// \note Original identifier: \c D_8009B350 (0x8009B350).
extern u8 gAmbientAudioEmitterTemplate[];

/// Create an ambient audio emitter with the requested playback and spatial settings.
/// \param position Three-component world-space emitter position.
/// \param kind Playback category: 0 for sound effects, 1 to play a cue, or 2 to
/// change the current cue.
/// \param delay Minimum delay between ambient sounds, in seconds.
/// \param delayRange Additional randomized delay range, in seconds.
/// \param firstSound First candidate sound identifier.
/// \param secondSound Second candidate sound identifier.
/// \param firstProbability Probability of choosing the first candidate sound.
/// \param sourceCount Number of source positions; a negative value together with
/// a zero sourcePositions retains the emitter position as the sole source.
/// \param sourcePositions Address of the source-position array, which must remain
/// valid while the emitter uses it. Zero with a negative sourceCount selects the
/// default source; zero with a nonnegative count disables spatial mixing.
/// \returns The created ambient emitter object.
/// \note Original decompilation identifier: \c func_80034094 (0x80034094).
u8 *ambientAudioCreate(f32 *position, s32 kind, f32 delay, f32 delayRange, s32 firstSound, s32 secondSound,
                       f32 firstProbability, s32 sourceCount, s32 sourcePositions) {
    u8 *obj = objectCreateFromTemplate((ObjectTemplate *)gAmbientAudioEmitterTemplate);
    u8 *fx;

    *(u16 *)(obj + 0x36) |= 0x100;
    fx = *(u8 **)(obj + 0x54);
    *(f32 *)(obj + 0x18) = position[0];
    *(f32 *)(obj + 0x1C) = position[1];
    *(f32 *)(obj + 0x20) = position[2];
    *(f32 *)(obj + 0x44) = position[0];
    *(f32 *)(obj + 0x48) = position[1];
    *(f32 *)(obj + 0x4C) = position[2];
    *(s32 *)(fx + 0) = kind;
    *(f32 *)(fx + 8) = delay;
    *(f32 *)(fx + 0xC) = delayRange;
    *(s32 *)(fx + 0x10) = firstSound;
    *(s32 *)(fx + 0x14) = secondSound;
    *(f32 *)(fx + 0x18) = firstProbability;
    if (delay > 30.0f) {
        *(f32 *)(fx + 0x24) = 0.0f;
    } else {
        *(f32 *)(fx + 0x24) = delay / 2;
    }
    if (sourceCount >= 0 || sourcePositions != 0) {
        *(s32 *)(fx + 0x1C) = sourceCount;
        *(s32 *)(fx + 0x20) = sourcePositions;
    }
    objectMakeHierarchyRoot(obj);
    objectRegisterActiveHierarchy(obj);
    return obj;
}
