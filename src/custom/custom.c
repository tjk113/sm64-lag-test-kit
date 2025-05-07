#include <ultra64.h>
#include <stdio.h>

#include "sm64.h"

#include "game/profiler.h"

#include "custom.h"

void custom_entry(void *func, s32 eventId) {
    if (func == profiler_log_thread5_time) {
        print_text_fmt_int(25 + eventId * 15, 60, "%d", eventId);
    }
}
