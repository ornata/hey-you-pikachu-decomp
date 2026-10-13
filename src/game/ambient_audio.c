#include "game_heap.h"
#include "camera_storage.h"
#include "random.h"
#include "audio_sound_effect.h"
#include "audio_playback_stop.h"
#include "ambient_audio.h"
#include "audio_playback_query.h"
#include "vector3_types.h"
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

/* One source line on purpose: IDO schedules per line, and the ROM's
 * register order only comes out when the three copies share one. */
#define VEC3F_COPY(dst, src)                                                                                           \
    (dst)[0] = (src)[0];                                                                                                 \
    (dst)[1] = (src)[1];                                                                                                 \
    (dst)[2] = (src)[2]
/// Owned ambient playback state borrowing its array of spatial source positions.
typedef struct AmbientAudioState {
    s32 playbackKind; ///< AmbientAudioPlaybackKind routing selector.
    s32 voice;        ///< Active playback handle, or zero.
    f32 delay;        ///< Minimum interval between playback starts in seconds.
    f32 delayRange;   ///< Additional randomized interval range in seconds.
    s32 first;        ///< First candidate sound or cue identifier.
    s32 second;       ///< Second candidate sound or cue identifier.
    f32 probability;  ///< Probability of selecting the first candidate.
    s32 count;        ///< Number of borrowed spatial source positions.
    Vec3f *src;       ///< Borrowed source positions, or null to disable spatial mixing.
    f32 timer;        ///< Seconds remaining before the next playback start.
} AmbientAudioState;

typedef char AmbientAudioStateSizeCheck[(sizeof(AmbientAudioState) == 0x28) ? 1 : -1];
#define AMBIENT_STATE_OFFSET_CHECK(field, offset)                                                                      \
    typedef char AmbientAudio_##field##OffsetCheck[(((u32) & ((AmbientAudioState *)0)->field) == (offset)) ? 1 : -1]
AMBIENT_STATE_OFFSET_CHECK(voice, 0x4);
AMBIENT_STATE_OFFSET_CHECK(delay, 0x8);
AMBIENT_STATE_OFFSET_CHECK(delayRange, 0xC);
AMBIENT_STATE_OFFSET_CHECK(first, 0x10);
AMBIENT_STATE_OFFSET_CHECK(second, 0x14);
AMBIENT_STATE_OFFSET_CHECK(probability, 0x18);
AMBIENT_STATE_OFFSET_CHECK(count, 0x1C);
AMBIENT_STATE_OFFSET_CHECK(src, 0x20);
AMBIENT_STATE_OFFSET_CHECK(timer, 0x24);
#undef AMBIENT_STATE_OFFSET_CHECK
/// Start the next ambient sound and schedule the next playback.
/// \param ext Ambient emitter state to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033BD0 (0x80033BD0).
void ambientAudioPlayNextSound(AmbientAudioState *ext) {
    s32 sound;
    ext->timer = randomNext() * (1.0 / 4294967296.0) * ext->delayRange + ext->delay;
    sound = randomNext() * (1.0 / 4294967296.0) < ext->probability ? ext->first : ext->second;
    switch (ext->playbackKind) {
    case AMBIENT_AUDIO_SOUND_EFFECT:
        audioStopPlayback(ext->voice, 0.0);
        ext->voice = audioPlaySoundEffect(sound);
        break;
    case AMBIENT_AUDIO_PLAY_CUE:
        ext->voice = audioPlayCue(sound);
        break;
    case AMBIENT_AUDIO_CHANGE_CUE:
        ext->voice = audioChangeCurrentCue(sound, 1.0, 1.0);
        break;
    }
}
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
void ambientAudioGetSpatialMix(AmbientAudioState *ext, s32 *volume, s32 *pan) {
    s32 left;
    s32 right;
    Vec3f *pos;
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

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance an ambient emitter and update its active playback.
/// \param obj Ambient emitter object to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033F20 (0x80033F20).
void ambientAudioUpdate(RuntimeObjectPrefix *obj) {
    AmbientAudioState *ext;
    s32 volume;
    s32 pan;

    ext = obj->state.pointer;
    VEC3F_COPY(obj->worldPosition, obj->position);
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
            ambientAudioGetSpatialMix(ext, &volume, &pan);
            audioSetSoundEffectPan(ext->voice, pan);
            audioSetSoundEffectVolume(ext->voice, volume);
        }
    }
}

void _bzero(u8 *, s32);

/// Initialize the ambient state of an emitter object.
/// \param object Ambient emitter object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033FF4 (0x80033FF4).
void ambientAudioInitialize(u8 *object) {
    AmbientAudioState *state;

    state = heapAllocLocal(&gGameHeap, sizeof(AmbientAudioState), 0x8E);
    _bzero((u8 *)state, sizeof(AmbientAudioState));
    state->count = 1;
    state->src = (Vec3f *)((RuntimeObjectPrefix *)object)->position;
    ((RuntimeObjectPrefix *)object)->state.pointer = state;
}

/// Stop an ambient emitter and release its state.
/// \param object Ambient emitter object whose state is released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80034050 (0x80034050).
void ambientAudioDestroy(u8 *object) {
    AmbientAudioState *soundState = ((RuntimeObjectPrefix *)object)->state.pointer;
    audioStopPlayback(soundState->voice, 0.0);
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
/// a null sourcePositions retains the emitter position as the sole source.
/// \param sourcePositions Borrowed source-position array, which must remain
/// valid while the emitter uses it. Null with a negative sourceCount selects the
/// default source; null with a nonnegative count disables spatial mixing.
/// \returns The created ambient emitter object.
/// \note Original decompilation identifier: \c func_80034094 (0x80034094).
u8 *ambientAudioCreate(f32 *position, s32 kind, f32 delay, f32 delayRange, s32 firstSound, s32 secondSound,
                       f32 firstProbability, s32 sourceCount, Vec3f *sourcePositions) {
    u8 *obj = objectCreateFromTemplate((ObjectTemplate *)gAmbientAudioEmitterTemplate);
    AmbientAudioState *fx;

    ((RuntimeObjectPrefix *)obj)->flags |= 0x100;
    fx = ((RuntimeObjectPrefix *)obj)->state.pointer;
    ((RuntimeObjectPrefix *)obj)->position[0] = position[0];
    ((RuntimeObjectPrefix *)obj)->position[1] = position[1];
    ((RuntimeObjectPrefix *)obj)->position[2] = position[2];
    ((RuntimeObjectPrefix *)obj)->worldPosition[0] = position[0];
    ((RuntimeObjectPrefix *)obj)->worldPosition[1] = position[1];
    ((RuntimeObjectPrefix *)obj)->worldPosition[2] = position[2];
    fx->playbackKind = kind;
    fx->delay = delay;
    fx->delayRange = delayRange;
    fx->first = firstSound;
    fx->second = secondSound;
    fx->probability = firstProbability;
    if (delay > 30.0f) {
        fx->timer = 0.0f;
    } else {
        fx->timer = delay / 2;
    }
    if (sourceCount >= 0 || sourcePositions != NULL) {
        fx->count = sourceCount;
        fx->src = sourcePositions;
    }
    objectMakeHierarchyRoot(obj);
    objectRegisterActiveHierarchy(obj);
    return obj;
}
