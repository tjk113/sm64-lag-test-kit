#include <string.h>

#include <game/area.h>
#include <game/game_init.h>
#include <game/level_update.h>
#include <game/print.h>

#include "custom.h"
#include "lag.h"
#include "level_load.h"
#include "playback.h"


extern u8 data[];
u32 marker = 0x5A83B1C0;
u8 *data_ptr = data;
u32 recordingCount = 0;
u8 data[0x10000] = { 0 };

struct RecordingHeader curRec;
u16 buttonsDownLastFrame = 0;
u16 recordingIndex = 0;
u16 frame = 0;
u8 camControl = FALSE;
u8 playbackInit = FALSE;

void write_mem_blocks() {
    u8 *curData = curRec.stateData;
    u16 i = 0;
    while (i < curRec.memBlocksLength) {
        struct MemBlock memBlock = curRec.stateMemBlocks[i];
        memcpy(memBlock.addr, curData, memBlock.size);
        curData += memBlock.size;
        i++;
    }
}

void load_state() {
    write_mem_blocks();
    load_level(&curRec.levelLoadParams);
    write_mem_blocks();
}

void write_inputs() {
    struct RecordingFrame curInputs = curRec.inputs[frame];

    u16 button = curInputs.button;
    if (camControl) {
        button = (button & 0xFFE0) | (gControllerPads[0].button & 0x001F);
    }

    gControllerPads[0].button = button;
    gControllerPads[0].stick_x = curInputs.stickX; 
    gControllerPads[0].stick_y = curInputs.stickY;

    if (gCurrentArea != NULL) {
        struct Camera *areaCam = gCurrentArea->camera;
        if (areaCam != NULL) {
            areaCam->yaw = curInputs.camYaw;
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
        if (frame == 1) {
            lagCounter = 0;
        }
        write_inputs();
    }
}

u8 do_control() {
    u8 restartPlayback = 0;
    u16 buttonsPressed = gControllerPads[0].button & ~buttonsDownLastFrame;
    buttonsDownLastFrame = gControllerPads[0].button;

    if (buttonsPressed & CONT_UP) { // toggle cam control
        camControl ^= 1;
    }
    if (buttonsPressed & CONT_L) { // advance to next recording + restart
        recordingIndex++;
        if (recordingIndex == recordingCount) {
            recordingIndex = 0;
        }
        curRec = *(struct RecordingHeader*)data;
        camControl = FALSE;
        restartPlayback = 1;
    }
    if (buttonsPressed & CONT_START) { // restart current
        restartPlayback = 1;
    }

    return restartPlayback;
}


void update_playback() {
    if (!playbackInit) {
        if (recordingCount == 0) {
            print_text(20, 20, "No recordings loaded");
        } else {
            curRec = *(struct RecordingHeader*)data;
            restart_playback();
            playbackInit = TRUE;
        }
    } else {
        if (do_control()) {
            restart_playback();
        } else {
            advance_playback();
        }
        gMarioStates->numLives = lagCounter % 100;
    }
    /*
    if (gGlobalTimer < START_TIMER) {
        return;
    } else if (gGlobalTimer == START_TIMER) {
        curRec = *((struct RecordingHeader*)data);
        restart_playback();
    } else {
        if (do_control()) {
            restart_playback();
        } else {
            advance_playback();
        }
    }*/
}
