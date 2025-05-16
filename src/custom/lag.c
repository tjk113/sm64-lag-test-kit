#include <game/profiler.h>
#include <types.h>

u32 lagCounter = 0;
u32 curFrameViCount = 0;
u8 logParity = FALSE;
OSTime lastTime = 0;

void update_lag() { // no clue how reliable this would be on console, need a proper hook per vi really
    if (logParity) {
        curFrameViCount++;
        if (curFrameViCount > 2) {
            lagCounter++;
        }
    }
    logParity = !logParity;
}
