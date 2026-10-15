#include "runtime_mode.h"
#include "player_profile.h"
#include "player_state.h"
#include "profile_counters.h"
#include "story_milestones.h"
#include "progress_storage.h"
#include "common.h"
#include "string_bytes.h"
#include "player_types.h"
#include "script_vm_globals.h"

void _bzero(void *, s32); /* extern */
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;
/// Reset the player profile and associated session state to their defaults.
/// The default player name is "You".
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800214E0 (0x800214E0).
void playerResetProfile(void) {
    _bzero(&gPlayerState, 0x100);
    _bzero(&gScriptVmGlobalValues, sizeof(gScriptVmGlobalValues));
    _bzero(&gProfilePackedCounters, sizeof(gProfilePackedCounters));
    progressInitializeSaveStorage();
    stringCopy(gPlayerProfileName, (const u8 *)"You");
    ((&gPlayerState))->points = 0x12C;
    ((&gPlayerState))->unknown0F4 = 6;
    ((&gPlayerState))->unknown0F5 = 8;
    ((&gPlayerState))->friendshipLevel = 0;
    ((&gPlayerState))->deliveredVoiceMessageCount = 0;
    ((&gPlayerState))->friendshipMilestone = -1;
    ((&gPlayerState))->chaseStage = 0;
    if (gRuntimeGameMode == GAME_MODE_TITLE) {
        progressSetChapterAndUnlockMilestones(0);
    }
}
