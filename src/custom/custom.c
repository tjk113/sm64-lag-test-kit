#include <ultra64.h>
#include <stdio.h>

#include "sm64.h"

#include "game/profiler.h"

#include "custom.h"

void custom_hook(s32 eventId, enum HookId hookId) {
	if (hookId == HOOK_THREAD5 && eventId == LEVEL_SCRIPT_EXECUTE) {
	    print_text(25, 60, "HI");
	}
}
