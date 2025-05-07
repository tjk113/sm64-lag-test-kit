#ifndef INIT_H
#define INIT_H

struct Hook {
    void *func;
    u32 wrapper[3];
};

extern void custom_init(void);

#endif
