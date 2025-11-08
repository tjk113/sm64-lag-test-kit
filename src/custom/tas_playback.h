#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <types.h>
#include "level_load.h"

#define CAM_CONTROL_OFF     0
#define CAM_CONTROL_ON      1
#define CAM_CONTROL_MARIO   2
#define CAM_CONTROL_FIXED   4

struct RecordingFrame {
    u16 button;
	s8 stickX;
	s8 stickY;
    u16 camYaw;
    s16 cameraMovementFlags;
    s16 cameraSelectionFlags;
    u16 gRandomSeed16;
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
