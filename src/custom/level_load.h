#ifndef LEVEL_LOAD_H
#define LEVEL_LOAD_H

#include <game/level_update.h>
#include <types.h>

struct LevelLoadParams {
    struct WarpDest warpDest;
    s16 actNum;
    u8 warpTransRed;
    u8 warpTransGreen;
    u8 warpTransBlue;
};

struct LevelCommand {
    /*00*/ u8 type;
    /*01*/ u8 size;
    /*02*/ // variable sized argument data
};

extern void load_level(struct LevelLoadParams *p);

#endif // LEVEL_LOAD_H
