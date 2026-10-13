#include "wisp_effect.h"
#include "ribbon_effect.h"
#include "particle_cluster.h"
#include "animated_radial_effect.h"
#include "flickering_effect.h"
#include "converging_particle.h"
#include "falling_particle.h"
#include "effect_emitter.h"
#include "linked_effect.h"
#include "attached_effect.h"
#include "item_participation.h"
#include "item_search.h"
#include "actor_action_cue.h"
#include "actor_appearance.h"
#include "actor_part_texture.h"
#include "actor_sound.h"
#include "item_object.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "direction_animation.h"
#include "item_response.h"
#include "runtime_clock.h"
#include "actor_parts.h"
#include "object_audio.h"
#include "audio_cue.h"
#include "actor_target.h"
#include "object_lookup.h"
#include "object_types.h"
#include "actor_script.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata and the dead branch still ends a scheduling block. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/* XYZ prefix of the player state; the larger object remains opaque here. */
typedef struct {
    f32 x, y, z;
} PlayerPosition23;
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerPosition23 gPlayerState;


/// Common scalar argument pair in action bytecode; interpretation depends on the opcode.
typedef struct ActionScalarArguments {
    s32 integer; ///< Integer selector or resource identifier supplied by the command.
    f32 scalar;  ///< Floating argument supplied by the command.
} ActionScalarArguments;
typedef char ActionScalarArgumentsSizeCheck[(sizeof(ActionScalarArguments) == 8) ? 1 : -1];

/* Orphan format string of actionCommandLaunchHeldItem's compiled-out default-case trace. */
static const char sUnknownThrowDirFmt[] = "Unknown throw_dir:%d\n";

/// Selected audio cue identifier, or minus one when no cue is selected.
/// \note Original identifier: \c D_80089474 (0x80089474).
extern s32 gCurrentAudioCueId;
/// Requested playback-rate multiplier for the selected audio cue.
/// \note Original identifier: \c D_80089480 (0x80089480).
extern f64 gCurrentAudioCuePlaybackRate;

/// Gets the selected audio cue and its requested playback rate.
/// \param cue Receives the selected cue ID, or minus one when no cue is selected.
/// \param rate Receives the requested playback rate multiplier.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80023130 (0x80023130).
s32 audioGetCurrentCueState(s32 *cue, f64 *rate) {
    *cue = gCurrentAudioCueId;
    *rate = gCurrentAudioCuePlaybackRate;
    return 0;
}

/// Playback handle for the selected audio cue, or zero when inactive.
/// \note Original identifier: \c D_80089470 (0x80089470).
extern s32 gCurrentAudioCuePlaybackHandle;
s32 audioPlaySequenceWithFade(s32, f64);
/// ROM address of the selected audio cue bank.
/// \note Original identifier: \c D_80089478 (0x80089478).
extern s32 gAudioCueBankRomAddress;
s32 audioFadeOutSequence(s32, f64); /* extern */

/// Transitions from the current audio cue to a new cue or silence.
/// \param cue Valid cue ID in the selected bank, or a negative value to stop the current cue.
/// \param fadeIn Nonnegative new cue fade-in duration in seconds.
/// \param fadeOut Nonnegative previous cue fade-out duration in seconds.
/// \returns The new playback identifier, or zero for silence or unavailable playback capacity.
/// \note Original decompilation identifier: \c func_80023150 (0x80023150).
s32 audioChangeCurrentCue(s32 cue, f64 fadeIn, f64 fadeOut) {
    audioFadeOutSequence(gCurrentAudioCuePlaybackHandle, fadeOut);
    if (cue >= 0) {
        gCurrentAudioCueId = cue;
        gCurrentAudioCuePlaybackHandle =
            audioPlaySequenceWithFade(audioGetCueDataRomAddress(gAudioCueBankRomAddress, cue), fadeIn);
        gCurrentAudioCuePlaybackRate = 1.0;
    } else {
        gCurrentAudioCueId = -1;
        gCurrentAudioCuePlaybackHandle = 0;
    }
    return gCurrentAudioCuePlaybackHandle;
}

s32 audioSetSequenceRate(s32, f64);

/// Records the requested playback rate and applies it to the current cue when that cue permits rate changes.
/// \param rate Positive playback rate multiplier; one uses the original rate.
/// \returns One if the playback rate was applied, or zero if no playback was updated.
/// \note Original decompilation identifier: \c func_800231F4 (0x800231F4).
s32 audioSetCurrentCueRate(f64 rate) {
    gCurrentAudioCuePlaybackRate = rate;
    if (gCurrentAudioCueId == 2) {
        return 0;
    }
    return audioSetSequenceRate(gCurrentAudioCuePlaybackHandle, gCurrentAudioCuePlaybackRate);
}


/// Starts an audio cue at full volume.
/// \param cue Valid cue ID in the initialized current audio cue bank.
/// \returns The playback identifier, or zero if no sequence slot is available.
/// \note Original decompilation identifier: \c func_80023244 (0x80023244).
s32 audioPlayCue(s32 cue) {
    return audioPlaySequenceWithFade(audioGetCueDataRomAddress(gAudioCueBankRomAddress, cue), 0.0);
}

/// Private buffered ROM reader, valid only in sixteen-byte-aligned storage.
/// The cached base uses the original signed address comparison and empty sentinel.
typedef struct RomWindow {
    u8 data[0x400]; ///< Embedded transfer storage, borrowed by the aligned buffer view.
    s32 base; ///< Cartridge address of the first cached byte, or 0x80000000 when empty.
    u8 *buf; ///< Aligned view of data, valid for the reader lifetime.
    OSMesgQueue mq; ///< Completion queue for the current blocking transfer.
    OSMesg msg[1]; ///< Embedded queue backing storage.
} RomWindow;

typedef char RomWindowSizeCheck[(sizeof(RomWindow) == 0x424) ? 1 : -1];
typedef char RomWindowQueueOffsetCheck[(((u32)&((RomWindow *)0)->mq) == 0x408) ? 1 : -1];
typedef char RomWindowMessageOffsetCheck[(((u32)&((RomWindow *)0)->msg) == 0x420) ? 1 : -1];

s32 romWindowInitialize(RomWindow *); /* extern */
/// Buffered ROM reader used to resolve audio cue sequence addresses.
/// \note Original identifier: \c D_800F0290 (0x800F0290).
extern RomWindow gAudioCueBankRomReader;

/// Selects the audio cue bank and prepares its ROM reader.
/// \param bank ROM address of the audio cue bank.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_8002327C (0x8002327C).
s32 audioSelectCueBank(s32 bank) {
    gAudioCueBankRomAddress = bank;
    romWindowInitialize(&gAudioCueBankRomReader);
    return 0;
}

u8 romWindowReadByte(RomWindow *, s32);

/// Gets the ROM address of a cue’s sequence data. Requires the cue-bank ROM reader to be initialized.
/// \param base ROM address of the cue bank.
/// \param index Valid cue index within the bank.
/// \returns The ROM address of the selected cue’s sequence data.
/// \note Original decompilation identifier: \c func_800232AC (0x800232AC).
s32 audioGetCueDataRomAddress(s32 base, s32 index) {
    u32 offset;
    u8 high;
    u8 middle;
    u8 low;
    u32 entry;

    romWindowReadByte(&gAudioCueBankRomReader, base);
    romWindowReadByte(&gAudioCueBankRomReader, base + 1);
    romWindowReadByte(&gAudioCueBankRomReader, base + 2);
    romWindowReadByte(&gAudioCueBankRomReader, base + 3);
    offset = index * 8;
    entry = base + offset;
    low = romWindowReadByte(&gAudioCueBankRomReader, entry + 6);
    middle = romWindowReadByte(&gAudioCueBankRomReader, entry + 5);
    high = romWindowReadByte(&gAudioCueBankRomReader, entry + 4);
    offset =
        (romWindowReadByte(&gAudioCueBankRomReader, entry + 7) | ((u32)high << 0x18) | (middle << 0x10) | (low << 8));
    return base + offset;
}

/// Initializes a ROM reader for subsequent byte reads.
/// \param window Sixteen-byte-aligned storage of at least 1060 bytes, valid throughout subsequent reads.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80023388 (0x80023388).
s32 romWindowInitialize(RomWindow *window) {
    window->base = 0x80000000;
    window->buf = (u8 *)((((u32)window + 0xF) >> 4) * 0x10);
    osCreateMesgQueue(&window->mq, window->msg, 1);
    return 0;
}

/// Reads a byte from ROM, waiting for any required transfer to finish.
/// \param w Initialized ROM reader to use.
/// \param addr ROM address of the byte to read; the containing transfer window of 1024 bytes must be readable.
/// \returns The byte at the requested ROM address.
/// \note Original decompilation identifier: \c func_800233D0 (0x800233D0).
u8 romWindowReadByte(RomWindow *w, s32 addr) {
    OSIoMesg io;
    OSMesg msg;

    if (addr < w->base || addr >= w->base + 0x400) {
        w->base = addr & ~0xF;
        osInvalDCache(w->buf, 0x400);
        osPiStartDma(&io, OS_MESG_PRI_HIGH, OS_READ, w->base, w->buf, 0x400, &w->mq);
        osRecvMesg(&w->mq, &msg, OS_MESG_BLOCK);
    }
    return *(w->buf + addr - w->base);
}


/// Configures an actor part animation from an action command.
/// \param actor Actor with an initialized tinted model part whose texture animation is configured.
/// \param state Unused action-handler context.
/// \param command Command specifying a valid animation, nonnegative playback speed, and texture channel from 0 through 2.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023480 (0x80023480).
void actionCommandConfigureActorPartAnimation(u8 *actor, u8 *state, u8 *command, f32 *elapsed) {
    actorSetPartTextureAnimation(actor, *(f32 *)(command + 0xC), *(s32 *)(command + 0x10), *(s32 *)(command + 8));
}


/// Changes the animation of the actor’s held item when that item is available.
/// \param actor Unused actor argument.
/// \param state Actor state identifying the held item.
/// \param command Action command specifying the item animation ID.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800234B8 (0x800234B8).
void actionCommandChangeHeldItemAnimation(s32 actor, u8 *state, u8 *command, f32 *elapsed) {
    u8 *liveObject;

    liveObject = objectFindLiveByHandle(((ActorScriptState *)state)->heldItemHandle);
    if (liveObject != 0) {
        itemStartMaterialAnimation(liveObject, ((ActionScalarArguments *)((u8 *)command + 8))->integer);
    }
}

void audioSetSoundEffectPitch(s32, f32);

/// Plays a tracked actor sound with the pitch requested by an action command.
/// \param actor Actor to receive the tracked sound.
/// \param state Unused action-handler context.
/// \param command Action command specifying the sound ID and pitch.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800234F8 (0x800234F8).
void actionCommandPlayTrackedSoundWithPitch(s32 actor, u8 *state, u8 *command, f32 *elapsed) {
    audioSetSoundEffectPitch(actorPlayTrackedSound(actor, ((ActionScalarArguments *)((u8 *)command + 8))->integer),
                             ((ActionScalarArguments *)((u8 *)command + 8))->scalar);
}

u32 randomNext(void);
s32 terrainGetSurfaceAttributes(f32, f32, f32, s32);
s32 terrainGetMaterialSound(s32, s32);
void terrainCreateContactEffect(u8 *, s32);
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;
typedef struct {
    s32 amount;
} SoundAmplitude;
/// Plays ground-dependent actor sounds and contact effects requested by an action command.
/// \param object Actor whose ground contact supplies the effect location and surface.
/// \param state Unused action-handler context.
/// \param command Action command specifying the contact-effect intensity.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023534 (0x80023534).
void actionCommandPlayGroundContactEffects(u8 *object, u8 *state, ActorActionCueCommand *command, f32 *elapsed) {
    ActorActionCueCommand **source = &command;
    s32 ground;
    f64 random;
    s32 handle;
    s32 sound;
    s32 type;
    SoundAmplitude *args;
    ground = terrainGetSurfaceAttributes(*(f32 *)(object + 0x44), *(f32 *)(object + 0x48), *(f32 *)(object + 0x4C), 0);
    args = (SoundAmplitude *)(*source)->args;
    if (args->amount > 0) {
        if (object == gPikachuActor)
            type = 3;
        else
            type = 4;
        sound = terrainGetMaterialSound(type, ground);
        if (sound >= 0) {
            handle = actorPlayTrackedSound(object, sound);
            random = randomNext();
            audioSetSoundEffectPitch(handle, (random * 2.3283064365386963e-10 - 0.5) * args->amount * 3.0);
        }
    }
    if (args->amount >= 2)
        terrainCreateContactEffect(actorGetDefaultPart(object) + 0x44, ground);
}

/// ABI compatibility declaration: the ROM caller supplies an unused command pointer.
/// ABI compatibility declaration: the ROM caller supplies an unused command pointer.

/// Updates an actor part’s scale and configuration from an action command.
/// \param actor Actor whose appearance is updated.
/// \param state Unused action-handler context.
/// \param command Command specifying appearance slot 0 or 1, its uniform scale, and a valid model variant; the selected part must exist.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023674 (0x80023674).
void actionCommandConfigureActorPartScale(s32 actor, u8 *state, u8 *command, f32 *elapsed) {
    u8 *p;

    p = command + 8;
    actorAppearanceSetVariant((u8 *)actor, *(s32 *)(command + 8), *(s32 *)(command + 0x10));
    actorAppearanceSetScale((u8 *)actor, *(s32 *)p, *(f32 *)(p + 4));
}


/// Sets the actor’s rendered color from an action command.
/// \param actor Actor with an initialized tinted model part.
/// \param state Unused action-handler context.
/// \param command Command specifying color components from 0 through 255; the green component is supplied as a floating-point value.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800236C8 (0x800236C8).
void actionCommandSetActorTint(s32 actor, s32 state, u8 *command, s32 elapsed) {
    actorSetPartTint((u8 *)actor, command[0xB], *(f32 *)(command + 0xC), command[0x13]);
}

/// Per-mode vertical launch factors, negated and multiplied by command speed.
/// \note Original identifier: \c D_800A4938 (0x800A4938).
const f32 gActionLaunchVerticalSpeedFactors[8] = {-30.0f, -30.0f, -60.0f, -600.0f, 300.0f, -30.0f, -60.0f, -0.4f};
/// Per-mode launch heading offsets in degrees.
/// \note Original identifier: \c D_800A4958 (0x800A4958).
const f32 gActionLaunchHeadingOffsets[8] = {0.0f, 0.0f, 240.0f, 0.0f, 0.0f, 0.0f, 200.0f, 0.0f};
/// Per-mode flags passed to the held-item throw interface.
/// \note Original identifier: \c D_800A4978 (0x800A4978).
const s32 gActionLaunchThrowFlags[8] = {1, 1, 1, 1, 1, 1, 1, 0};

u32 randomBelow(s32);
/// Per-mode vertical launch factors, negated and multiplied by command speed.
/// \note Original identifier: \c D_800A4938 (0x800A4938).
extern const f32 gActionLaunchVerticalSpeedFactors[];
/// Per-mode launch heading offsets in degrees.
/// \note Original identifier: \c D_800A4958 (0x800A4958).
extern const f32 gActionLaunchHeadingOffsets[];
/// Per-mode flags passed to the held-item throw interface.
/// \note Original identifier: \c D_800A4978 (0x800A4978).
extern const s32 gActionLaunchThrowFlags[];
f32 vector3Normalize(u8 *);
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);

typedef struct Launch {
    s32 kind;
    f32 speed;
} Launch;

/// Launches the actor’s held item according to the action command, when the item is available.
/// \pre Player-directed mode 0 requires nonzero horizontal distance between the item and player.
/// \param actor Actor launching the held item.
/// \param state Actor state containing the held item and aiming information.
/// \param command Action command specifying a launch mode from zero through seven and a speed multiplier.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
///
/// \note Portability: matching this function requires the accepted assembler-input boundary insertion declared in tools/binasm_patches/action_script_commands.json.
/// \note Original decompilation identifier: \c func_80023788 (0x80023788).
void actionCommandLaunchHeldItem(u8 *actor, u8 *state, ActorActionCueCommand *command, f32 *elapsed) {
    ActorActionCueCommand **source = &command;
    u8 *obj;
    f32 speed;
    f32 ang;
    f32 vel[3];
    f32 pos[3];
    f32 dx;
    f32 dz;
    f32 len;
    u8 *p;
    Launch *l;
    f32 ang6;
    f32 ang7;
    f32 dir[3];

    obj = (u8 *)objectFindLiveByHandle(*(s32 *)(state + 0x190));
    if (obj == NULL) {
        return;
    }
    l = (Launch *)(*source)->args;
    speed = l->speed * 60.0f;
    actorGetConfiguredAttachmentPosition(actor, *(f32 *)(obj + 0x24), pos);
    switch (l->kind) {
    case 0:
        if (*(s32 *)(state + 0x19C) == 1 && (p = actorResolveStoredTarget(actor)) != NULL && *(u16 *)(p + 0x34) == 0x7F) {
            dx = gPlayerState.x - *(f32 *)(obj + 0x18);
            dz = gPlayerState.z - *(f32 *)(obj + 0x20);
            len = sqrtf(dx * dx + dz * dz);
            speed /= len;
            vel[0] = dx * speed;
            vel[1] = -gActionLaunchVerticalSpeedFactors[l->kind] * l->speed;
            vel[2] = dz * speed;
            break;
        }
    case 1:
    case 2:
    case 3:
    case 4:
        ang = *(f32 *)(state + 0x10) + gActionLaunchHeadingOffsets[l->kind];
        vel[0] = mathSinDegrees(ang) * speed;
        vel[1] = -gActionLaunchVerticalSpeedFactors[l->kind] * l->speed;
        vel[2] = mathCosDegrees(ang) * speed;
        break;
    case 6:
        speed *= randomNext() * 2.3283064365386963e-10 * 0.5 + 0.5;
        ang6 = (*(f32 *)(state + 0x10) + gActionLaunchHeadingOffsets[l->kind]) +
               (randomNext() * 2.3283064365386963e-10 - 0.5) * 90.0;
        vel[0] = mathSinDegrees(ang6) * speed;
        vel[1] = -gActionLaunchVerticalSpeedFactors[l->kind] * l->speed;
        vel[2] = mathCosDegrees(ang6) * speed;
        break;
    case 7:
        linkedEffectCreateChain(
            randomBelow(2) != 0 ? LINKED_EFFECT_REBOUND : LINKED_EFFECT_FALLING, pos[0], pos[1], pos[2], randomNext() * 2.3283064365386963e-10 * 0.1f + 0.03f,
            actorGetPartObjectHandle(gPikachuActor, 0x79), 1.0f, 0.8f, 2.0f, 0.1f, 0.1f, 0xC2, 1, randomBelow(2) + 1);
        vel[0] = (randomNext() * 2.3283064365386963e-10 - 0.5) * 8.0;
        vel[1] = 0.0f;
        vel[2] = (randomNext() * 2.3283064365386963e-10 - 0.5) * 8.0;
        fallingParticleCreate(randomBelow(7), randomBelow(3), pos, vel,
                      randomNext() * 2.3283064365386963e-10 * 0.4f + 0.8f);
        speed = randomNext() * 2.3283064365386963e-10 * 0.1f + 0.8f;
        ang7 = (*(f32 *)(state + 0x10) + gActionLaunchHeadingOffsets[l->kind]) +
               (randomNext() * 2.3283064365386963e-10 - 0.5) * 360.0;
        vel[0] = mathSinDegrees(ang7) * speed;
        vel[1] = -gActionLaunchVerticalSpeedFactors[l->kind] * l->speed;
        vel[2] = mathCosDegrees(ang7) * speed;
        break;
    case 5:
        dir[0] = *(f32 *)(state + 0x1EC) - *(f32 *)(obj + 0x44);
        dir[1] = *(f32 *)(state + 0x1F0) - *(f32 *)(obj + 0x48);
        dir[2] = *(f32 *)(state + 0x1F4) - *(f32 *)(obj + 0x4C);
        vector3Normalize((u8 *)dir);
        vel[0] = dir[0] * speed;
        vel[1] = dir[1] * speed;
        vel[2] = dir[2] * speed;
        break;
    default:
        break;
    }
    itemThrow(*(s32 *)(obj + 0x38), (f32 *)(obj + 0x18), pos, vel, gActionLaunchThrowFlags[l->kind]);
    *(u64 *)(state + 0x1A8) = runtimeGetElapsedMilliseconds();
}

void actorTryHoldItem(u8 *, s32, s32);

/// Selects a nearby matching item for the actor to hold when it has no live held item.
/// \param actor Actor that will hold the selected item.
/// \param state Actor state providing the current held item, desired item kind, and search distance.
/// \param command Action command specifying the item search filter.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023E30 (0x80023E30).
void actionCommandPickUpNearbyItem(s32 actor, u8 *state, u8 *command, s32 elapsed) {
    s32 pad[2]; /* unused, sizes the frame */
    f32 pos[3];
    u8 *r;

    if (objectFindLiveByHandle(*(s32 *)(state + 0x190)) == 0) {
        actorGetConfiguredAttachmentPosition((u8 *)actor, 0.0f, pos);
        r = itemFindNearest(pos, *(f32 *)(state + 0x1B4), 0.0f, 0.0f, 360.0f, *(s32 *)(state + 0x1B0),
                          *(s32 *)(state + 0x1B0), *(s32 *)(command + 8), 0);
        if (r != NULL) {
            actorTryHoldItem((u8 *)actor, *(s32 *)(r + 0x38), 0);
        }
    }
}

typedef struct SpawnChoice {
    s32 firstKind;
    f32 kindCount;
} SpawnChoice;
/// Creates an item of a randomly selected kind for the actor to hold when it has no live held item.
/// \param object Actor that will hold the created item.
/// \param state Actor state identifying its current held item.
/// \param command Command specifying a valid first item kind and a representable floating-point count that truncates to a positive integer; all selectable kinds must be valid.
/// \param unused Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023ECC (0x80023ECC).
void actionCommandCreateRandomHeldItem(u8 *object, u8 *state, ActorActionCueCommand *command, f32 *unused) {
    ActorActionCueCommand **source = &command;
    s32 count;
    struct {
        f32 x, y, z;
    } position;
    u8 *spawned;
    SpawnChoice *choice;
    if (objectFindLiveByHandle(*(s32 *)(state + 0x190)) == 0) {
        actorGetConfiguredAttachmentPosition((u8 *)object, 0.0f, &position.x);
        choice = (SpawnChoice *)(*source)->args;
        count = choice->kindCount;
        spawned =
            itemCreate(position.x, position.y, position.z, 0.0f, 0.0f, randomBelow(count) + choice->firstKind);
        if (spawned != NULL) {
            actorTryHoldItem(object, *(s32 *)(spawned + 0x38), 1);
        }
    }
}


/// Creates a vertical stack of effects at a world position.
/// \param position World-space base position as three consecutive coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023F6C (0x80023F6C).
void effectCreateVerticalStack(u8 *position) {
    wispEffectCreate(1, ((f32 *)position)[0], ((f32 *)position)[1], ((f32 *)position)[2]);
    wispEffectCreate(1, ((f32 *)position)[0], ((f32 *)position)[1] + 19.5f, ((f32 *)position)[2]);
    wispEffectCreate(1, ((f32 *)position)[0], ((f32 *)position)[1] + 39.5f, ((f32 *)position)[2]);
}


/// Gets the position to use when directing an actor’s effects toward its target.
/// \param actor Actor whose target position is requested.
/// \returns A borrowed address of the tracking target’s world position, or the actor’s stored target position when no tracking target is available; valid while its owner remains live.
/// \note Original decompilation identifier: \c func_80023FF4 (0x80023FF4).
s32 actorGetEffectTargetPosition(u8 *actor) {
    s32 actorStateAddress;
    u8 *trackingTarget;

    actorStateAddress = ((RuntimeObjectPrefix *)actor)->state.address;
    trackingTarget = actorGetTrackingTarget(actor);
    if (trackingTarget != 0) {
        return (s32)(trackingTarget + 0x44);
    }
    return actorStateAddress + 0x1EC;
}


/// Attempts to transform an item into its configured replacement.
/// \param id Handle of the item to transform.
/// \returns One if a different live item results, or zero if no live replacement is produced.
/// \note Original decompilation identifier: \c func_80024028 (0x80024028).
s32 itemTryTransformToReplacement(s32 id) {
    s32 savedReplacementHandle;
    s32 replacementHandle;

    replacementHandle = itemTransform(id, 1);
    savedReplacementHandle = replacementHandle;
    if ((objectFindLiveByHandle(replacementHandle) == 0) || (id == savedReplacementHandle)) {
        return 0;
    }
    return 1;
}

typedef struct {
    s32 op;
    f32 time;
    s32 count;
    f32 lifetime;
    s32 kind;
} BurstCommand;
/// Emits paired particle bursts toward the actor’s target and attempts to transform a targeted item, using the actor’s available burst allowance.
/// \param object Actor that emits the bursts.
/// \param state Actor state containing the remaining burst allowance and result flag.
/// \param command Command specifying the requested burst count, positive lifetime in seconds, and valid ribbon-width style.
/// \param unused Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024074 (0x80024074).
void actionCommandEmitPairedTargetedBursts(u8 *object, u8 *state, BurstCommand *command, s32 unused) {
    s32 count;
    s32 particles;
    f32 left[3];
    f32 right[3];
    f32 *target;
    u8 *other;
    u32 random;
    s32 yaw;
    struct {
        s32 count;
        f32 lifetime;
        s32 kind;
    } *args;
    args = (void *)&command->count;
    count = args->count < ((ActorScriptState *)state)->targetedBurstAllowance ? args->count : ((ActorScriptState *)state)->targetedBurstAllowance;
    if (count > 0) {
        ((ActorScriptState *)state)->targetedBurstAllowance -= count;
        actorGetPartWorldPosition(object, 0x74, left);
        actorGetPartWorldPosition(object, 0x75, right);
        target = (f32 *)actorGetEffectTargetPosition(object);
        for (; count > 0; count--) {
            particles = randomBelow(10);
            random = randomNext();
            yaw = (s32)randomBelow(90);
            ribbonEffectCreate(particles + 10, args->lifetime, 0.5f, random * 2.3283064365386963e-10 * 0.3f, args->kind, left[0],
                          left[1], left[2], *(f32 *)(object + 0x10) - 120.0f + 30.0f - yaw,
                          *(f32 *)(object + 0x14) + 45.0f - (s32)randomBelow(90), target);
            particles = randomBelow(10);
            random = randomNext();
            yaw = (s32)randomBelow(90);
            ribbonEffectCreate(particles + 10, args->lifetime, 0.5f, random * 2.3283064365386963e-10 * 0.3f, args->kind,
                          right[0], right[1], right[2], *(f32 *)(object + 0x10) - 60.0f + 60.0f - yaw,
                          *(f32 *)(object + 0x14) + 45.0f - (s32)randomBelow(90), target);
        }
        other = actorGetTrackingTarget(object);
        if (other != NULL && *(u16 *)(other + 0x34) == 0x86 && itemTryTransformToReplacement(*(s32 *)(other + 0x38))) {
            ((ActorScriptState *)state)->costedActionResult = 1;
        }
    }
}

typedef struct {
    s32 target;
    f32 lifetime;
    s32 count;
} EmitArgs;
/// Emits particles at an actor part or tracking target selected by the action command.
/// \param object Actor supplying the effect target.
/// \param unused Unused action-handler context.
/// \param command Command selecting the target, positive particle lifetime in seconds, and positive particle count.
/// \param unused2 Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002436C (0x8002436C).
void actionCommandEmitParticlesAtTarget(u8 *object, u8 *unused, ActorActionCueCommand *command, f32 *unused2) {
    EmitArgs *args = (EmitArgs *)command->args;
    u8 *target;
    switch (args->target) {
    case 1:
        target = actorFindPart(object, 0x79);
        break;
    case 2:
        target = actorGetTrackingTarget(object);
        if (target == NULL)
            return;
        break;
    default:
        target = actorGetDefaultPart(object);
        break;
    }
    if (target == NULL)
        target = object;
    particleClusterCreate(args->count, args->lifetime, 5.0f, randomNext() * 2.3283064365386963e-10 * 0.4f + 0.1f, target);
}

typedef struct {
    f32 v[3];
} GroundVector;
f32 terrainGetWorldHeight(f32, f32, f32, void *);
/// Create a scattered particle effect at the actor or its tracking target.
/// \param actor Actor supplying the effect position or tracking target.
/// \param state Unused action-handler context.
/// \param command Command selecting the target and positive effect lifetime in seconds.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024474 (0x80024474).
void actionCommandCreateScatteredParticleEffect(u8 *actor, s32 state, u8 *command, s32 elapsed) {
    u8 *resolved;
    u8 *object;
    GroundVector ground;
    if (*(s32 *)(command + 8) == 2) {
        resolved = (u8 *)actorGetTrackingTarget(actor);
        object = resolved;
        if (object == NULL) {
            return;
        }
    } else {
        object = actor;
    }
    ground.v[0] = *(f32 *)(object + 0x44);
    ground.v[1] = terrainGetWorldHeight(*(f32 *)(object + 0x44), *(f32 *)(object + 0x48), *(f32 *)(object + 0x4C), NULL);
    ground.v[2] = *(f32 *)(object + 0x4C);
    effectEmitterCreateForStyle(EFFECT_EMIT_SCATTERED_PARTICLES, (f32 *)(object + 0x44), *(f32 *)(command + 0xC), 0, *(s32 *)(object + 0x38), -6.0f);
}

/// Script operands for a converging effect around an actor or its linked part.
typedef struct LinkedPartEffectArguments {
    s32 style;    ///< Converging-effect style-table index.
    f32 lifetime; ///< Positive duration in seconds.
    s32 count;    ///< Positive number of visual elements.
} LinkedPartEffectArguments;

typedef char LinkedPartEffectArgumentsSizeCheck[(sizeof(LinkedPartEffectArguments) == 12) ? 1 : -1];
typedef char LinkedPartEffectLifetimeOffsetCheck[(((u32) & ((LinkedPartEffectArguments *)0)->lifetime) == 4) ? 1 : -1];

/// Creates a linked effect around an actor part, using the actor itself when that part is unavailable.
/// \param actor Actor supplying the effect target.
/// \param state Unused action-handler context.
/// \param command Command specifying a valid effect style, positive lifetime in seconds, and positive element count.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024520 (0x80024520).
void actionCommandCreateLinkedPartEffect(u8 *actor, u8 *state, u8 *command, f32 *elapsed) {
    u8 *linkedPart;
    LinkedPartEffectArguments *commandArguments;
    LinkedPartEffectArguments *effectArguments;
    u8 *effectOwner;

    linkedPart = actorFindPart(actor, 0x79);
    commandArguments = (LinkedPartEffectArguments *)(command + 8);
    effectOwner = linkedPart;
    if (linkedPart == 0) {
        effectOwner = actor;
    }
    effectArguments = commandArguments;
    convergingParticleEffectCreate(*(s32 *)(effectOwner + 0x38), effectArguments->lifetime, effectArguments->count,
                                   effectArguments->style);
}

s32 itemTryTransformToReplacement(s32); /* extern */

/// Creates an effect at the selected target or at the actor; the actor variant also attempts to transform its held item.
/// \param actor Actor supplying the effect position or target.
/// \param state Actor state containing the held item and transformation result flag.
/// \param command Action command selecting the actor or its effect target.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024578 (0x80024578).
void actionCommandCreateItemTransformationEffect(u8 *actor, u8 *state, u8 *command, f32 *elapsed) {
    u8 *effectPosition;

    if (((ActionScalarArguments *)((u8 *)command + 8))->integer == 2) {
        effectPosition = actorGetEffectTargetPosition(actor);
        wispEffectCreate(0, ((f32 *)effectPosition)[0], ((f32 *)effectPosition)[1], ((f32 *)effectPosition)[2]);
        return;
    }
    wispEffectCreate(0, ((RuntimeObjectPrefix *)actor)->worldPosition[0],
                     ((RuntimeObjectPrefix *)actor)->worldPosition[1] + 2.0f,
                     ((RuntimeObjectPrefix *)actor)->worldPosition[2]);
    if (itemTryTransformToReplacement(((ActorScriptState *)state)->heldItemHandle) != 0) {
        ((ActorScriptState *)state)->costedActionResult = 1;
    }
}

/// Shared count and lifetime operands for target-following visual effects.
typedef struct FollowingEffectArguments {
    s32 count;    ///< Number of visual elements.
    f32 lifetime; ///< Effect lifetime in seconds.
} FollowingEffectArguments;

typedef char FollowingEffectArgumentsSizeCheck[(sizeof(FollowingEffectArguments) == 8) ? 1 : -1];
typedef char
    FollowingEffectLifetimeOffsetCheck[(((u32) & ((FollowingEffectArguments *)0)->lifetime) == 4) ? 1 : -1];

/// Create a flickering visual effect that follows the actor's default part.
/// \param actor Actor whose default part receives the effect.
/// \param state Unused action-handler context.
/// \param command Command specifying a positive lifetime in seconds and a positive element count.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024618 (0x80024618).
void actionCommandCreateFlickeringPartEffect(s32 actor, u8 *state, u8 *command, f32 *elapsed) {
    FollowingEffectArguments *arguments = (FollowingEffectArguments *)(command + 8);

    flickeringEffectCreate(((RuntimeObjectPrefix *)actorGetDefaultPart((u8 *)actor))->handle, arguments->lifetime,
                           arguments->count);
}

/// Creates a vertical stack of effects at the actor or its effect target.
/// \param actor Actor supplying the effect position or target.
/// \param state Unused action-handler context.
/// \param command Action command selecting the actor or its effect target.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024658 (0x80024658).
void actionCommandCreateStackedEffects(u8 *actor, u8 *state, u8 *command, f32 *elapsed) {
    if (((ActionScalarArguments *)((u8 *)command + 8))->integer == 2) {
        effectCreateVerticalStack(actorGetEffectTargetPosition(actor));
        return;
    }
    effectCreateVerticalStack(actor + 0x44);
}


/// Create an animated radial effect that follows the actor.
/// \param actor Actor receiving the effect.
/// \param state Unused action-handler context.
/// \param command Command specifying a positive lifetime in seconds and a positive element count.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800246B0 (0x800246B0).
void actionCommandCreateAnimatedRadialEffect(u8 *actor, u8 *state, u8 *command, f32 *elapsed) {
    FollowingEffectArguments *arguments = (FollowingEffectArguments *)(command + 8);

    animatedRadialEffectCreate(((RuntimeObjectPrefix *)actor)->handle, arguments->lifetime, arguments->count);
}

typedef struct {
    s32 posType;
    f32 lifetime;
    u32 kind;
} EmitArgs246;

/// Emits an effect at the actor attachments or target selected by an action command.
/// \param object Actor that owns the emitted effect.
/// \param state Actor state providing a fallback target position.
/// \param command Command selecting attachment 0 or 1 or target 2, positive lifetime in seconds, and a valid emitter style.
/// \param unused Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800246E8 (0x800246E8).
void actionCommandEmitEffectAtSelectedAttachment(u8 *object, u8 *state, ActorActionCueCommand *command, f32 *unused) {
    EmitArgs246 *args;
    u8 *other;
    s32 target;

    args = (EmitArgs246 *)command->args;
    switch (args->posType) {
    case 0:
        target = actorGetPartObjectHandle(object, 0x76);
        effectEmitterCreateForStyle(args->kind, (f32 *)(object + 0x44), args->lifetime, target, *(s32 *)(object + 0x38),
                      *(f32 *)(object + 0x24));
        break;
    case 1:
        target = actorGetPartObjectHandle(object, 0x74);
        effectEmitterCreateForStyle(args->kind, (f32 *)(object + 0x44), args->lifetime, target, *(s32 *)(object + 0x38),
                      *(f32 *)(object + 0x24));
        target = actorGetPartObjectHandle(object, 0x75);
        effectEmitterCreateForStyle(args->kind, (f32 *)(object + 0x44), args->lifetime, target, *(s32 *)(object + 0x38),
                      *(f32 *)(object + 0x24));
        break;
    case 2:
        other = actorGetTrackingTarget(object);
        if (other != NULL) {
            effectEmitterCreateForStyle(args->kind, (f32 *)(other + 0x44), args->lifetime, *(s32 *)(other + 0x38),
                          *(s32 *)(object + 0x38), *(f32 *)(object + 0x24));
        } else {
            effectEmitterCreateForStyle(args->kind, (f32 *)(state + 0x1EC), args->lifetime, 0, *(s32 *)(object + 0x38),
                          *(f32 *)(object + 0x24));
        }
        break;
    default:
        TRACE("ERROR: Illegal posType(%d)\n", args->posType);
        break;
    }
}

/// Creates an effect at the actor or its available tracking target.
/// \param actor Actor supplying the effect owner or position.
/// \param state Unused action-handler context.
/// \param command Action command specifying the position selector and effect kind.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024864 (0x80024864).
void actionCommandCreateEffectAtActorOrTarget(u8 *actor, u8 *state, u8 *command, f32 *elapsed) {
    s32 positionKind;
    u8 *trackingTarget;

    positionKind = ((ActionScalarArguments *)((u8 *)command + 8))->integer;
    switch (positionKind) { /* irregular */
    case 0:
        wispEffectCreate((s32)((ActionScalarArguments *)((u8 *)command + 8))->scalar,
                         ((RuntimeObjectPrefix *)actor)->worldPosition[0],
                         ((RuntimeObjectPrefix *)actor)->worldPosition[1],
                         ((RuntimeObjectPrefix *)actor)->worldPosition[2]);
        return;
    case 2:
        trackingTarget = actorGetTrackingTarget(actor);
        if (trackingTarget != 0) {
            wispEffectCreate((s32)((ActionScalarArguments *)((u8 *)command + 8))->scalar,
                             ((RuntimeObjectPrefix *)trackingTarget)->worldPosition[0],
                             ((RuntimeObjectPrefix *)trackingTarget)->worldPosition[1],
                             ((RuntimeObjectPrefix *)trackingTarget)->worldPosition[2]);
        }
        return;
    default:
        TRACE("ERROR: Illegal posType(%d)\n", positionKind);
        return;
    }
}


/// Creates an animated effect attached to the actor, replacing its previous attached effect.
/// \param actor Actor supplying the effect owner or position.
/// \param state Unused action-handler context.
/// \param command Command specifying valid effect and animation-track indices; the track is supplied as a representable floating-point integer.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002490C (0x8002490C).
void actionCommandCreateAttachedEffect(u8 *actor, u8 *state, u8 *command, f32 *elapsed) {
    attachedEffectCreate(actor, ((ActionScalarArguments *)((u8 *)command + 8))->integer,
                         (s32)((ActionScalarArguments *)((u8 *)command + 8))->scalar);
}

/// Separate symbol view of the primary player record's position Y.
/// \note Original identifier: \c D_800EF6A4 (0x800EF6A4).
extern f32 gPlayerPositionY;
/// Separate symbol view of the primary player record's position Z.
/// \note Original identifier: \c D_800EF6A8 (0x800EF6A8).
extern f32 gPlayerPositionZ;
/// Separate symbol view of the primary player record's mode and camera-transition state.
/// \note Original identifier: \c D_800EF728 (0x800EF728).
extern s32 gPlayerModeState;
f32 vector3Length(f32 *);
void playerBeginStun(f32 *, f32);
typedef struct ArgsSpawn {
    s32 base;
    f32 range;
} ArgsSpawn;

/// Stun the player when within the commanded range or targeted by the actor.
/// \param actor Actor whose effect-target position supplies the range check origin.
/// \param state Actor state identifying its active player target.
/// \param command Command specifying a nonnegative integer stun duration in seconds and the range from the actor’s effect-target position.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024948 (0x80024948).
void actionCommandStunPlayerInRange(u8 *actor, u8 *state, ActorActionCueCommand *command, s32 elapsed) {
    u8 *o;
    ArgsSpawn *a;
    f32 dist;
    f32 d[3];

    o = (u8 *)actorGetEffectTargetPosition(actor);
    d[0] = *(f32 *)(o + 0) - gPlayerState.x;
    d[1] = *(f32 *)(o + 4) - gPlayerPositionY;
    d[2] = *(f32 *)(o + 8) - gPlayerPositionZ;
    a = (ArgsSpawn *)command->args;
    dist = vector3Length(d);
    TRACE("vec:%f,%f,%f len:%f  stunRange:%f\n", d[0], d[1], d[2], dist, a->range);
    if (dist <= a->range || (*(s32 *)(state + 0x19C) != 0 && gPlayerModeState == *(s32 *)(state + 0x198))) {
        playerBeginStun(&gPlayerState.x, a->base);
    }
}


/* Script arguments: integer endpoints and a floating-point duration. */
typedef struct {
    s32 start;
    f32 duration;
    s32 end;
} LaunchArgs24A18;

/// Starts direction motion using the angular rates and duration requested by an action command.
/// \param actor Unused actor argument.
/// \param state Actor state containing the direction animation.
/// \param command Command specifying integer pitch and yaw rates in degrees per second and a nonnegative duration in seconds.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024A18 (0x80024A18).
void actionCommandStartDirectionMotion(s32 actor, u8 *state, u8 *command, s32 elapsed) {
    LaunchArgs24A18 *p = (LaunchArgs24A18 *)(command + 8);
    DirectionAngularRates pts[2];
    s32 i;

    for (i = 0; i < 2; i++) {
        pts[i].pitch = p->start;
        pts[i].yaw = p->end;
        directionAnimationSetNextPoseAngles((u8 *)&((ActorScriptState *)state)->directionAnimation, -40.0f, 0.0f, i);
    }
    directionAnimationStartMotion((u8 *)&((ActorScriptState *)state)->directionAnimation, p->duration, pts);
}

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
typedef struct {
    s32 id;
    f32 val;
} Pair24AE0;

/// Updates the held-item configuration and optionally scales the actor’s live held item.
/// \param actor Unused actor argument.
/// \param state Actor state containing the held item and its configuration.
/// \param command Action command specifying a configuration selector and a positive scale, or a nonpositive scale to leave size unchanged.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024AE0 (0x80024AE0).
void actionCommandConfigureHeldItem(s32 actor, u8 *state, u8 *command, f32 *elapsed) {
    Pair24AE0 *p = (Pair24AE0 *)(command + 8);
    f32 *v;
    f32 f;

    if (p->id >= 0) {
        if (p->id < 6) {
            *(s32 *)(state + 0x194) = p->id;
        }
    }
    if (p->val > 0.0f) {
        v = (f32 *)objectFindLiveByHandle(*(s32 *)(state + 0x190));
        if (v != NULL) {
            f = *(f32 *)(gCurrentAreaConfiguration + 0xC) * p->val;
            v[0] = f;
            v[1] = f;
            v[2] = f;
        }
    }
}

void objectDebugReportNoOp(u8 *);                /* extern */

/// Requests a transformation of the actor’s stored target item and optionally plays a sound when no replacement is produced.
/// \param actor Actor whose stored target item is requested to transform.
/// \param state Unused action-handler context.
/// \param command Action command specifying the transformation mode and an optional sound ID.
/// \param elapsed Unused action-handler timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024B70 (0x80024B70).
void actionCommandTransformTargetItem(s32 actor, u8 *state, u8 *command, f32 *elapsed) {
    s32 replacementHandle;
    u8 *targetItem;
    u8 *transformArguments;

    targetItem = actorResolveStoredTarget((u8 *)actor);
    if ((targetItem == NULL) ||
        (transformArguments = command + 8, ((u16)((RuntimeObjectPrefix *)targetItem)->kind != 0x86))) {
        TRACE("Warning: Tried to change non_item_goal_obj\n");
        if (targetItem != NULL) {
            objectDebugReportNoOp(targetItem);
        }
    } else {
        replacementHandle = itemTransform(((RuntimeObjectPrefix *)targetItem)->handle,
                                          ((ActionScalarArguments *)transformArguments)->integer);
        if ((((ActionScalarArguments *)transformArguments)->scalar >= 0.0f) &&
            ((replacementHandle == 0) || (replacementHandle == ((RuntimeObjectPrefix *)targetItem)->handle))) {
            objectPlaySpatialSound(targetItem, (s32)((ActionScalarArguments *)transformArguments)->scalar, 20.0f,
                                   100.0f);
        }
    }
}

typedef struct ArgsMsg {
    s32 a;
    f32 c;
    s32 d;
} ArgsMsg;

/// Send an action notification to the actor's eligible stored target.
/// \param actor Actor sending the notification.
/// \param unusedState Unused action-state argument.
/// \param command Command containing the notification values.
/// \param unusedTiming Unused timing argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024C30 (0x80024C30).
void actionCommandNotifyStoredTarget(u8 *actor, s32 unusedState, ActorActionCueCommand *command, s32 unusedTiming) {
    ArgsMsg *args;
    ScriptMessage m;
    u8 *o = actorResolveStoredTarget(actor);

    args = (ArgsMsg *)command->args;
    if (o != NULL && *(u16 *)(o + 0x34) > 0 && *(u16 *)(o + 0x34) < 0x26) {
        m.arg1 = 0;
        m.type = 0x10;
        m.arg0 = args->a;
        m.arg2 = args->c;
        m.arg3 = args->d;
        m.arg4 = *(s32 *)(actor + 0x38);
        actorPostScriptMessage((s32)o, m, 0);
    }
}

/// Advance the action script clock by the commanded amount.
/// \param unusedActor Unused actor argument.
/// \param unusedState Unused action-state argument.
/// \param command Command specifying the signed clock change in seconds.
/// \param clock Writable action script clock in seconds to adjust.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024D00 (0x80024D00).
void actionCommandAdvanceClock(s32 unusedActor, u8 *unusedState, u8 *command, f32 *clock) {
    *clock += ((ActionScalarArguments *)((u8 *)command + 8))->scalar;
}

/// Advance the action script clock by a randomized amount.
/// \param unusedActor Unused actor argument.
/// \param unusedState Unused action-state argument.
/// \param command Command specifying the number of random clock changes and their signed amplitude in seconds; a nonpositive count leaves the clock unchanged.
/// \param clock Writable action script clock in seconds to adjust.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024D1C (0x80024D1C).
void actionCommandAdvanceClockRandomly(s32 unusedActor, s32 unusedState, u8 *command, f32 *clock) {
    s32 n = *(s32 *)(command + 8);
    f32 amp = *(f32 *)(command + 0xC);
    s32 i;

    for (i = 0; i < n; i++) {
        *clock += randomNext() * 2.3283064365386963e-10 * amp;
    }
}

typedef struct CmdJump {
    ActorActionCueCommand *target;
    f32 reset;
} CmdJump;
typedef void (*CmdFn)(u8 *, u8 *, ActorActionCueCommand *, f32 *);
/// Timed action-command handlers indexed by opcode minus nineteen.
/// \note Original identifier: \c D_80089490 (0x80089490).
extern CmdFn gTimedActionCommandHandlers[];
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance an actor's timed action script through the commands ready to run.
/// \pre Commands and jump targets must remain valid, use supported handlers, and eventually finish or wait for a future time.
/// \param actor Actor performing the actions.
/// \param nextCommand Current command, updated to the next pending command or null when finished.
/// \param clock Writable elapsed script time in seconds, updated as the script advances.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80024DD0 (0x80024DD0).
void actorUpdateActionScript(u8 *actor, ActorActionCueCommand **nextCommand, f32 *clock) {
    u8 *st = *(u8 **)(actor + 0x54);
    ActorActionCueCommand *c;
    CmdJump *j;
    s32 op;
    f32 d;

    *clock += gRuntimeFrameSeconds;
    c = *nextCommand;
    if (c == NULL) {
        return;
    }
    while (c->time <= *clock) {
        op = c->opcode;
        if (op < 19) {
            c++;
            continue;
        }
        switch (op) {
        case 46:
            j = (CmdJump *)c->args;
            c = j->target;
            if (j->reset != 0.0f) {
                *clock = 0.0f;
            }
            break;
        case 47:
            d = *(f32 *)(st + 0x134) - *(f32 *)(st + 0x138);
            if (d < 0.001f && d > -0.001f) {
                *clock = 0.0f;
                c++;
            } else {
                goto out;
            }
            break;
        case 48:
        case 49:
            *nextCommand = NULL;
            return;
        default:
            gTimedActionCommandHandlers[op - 19](actor, st, c, clock);
            c++;
            break;
        }
    }
out:
    *nextCommand = c;
}


/// Select an action script for the actor's current action state.
/// \param actor Actor whose action state selects the script.
/// \param list Borrowed script pointer for action state 18, otherwise a nullable tagged collection terminated by tag 49; it must remain valid during playback.
/// \param unusedFlag Legacy flag that does not affect the selection.
/// \returns A borrowed selected script, the supplied list when no state selection is needed, or null if no script matches.
/// \note Original decompilation identifier: \c func_80024F9C (0x80024F9C).
ActorActionCueCommand *actorSelectActionScript(u8 *actor, ActorActionCueCommand *list, s32 unusedFlag) {
    u8 *o = *(u8 **)(actor + 0x54);
    ActorActionCueCommand *p;
    s32 st;

    if (((ActorScriptState *)o)->selectedActionKind == 0x12) {
        if (unusedFlag != 0) {
            return list;
        }
        return list;
    }
    p = list;
    if (p == NULL) {
        return NULL;
    }
    st = ((ActorScriptState *)o)->selectedActionKind;
    for (; p->opcode != 0x31; p++) {
        if (p->opcode == 0x12 || p->opcode == st) {
            return p + 1;
        }
    }
    return NULL;
}
