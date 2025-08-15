#ifndef CORE_SYNC_H
#define CORE_SYNC_H

#include <stdint.h>
struct gb_core;

int64_t get_nanoseconds(void);

int64_t synchronize(struct gb_core *gb);

#endif
