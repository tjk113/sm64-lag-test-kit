#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <types.h>

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

extern void update_recording();

#endif
