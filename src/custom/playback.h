#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <types.h>
#include "level_load.h"

struct RecordingFrame {
    u16 button;
	s8 stickX;
	s8 stickY;
    u16 camYaw;
};

struct MemBlock {
    void *addr;
    u32 size;
};

struct RecordingHeader {
    u16 memBlocksLength;
    u16 length;
    struct LevelLoadParams levelLoadParams;
    struct MemBlock *stateMemBlocks;
    u8 *stateData;
    struct RecordingFrame *inputs;
};

extern void update_playback();

#endif
