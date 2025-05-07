#include <ultra64.h>
#include <stdio.h>
#include <string.h>

#include "sm64.h"

#include "game/profiler.h"

#include "custom.h"
#include <game/game_init.h>
#include <game/level_update.h>
#include <lib/src/controller.h>

u32 marker = 0x5A83B1C0;
u32 recording_count = 0;
u8 data[0x10000] = { 0 };

struct RecordingFrame {
    u16 button;
	s8 stick_x;
	s8 stick_y;
    u16 cam_yaw;
};

struct MemBlock {
    void *addr;
    u32 size;
};

struct RecordingHeader {
    u16 mem_blocks_length;
    u16 length;
    struct MemBlock *state_mem_blocks;
    u8 *state_data;
    struct RecordingFrame *inputs;
};

struct RecordingHeader cur_rec;
u16 recording_index = 0;
u16 frame = 0;

void loadState() {
    u8 *cur_data = cur_rec.state_data;
    u16 i = 0;
    while (i < cur_rec.mem_blocks_length) {
        struct MemBlock mem_block = cur_rec.state_mem_blocks[i];
        memcpy(mem_block.addr, cur_data, mem_block.size);
        cur_data += mem_block.size;
        i++;
    }
}

void startPlayback() {
    frame = 0;
    //loadState();
}

void writeInputs() {
    struct RecordingFrame cur_inputs = cur_rec.inputs[frame];
    gControllerPads[0].button = cur_inputs.button;
    gControllerPads[0].stick_x = cur_inputs.stick_x;
    gControllerPads[0].stick_y = cur_inputs.stick_y;
}

void advancePlayback() {
    if (frame == cur_rec.length) {
        startPlayback();
    } else {
        writeInputs();
        frame++;
    }
}

u8 doControl() {
    return 0;
}

#define START_TIMER 500

void entry() {
    if (gGlobalTimer < START_TIMER) {
        return;
    } else if (gGlobalTimer == START_TIMER) {
        cur_rec = *((struct RecordingHeader *) data);
        startPlayback();
    } else {
        if (doControl()) {
            startPlayback();
        } else {
            advancePlayback();
        }
    }
    gMarioStates->numLives = frame;
}

void custom_entry(void *func, s32 eventId) {
    if (func == profiler_log_thread5_time) {
        entry();
    }
}
