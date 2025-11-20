#include <string.h>

#include "buffers/buffers.h"
#include "game/area.h"
#include "game/camera.h"
#include "game/game_init.h"
#include "game/level_update.h"
#include "game/print.h"
#include "sm64.h"

#include "custom.h"
#include "lag.h"
#include "level_load.h"
#include "tas_playback.h"

#define DATA_SIZE 0x20000

extern u8 sData[];

u32 sMarker = 0x5A83B1C0;
u8 *sDataPtr = sData;
u32 sDataSize = DATA_SIZE;
u32 sRecordingCount = 0;
u8 sData[DATA_SIZE] = { 0 };

struct RecordingHeader sCurRec;
u32 sLoadedRecordingIndex = 0;
u32 sScheduledRecordingIndex = 0;
u16 sFrame = 0;
u16 sButtonsDownLastFrame = 0;
u8 sCamControl = CAM_CONTROL_OFF;
u8 sPlaybackInit = FALSE;
u8 sRestartPlaybackScheduled = FALSE;

u16 *const gRandomSeed16 = (u16 *const) 0x8038EEE0;

u8 load_recording_data(u32 index) {
    if (!sPlaybackInit || sLoadedRecordingIndex != index) {
        sCurRec = ((struct RecordingHeader *) sData)[index];
        sLoadedRecordingIndex = index;
        return TRUE;
    }
    return FALSE;
}

void write_mem_blocks(void) {
    u8 *curData = sCurRec.stateData;
    u16 i = 0;
    while (i < sCurRec.memBlocksLength) {
        struct MemBlock memBlock = sCurRec.stateMemBlocks[i];
        memcpy(memBlock.addr, curData, memBlock.size);
        curData += memBlock.size;
        i++;
    }
}

void load_state(void) {
    write_mem_blocks();
    load_level(&sCurRec.levelLoadParams);
    write_mem_blocks();
}

void write_inputs(void) {
    struct RecordingFrame curInputs = sCurRec.inputs[sFrame];
    u16 button = curInputs.button;

    if (sCamControl & CAM_CONTROL_ON) {
        button = (button & ~(R_TRIG | C_BUTTONS)) | (gControllerPads[0].button & (R_TRIG | C_BUTTONS));

        if (sCamControl & CAM_CONTROL_MARIO) {
            sSelectionFlags |= CAM_MODE_MARIO_SELECTED;
        } else if (sCamControl & CAM_CONTROL_FIXED) {
            sSelectionFlags &= ~CAM_MODE_MARIO_SELECTED;
        }
    } else {
        sSelectionFlags = curInputs.cameraSelectionFlags;
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
    gCameraMovementFlags = (curInputs.cameraMovementFlags & (CAM_MOVING_INTO_MODE | CAM_MOVE_C_UP_MODE))
                           | (gCameraMovementFlags & ~(CAM_MOVING_INTO_MODE | CAM_MOVE_C_UP_MODE));

    *gRandomSeed16 = curInputs.randomSeed16;
}

u8 can_restart_playback(u32 index) {
    u8 *curData;
    u16 i = 0;

    struct RecordingHeader *rec = &((struct RecordingHeader *) sData)[index];
    u32 recGlobalTimer = gGlobalTimer;

    curData = rec->stateData;
    while (i < rec->memBlocksLength) {
        struct MemBlock memBlock = rec->stateMemBlocks[i];
        if (memBlock.addr == &gGlobalTimer && memBlock.size == sizeof(gGlobalTimer)) {
            memcpy(&recGlobalTimer, curData, memBlock.size);
            break;
        }
        curData += memBlock.size;
        i++;
    }

    return (recGlobalTimer % ARRAY_COUNT(gGfxPools)) == (gGlobalTimer % ARRAY_COUNT(gGfxPools));
}

u8 restart_playback(void) {
    u32 index = sScheduledRecordingIndex;
    u8 restarted;

    if (can_restart_playback(index)) {
        if (load_recording_data(index)){
            sCamControl = CAM_CONTROL_OFF;
        }
        sFrame = 0;
        gLagCounter = 0;
        load_state();
        write_inputs();
        sRestartPlaybackScheduled = FALSE;
        restarted = TRUE;
    } else {
        sRestartPlaybackScheduled = TRUE;
        restarted = FALSE;
    }

    return restarted;
}

void advance_playback(void) {
    if (++sFrame >= sCurRec.length) {
        restart_playback();
    } else {
        if (sFrame == 1) {
            gLagCounter = 0;
        }
        write_inputs();
    }
}

u8 do_control(void) {
    u8 restartPlayback = FALSE;
    u16 buttonsPressed = gControllerPads[0].button & ~sButtonsDownLastFrame;
    sButtonsDownLastFrame = gControllerPads[0].button;

    if (buttonsPressed & D_JPAD) { // orig cam (need to restart if switched cam modes)
        sCamControl = CAM_CONTROL_OFF;
    } else if (buttonsPressed & U_JPAD) { // cam control
        sCamControl = CAM_CONTROL_ON;
    } else if (buttonsPressed & L_JPAD) { // cam control with R -> mario cam
        sCamControl = CAM_CONTROL_ON | CAM_CONTROL_MARIO;
    } else if (buttonsPressed & R_JPAD) { // cam control with R -> fixed cam
        sCamControl = CAM_CONTROL_ON | CAM_CONTROL_FIXED;
    }

    if (buttonsPressed & L_TRIG) { // advance to next recording + restart
        sScheduledRecordingIndex = (sScheduledRecordingIndex + 1) % sRecordingCount;
        if (can_restart_playback(sScheduledRecordingIndex)) {
            restartPlayback = TRUE;
        } else {
            sRestartPlaybackScheduled = TRUE;
        }
    } else if (buttonsPressed & START_BUTTON) { // restart current
        if (can_restart_playback(sScheduledRecordingIndex)) {
            restartPlayback = TRUE;
        } else {
            sRestartPlaybackScheduled = TRUE;
        }
    }

    return restartPlayback;
}

void update_playback(void) {
    if (!sPlaybackInit) {
        if (sRecordingCount == 0) {
            print_text(20, 20, "No recordings loaded");
        } else {
            sPlaybackInit = restart_playback();
        }
    } else {
        u8 restarted = FALSE;

        if (sRestartPlaybackScheduled || do_control()) {
            restarted = restart_playback();
        }
        if (!restarted) {
            advance_playback();
        }

        gMarioStates[0].numLives = gLagCounter % 100;
    }
}
