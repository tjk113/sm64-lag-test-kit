#include <engine/geo_layout.h>
#include <game/area.h>
#include <game/level_update.h>
#include <level_commands.h>
#include <levels/scripts.h>
#include <segment_symbols.h>
#include <types.h>


struct LevelCommand {
    /*00*/ u8 type;
    /*01*/ u8 size;
    /*02*/ // variable sized argument data
};

extern u8 sTransitionColorFadeCount[4];
extern u8 gWarpTransRed;
extern u8 gWarpTransGreen;
extern u8 gWarpTransBlue;

// ptrs to static vars
struct MainPoolState **const gMainPoolState = 0x8032DD70;
LevelScript **const thread5GameLoopVar_addr = 0x80206DEC;
LevelScript **const sCurrentCmd = 0x8038BE28;
void (**const LevelScriptJumpTable)(void) = 0x8038B8B8;
void (*const level_cmd_load_and_execute)(void) = 0x8037E2C4;
u32 *const sStack = 0x8038BDA0;
u32 **const sStackTop = 0x8038B8B0;
u32 **const sStackBase = 0x8038B8B4;
s32 *const sRegister = 0x8038BE24;
u16 *const sDelayFrames = 0x8038B8A4;
u16 *const sDelayFrames2 = 0x8038B8A8;


LevelScript levelMainScriptsEntry[] = {
    EXECUTE(0x15, _scriptsSegmentRomStart, _scriptsSegmentRomEnd, level_main_scripts_entry)
};

void load_level() {
    u8 i;

    // init_level
    init_graph_node_start(NULL, (struct GraphNodeStart *) &gObjParentGraphNode);
    clear_objects();
    clear_area_graph_nodes();
    clear_areas();

    // restore pool to initial state for level_script_entry
    while (*gMainPoolState != NULL) {
        main_pool_pop_state();
    }

    // reset level script stack
    *sStackTop = sStack;
    *sStackBase = NULL;

    // load and execute level_main_scripts_entry
    *sCurrentCmd = levelMainScriptsEntry;
    level_cmd_load_and_execute();
    *sStack = 0x80064F60; // restore reference to level_script_entry

    gCurrActNum = 1;
    *sRegister = 9; // bob-omhb battlefield
    *sDelayFrames = 0;
    *sDelayFrames2 = 0;

    // advance to level_main_menu_entry_2[1]
    for (i = 0; i < 58; i++) {
        LevelScriptJumpTable[((struct LevelCommand*)(*sCurrentCmd))->type]();
    }
    *sRegister = 0; // overwrite lvl_set_current_level return value to 0 to skip star select

    // fix transition
    gWarpTransRed = 255;
    gWarpTransGreen = 255;
    gWarpTransBlue = 255;
    sTransitionColorFadeCount[0] = 0;
    sTransitionColorFadeCount[1] = 0;
    sTransitionColorFadeCount[2] = 0;
    sTransitionColorFadeCount[3] = 0;

    // warp dest
    sWarpDest.type = 1;
    sWarpDest.levelNum = 9;
    sWarpDest.areaIdx = 1;
    sWarpDest.nodeId = 10;

    // prevent crashes
    gHudDisplay.flags = 0;
    gMarioStates->action = 1;

    *thread5GameLoopVar_addr = *sCurrentCmd;
}