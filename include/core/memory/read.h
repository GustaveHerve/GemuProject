#ifndef CORE_MEMORY_READ_H
#define CORE_MEMORY_READ_H

#include <stdint.h>

struct gb_core;

uint8_t read_mem(struct gb_core *gb, uint16_t address);

uint8_t read_mem_tick(struct gb_core *gb, uint16_t address);

#endif
