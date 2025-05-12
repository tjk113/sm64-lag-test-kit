#include <string.h>

#include <game/area.h>
#include <game/game_init.h>
#include <game/level_update.h>
#include <game/object_list_processor.h>

#include "custom.h"
#include "lag.h"
#include "playback.h"
#include <levels/intro/header.h>
#include <engine/level_script.h>
#include <level_commands.h>
#include <segment_symbols.h>
#include <levels/scripts.h>


extern u8 data[];
u32 marker = 0x5A83B1C0;
u8 *data_ptr = data;
u32 recordingCount = 0;
u8 data[0x200000] = { 0 };

struct RecordingHeader curRec;
u16 buttonsDownLastFrame = 0;
u16 recordingIndex = 0;
u16 frame = 0;
u8 camControl = 0;

extern s32 init_level(void);


void load_state() {
    u8 *curData = curRec.state_data;
    u16 i = 0;
    while (i < curRec.mem_blocks_length) {
        struct MemBlock memBlock = curRec.state_mem_blocks[i];
        memcpy(memBlock.addr, curData, memBlock.size);
        curData += memBlock.size;
        i++;
    }
}

void write_inputs() {
    struct RecordingFrame curInputs = curRec.inputs[frame];

    u16 button = curInputs.button;
    if (camControl) {
        button = (button & 0xFFE0) | (gControllerPads[0].button & 0x001F);
    }

    gControllerPads[0].button = button;
    gControllerPads[0].stick_x = curInputs.stick_x;     
    gControllerPads[0].stick_y = curInputs.stick_y;

    if (gCurrentArea != NULL) {
        struct Camera *areaCam = gCurrentArea->camera;
        if (areaCam != NULL) {
            areaCam->yaw = curInputs.cam_yaw;
        }
    }
}

void restart_playback() {
    frame = 0;
    lagCounter = 0;
    load_state();
    write_inputs();
}

void advance_playback() {
    if (++frame >= curRec.length) {
        restart_playback();
    } else {
        write_inputs();
    }
}

u8 do_control() {
    u8 restartPlayback = 0;
    u16 buttonsPressed = gControllerPads[0].button & ~buttonsDownLastFrame;
    buttonsDownLastFrame = gControllerPads[0].button;

    if (buttonsPressed & CONT_UP) {
        camControl ^= 1;
    }
    if (buttonsPressed & CONT_L) {
        recordingIndex++;
        if (recordingIndex == recordingCount) {
            recordingIndex = 0;
        }
        curRec = ((struct RecordingHeader*)data)[recordingIndex];
        restartPlayback = 1;
    }
    /*if (buttonsPressed & CONT_START) {
        restartPlayback = 1;
    }*/

    return restartPlayback;
}


struct MainPoolState {
    u32 freeSpace;
    struct MainPoolBlock *listHeadL;
    struct MainPoolBlock *listHeadR;
    struct MainPoolState *prev;
};

struct LevelCommand {
    /*00*/ u8 type;
    /*01*/ u8 size;
    /*02*/ // variable sized argument data
};

extern u32 sSegmentTable[32];
extern u32 sPoolFreeSpace;
extern s16 sTransitionTimer;
extern u8 sTransitionColorFadeCount[4];
extern u8 gWarpTransRed;
extern u8 gWarpTransGreen;
extern u8 gWarpTransBlue;
extern s16 gDialogID;

LevelScript localCmds[4]; // temp buffer for running custom level script commands

// ptrs to static vars
s32 *sRegister = 0x8038be24;
u16 *sDelayFrames = 0x8038B8A4;
u16 *sDelayFrames2 = 0x8038B8A8;
s16 *sCurrAreaIndex = 0x8038B8AC;
LevelScript **thread5GameLoopVar_addr = 0x80206DEC;
LevelScript **sCurrentCmd = 0x8038BE28;
u32 **sStackTop = 0x8038b8b0;
u32 **sStackBase = 0x8038b8b4;
u32 *sStack = 0x8038BDA0;
void (*level_cmd_exit)() = 0x8037e404;
void (*level_cmd_load_and_execute)(void) = 0x8037e2c4;
void (*level_cmd_exit_and_execute)(void) = 0x8037e388;
struct AllocOnlyPool **sLevelPool = 0x8038b8a0;
struct MainPoolState **gMainPoolState = 0x8032dd70;
void (**LevelScriptJumpTable)(void) = 0x8038b8b8;
int i;

void update_recording() {
    gMarioState->numLives = lagCounter % 100;
    if (do_control()) {
        // init_level
        init_graph_node_start(NULL, (struct GraphNodeStart *) &gObjParentGraphNode);
        clear_objects();
        clear_area_graph_nodes();
        clear_areas();

        while ((*gMainPoolState) != NULL) { // restore pool to initial state for level_script_entry
            main_pool_pop_state();
        }

        gHudDisplay.flags = 0; // prevent crash on hud

        // reset level script stack
        *sStackTop = sStack;
        *sStackBase = NULL;

        // load and execute level_main_scripts_entry
        (*sCurrentCmd) = localCmds;
        (*sCurrentCmd)[0] = CMD_BBH(0x00, 0x10, 0x15);
        (*sCurrentCmd)[1] = CMD_PTR(_scriptsSegmentRomStart);
        (*sCurrentCmd)[2] = CMD_PTR(_scriptsSegmentRomEnd);
        (*sCurrentCmd)[3] = CMD_PTR(level_main_scripts_entry);
        level_cmd_load_and_execute();
        *sStack = 0x80064F60; // restore reference to level_script_entry

        *sRegister = 9; // bob-omhb battlefield
        *sDelayFrames = 0;
        *sDelayFrames2 = 0;

        for (i = 0; i < 58; i++) { // advance to level_main_menu_entry_2[1]
            LevelScriptJumpTable[((struct LevelCommand*)(*sCurrentCmd))->type]();
        }
        *sRegister = 0; // bypass star select by overwriting lvl_set_current_level return value to 0

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

        gMarioStates->action = 1; // crashes if uninitialized

        *thread5GameLoopVar_addr = *sCurrentCmd;
    }
}
