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
    { profiler_log_thread5_time, { 0 } },
    { profiler_log_thread4_time, { 0 } },
    { profiler_log_gfx_time, { 0 } },
    { profiler_log_vblank_time, { 0 } },
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

    load_engine_code_segment();

    for (i = 0; i < sizeof(sHookTargets) / sizeof(sHookTargets[0]); i++) {
        struct Hook *hook = &sHookTargets[i];
        u32 *cur = (u32 *) hook->func;

        // jr ra
        while (*cur != 0x03e00008) {
            cur++;
        }

        cur[1] = cur[-1]; // move stack addiu to delay slot
        cur[-1] = 0x8fa50018; // lw a1, 24(sp) (before frame is pushed out)
        cur[0] = 0x08000000 | (((u32) hook->wrapper & 0xffffff) >> 2); // j hook->wrapper
        osInvalICache(&cur[-1], 12);

        hook->wrapper[0] = 0x3c040000 | ((u32) hook->func >> 16); // lui a0, uhi hook->func
        hook->wrapper[1] = 0x08000000 | (((u32) custom_entry & 0xffffff) >> 2); // j custom_entry
        hook->wrapper[2] = 0x34840000 | ((u32) hook->func & 0xffff); // ori a0, a0, ulo hook->func
        osInvalICache(hook->wrapper, 12);
    }
}
