#include <game/profiler.h>

#include "playback.h"

void custom_entry(void *func, s32 eventId) {
    if (func == profiler_log_thread5_time) {
        if (eventId == INPUT_POLL) {
            update_recording();
        }
    }
}
