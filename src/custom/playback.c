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

#define START_TIMER 500

struct MainPoolState {
    u32 freeSpace;
    struct MainPoolBlock *listHeadL;
    struct MainPoolBlock *listHeadR;
    struct MainPoolState *prev;
};


extern struct MainPoolState *gMainPoolState;

void update_recording() {
    u8 i;
    LevelScript **p = 0x80206DEC;
    LevelScript *sCurrentCmd = 0x8038BE28;
    u32 *sStackTop = 0x8038b8b0;
    u32 *sStackBase = 0x8038b8b4;
    void (*level_cmd_exit)() = 0x8037e404;
    void (*level_cmd_load_and_execute)(void) = 0x8037e2c4;
    void (*level_cmd_exit_and_execute)(void) = 0x8037e388;
    struct MainPoolState **gMainPoolState = 0x8032dd70;
    struct AllocOnlyPool **sLevelPool = 0x8038b8a0;
    s32 *sRegister = 0x8038be24;
    s32 *sPoolFreeSpace = 0x8033b480;
    s16 *sCurrAreaIndex = 0x8038b8ac;
    u32 *gObjParentGraphNode = 0x08038bd88;
    u32 addr;

    if (do_control()) {
        curRec = *((struct RecordingHeader*)data);
        //restart_playback();
        clear_objects();
        clear_area_graph_nodes();
        clear_areas();
        while ((*gMainPoolState) != NULL) {
            *sPoolFreeSpace = (*gMainPoolState)->freeSpace;
            main_pool_pop_state();
        }
        gHudDisplay.flags = 0;
        *sStackTop = 0x8038BDA0;
        *sStackBase = 0x8038BDA0;
        //curCmd[0] = EXIT();
        //level_cmd_exit();
        *sCurrentCmd = segmented_to_virtual(0x80064F60 - 0x1C);
        /*sRegister = 16;
        addr = (u32)load_segment(0x15, _scriptsSegmentRomStart, _scriptsSegmentRomEnd, MEMORY_POOL_LEFT) & 0x1FFFFFFF;
        a = addr;
        sCurrentCmd[0] = addr | 0x80000000;*/
        /*sCurrentCmd[0] = CMD_BBH(0x00, 0x10, 0x15);
        sCurrentCmd[1] = CMD_PTR(_scriptsSegmentRomStart);
        sCurrentCmd[2] = CMD_PTR(_scriptsSegmentRomEnd);
        sCurrentCmd[3] = CMD_PTR(level_main_scripts_entry);
        level_cmd_load_and_execute();*/

        *p = *sCurrentCmd;
    } else {
        /*if (do_control()) {
            restart_playback();
        } else {
            advance_playback();
        }*/
    }
    gMarioStates->numLives = lagCounter % 100;
}
