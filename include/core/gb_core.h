#ifndef GB_H
#define GB_H

#include "apu.h"
#include "cpu.h"
#include "ppu.h"

struct gb_core
{
    // Components
    struct cpu cpu;
    struct ppu ppu;
    struct apu apu;

    // Internal registers
    uint8_t *membus;
    int ime;

    uint16_t previous_div;
    uint16_t internal_div;

    uint8_t disabling_timer;
    uint8_t schedule_tima_overflow;

    int halt;
    int stop;

    uint8_t serial_clock;
    uint8_t serial_acc;

    struct mbc_base *mbc;

    uint8_t joyp_a;
    uint8_t joyp_d;

    size_t tcycles_since_sync;
    int64_t last_sync_timestamp;

    // Callbacks
    struct callbacks
    {

    } callbacks;
};

void init_gb_core(struct gb_core *gb);

void free_gb_core(struct gb_core *gb);

#endif
