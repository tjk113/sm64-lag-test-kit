#ifndef INIT_H
#define INIT_H

struct Hook {
    void *target;
    void *dest;
};

void custom_init(void);

#endif // INIT_H
