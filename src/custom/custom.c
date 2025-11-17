#include <game/profiler.h>

#include "lag.h"
#include "tas_playback.h"

u32 sStartDelayFrames = 5;

void custom_entry(void *func, s32 eventId) {
    if (func == profiler_log_thread5_time) {
        if (eventId == THREAD5_END) {
            gCurFrameViCount = 0;
        } else if (eventId == INPUT_POLL) {
            if (sStartDelayFrames > 0) {
                sStartDelayFrames--;
                return;
            }
            update_playback();
        }
    } else if (func == profiler_log_vblank_time) {
        update_lag();
    }
}
