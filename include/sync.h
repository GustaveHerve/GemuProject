#ifndef CORE_SYNC_H
#define CORE_SYNC_H

#include <stdint.h>
struct gb_core;

int64_t get_nanoseconds(void);

void synchronize(struct gb_core *gb);

#endif
