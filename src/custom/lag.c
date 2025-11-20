#include <PR/ultratypes.h>

#include "game/main.h"

u32 sPrevNumVblanks = 0;
u32 gLagCounter = 0;

void update_lag(void) {
    // FIXME: use more reliable vi counter
    u32 lagFrame = gNumVblanks - (sPrevNumVblanks + 2);
    sPrevNumVblanks = gNumVblanks;
    gLagCounter += lagFrame;
}
