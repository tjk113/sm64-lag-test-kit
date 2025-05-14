#include <engine/geo_layout.h>
#include <game/area.h>
#include <game/level_update.h>
#include <level_commands.h>
#include <levels/scripts.h>
#include <segment_symbols.h>
#include <types.h>

#include "level_load.h"


extern u8 sTransitionColorFadeCount[4];

// ptrs to static vars
struct MainPoolState **const gMainPoolState = 0x8032DD70;
LevelScript **const thread5GameLoopVar_addr = 0x80206DEC;
LevelScript **const sCurrentCmd = 0x8038BE28;
void (**const LevelScriptJumpTable)(void) = 0x8038B8B8;
void (*const level_cmd_load_and_execute)(void) = 0x8037E2C4;
uintptr_t *const sStack = 0x8038BDA0;
uintptr_t **const sStackTop = 0x8038B8B0;
uintptr_t **const sStackBase = 0x8038B8B4;
s32 *const sRegister = 0x8038BE24;
u16 *const sDelayFrames = 0x8038B8A4;
u16 *const sDelayFrames2 = 0x8038B8A8;


LevelScript levelMainScriptsEntry[] = {
    EXECUTE(0x15, _scriptsSegmentRomStart, _scriptsSegmentRomEnd, level_main_scripts_entry)
};

void load_level(struct LevelLoadParams *p) {
    u8 i;

    // init_level
    clear_areas();

    // restore pool to initial state for level_script_entry
    while (*gMainPoolState != NULL) {
        main_pool_pop_state();
    }

    // reset level script stack
    *sStackTop = sStack;
    *sStackBase = NULL;

    *sDelayFrames = 0;
    *sDelayFrames2 = 0;

    // load and execute level_main_scripts_entry
    *sCurrentCmd = levelMainScriptsEntry;
    level_cmd_load_and_execute();
    *sStack = 0x80064F60; // restore reference to level_script_entry

    // set level
    *sRegister = p->warpDest.levelNum;
    gCurrActNum = p->actNum;

    // advance to level_main_menu_entry_2[1]
    for (i = 0; i < 58; i++) {
        LevelScriptJumpTable[((struct LevelCommand*)(*sCurrentCmd))->type]();
    }
    *sRegister = 0; // overwrite lvl_set_current_level return value to 0 to skip star select

    // set warp destination
    sWarpDest = p->warpDest;

    // fix transition
    set_warp_transition_rgb(p->warpTransRed, p->warpTransGreen, p->warpTransBlue);
    sTransitionColorFadeCount[0] = 0;
    sTransitionColorFadeCount[1] = 0;
    sTransitionColorFadeCount[2] = 0;
    sTransitionColorFadeCount[3] = 0;

    // prevent crashes
    gHudDisplay.flags = 0;
    gMarioStates->action = 1;

    *thread5GameLoopVar_addr = *sCurrentCmd;
}