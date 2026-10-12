#include "profile_counters.h"
#include "story_milestones.h"
#include "progress_storage.h"
#include "common.h"
#include "string_bytes.h"
#include "player_types.h"

void _bzero(void *, s32);                                     /* extern */
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;
/// Player profile name string borrowed by actor scripts.
/// \note Original identifier: \c D_800EF780 (0x800EF780).
extern u8 gPlayerProfileName[];
/// Global script-variable storage containing 500 packed five-byte values.
/// \note Original identifier: \c D_800EF7A0 (0x800EF7A0).
extern u8 gScriptVmGlobalValues[];

/// Reset the player profile and associated session state to their defaults.
/// The default player name is "You".
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800214E0 (0x800214E0).
void playerResetProfile(void) {
    _bzero(&gPlayerState, 0x100);
    _bzero(&gScriptVmGlobalValues, 0x9C4);
    _bzero(&gProfilePackedCounters, sizeof(gProfilePackedCounters));
    progressInitializeSaveStorage();
    stringCopy(gPlayerProfileName, (const u8 *)"You");
    ((PlayerControlPrefix *)&gPlayerState)->points = 0x12C;
    ((PlayerControlPrefix *)&gPlayerState)->unknown0F4 = 6;
    ((PlayerControlPrefix *)&gPlayerState)->unknown0F5 = 8;
    ((PlayerControlPrefix *)&gPlayerState)->friendshipLevel = 0;
    ((PlayerControlPrefix *)&gPlayerState)->unknown0FC = 0;
    ((PlayerControlPrefix *)&gPlayerState)->friendshipMilestone = -1;
    ((PlayerControlPrefix *)&gPlayerState)->unknown0F7 = 0;
    if (gRuntimeGameMode == 0x12) {
        progressSetChapterAndUnlockMilestones(0);
    }
}
