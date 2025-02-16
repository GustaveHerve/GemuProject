#ifndef CORE_SERIAL_H
#define CORE_SERIAL_H

#include "gb_core.h"

static inline uint8_t get_clock_select(struct gb_core *gb)
{
    return gb->memory.io[IO_OFFSET(SC)] & 0x01;
}

static inline uint8_t get_transfer_enable(struct gb_core *gb)
{
    return (gb->memory.io[IO_OFFSET(SC)] >> 7) & 0x01;
}

void serial_transfer(struct gb_core *gb);

void update_serial(struct gb_core *gb);

#endif
