#ifndef CORE_MEMORY_WRITE_H
#define CORE_MEMORY_WRITE_H

#include <stdint.h>
struct gb_core;

void write_mem(struct gb_core *gb, uint16_t address, uint8_t val);

#endif
