#include <game/profiler.h>
#include <types.h>

u32 lagCounter = 0;
OSTime lastTime = 0;

void update_lag() { // no clue how reliable this would be on console, need a proper hook per vi really
    OSTime time = osGetTime();
    if (lastTime != 0) {
        OSTime cycleDiff = time - lastTime;
        OSTime cyclesPerVI = osClockRate / 60;
        u32 numVIs = (cycleDiff + (cyclesPerVI >> 1)) / cyclesPerVI;
        if (numVIs > 2) {
            lagCounter += numVIs - 2;
        }
    }
    lastTime = time;
}
