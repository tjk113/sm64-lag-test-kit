#include <string.h>

#include <game/area.h>
#include <game/game_init.h>
#include <game/level_update.h>

#include "custom.h"
#include "lag.h"
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
u8 camControl = 0;


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
        restartPlayback = 1;
    }
    if (buttonsPressed & CONT_START) {
        restartPlayback = 1;
    }

    return restartPlayback;
}

#define START_TIMER 500

void update_recording() {
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
    }
    gMarioStates->numLives = lagCounter % 100;
}
