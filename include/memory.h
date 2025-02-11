#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stdint.h>
struct gb_core;

void write_mem(struct gb_core *gb, uint16_t address, uint8_t val);

uint8_t read_mem(struct gb_core *gb, uint16_t address);

uint8_t read_mem_tick(struct gb_core *gb, uint16_t address);

#endif
