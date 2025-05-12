#include <game/profiler.h>

#include "lag.h"
#include "playback.h"

void custom_entry(void *func, s32 eventId) {
    if (func == profiler_log_thread5_time) {
        if (eventId == THREAD5_START) {
            curFrameViCount = 0;
        } else if (eventId == INPUT_POLL) {
            update_recording();
        }
    } else if (func == profiler_log_vblank_time) {
        update_lag();
    }
}
