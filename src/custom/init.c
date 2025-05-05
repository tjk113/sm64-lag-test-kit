#include <ultra64.h>
#include <stdio.h>

#include "sm64.h"

#include "segments.h"
#include "segment_symbols.h"

#include "game/memory.h"
#include "game/profiler.h"

#include "custom.h"
#include "init.h"

#define ALIGN16(val) (((val) + 0xF) & ~0xF)

static struct Hook sHookTargets[] = {
    { HOOK_THREAD5, profiler_log_thread5_time },
    { HOOK_THREAD4, profiler_log_thread4_time },
    { HOOK_GFX, profiler_log_gfx_time },
    { HOOK_VBLANK, profiler_log_vblank_time },
};

static void load_engine_code_segment(void) {
    void *startAddr = (void *) SEG_ENGINE;
    u32 totalSize = SEG_FRAMEBUFFERS - SEG_ENGINE;
    UNUSED u32 alignedSize = ALIGN16(_engineSegmentRomEnd - _engineSegmentRomStart);

    bzero(startAddr, totalSize);
    osWritebackDCacheAll();
    dma_read(startAddr, _engineSegmentRomStart, _engineSegmentRomEnd);
    osInvalICache(startAddr, totalSize);
    osInvalDCache(startAddr, totalSize);
}

void custom_init(void) {
    u32 i;
    u32 hook = 0x08000000 | (((u32) custom_hook & 0xffffff) >> 2); // j custom_hook

    load_engine_code_segment();

    for (i = 0; i < sizeof(sHookTargets) / sizeof(sHookTargets[0]); i++) {
        u32 *cur = (u32 *) sHookTargets[i].func;

        // jr ra
        while (*cur != 0x03e00008) {
            cur++;
        }

        cur[0] = hook;
        cur[1] = 0x24050000 | sHookTargets[i].id; // li a1, id
        osInvalICache(cur, 8);
    }
}
