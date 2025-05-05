#ifndef CUSTOM_H
#define CUSTOM_H

#include "game/profiler.h"

enum HookId {
	HOOK_THREAD5,
	HOOK_THREAD4,
	HOOK_GFX,
	HOOK_VBLANK
};

struct Hook {
	enum HookId id;
	void *func;
};

extern void custom_hook(int eventId, enum HookId hookId);

#endif
