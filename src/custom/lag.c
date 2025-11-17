#include <game/profiler.h>
#include <types.h>

u32 gLagCounter = 0;
u32 gCurFrameViCount = 0;
u8 sLogParity = FALSE;

void update_lag(void) { // no clue how reliable this would be on console, need a proper hook per vi really
    if (sLogParity) {
        gCurFrameViCount++;
        if (gCurFrameViCount > 2) {
            gLagCounter++;
        }
    }
    sLogParity = !sLogParity;
}
