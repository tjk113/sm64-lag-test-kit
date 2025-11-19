#include <ultra64.h>
#include <stdio.h>

#include "sm64.h"

#include "segments.h"
#include "segment_symbols.h"

#include "game/game_init.h"
#include "game/memory.h"

#include "custom.h"
#include "init.h"

#define ALIGN16(val) (((val) + 0xF) & ~0xF)

static struct Hook sHookTargets[] = {
    { run_demo_inputs, run_demo_inputs_hook },
    { display_and_vsync, display_and_vsync_hook }
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

    for (i = 0; i < ARRAY_COUNT(sHookTargets); i++) {
        struct Hook *hook = &sHookTargets[i];
        u32 *cur = (u32 *) hook->target;

        // jr ra
        while (*cur != 0x03e00008) {
            cur++;
        }

        cur[0] = 0x08000000 | (((u32) hook->dest & 0xffffff) >> 2); // j hook->dest
        osInvalICache(&cur[0], 4);
    }
}
