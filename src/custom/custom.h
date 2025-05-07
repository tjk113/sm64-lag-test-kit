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

extern void custom_hook(enum ProfilerGameEvent eventId, enum HookId hookId);
extern void custom_entry(void *func, s32 eventId);

#endif
