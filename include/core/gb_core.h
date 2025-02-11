#ifndef CORE_GB_H
#define CORE_GB_H

#include "apu.h"
#include "common.h"
#include "cpu.h"
#include "ppu.h"

struct gb_core
{
    // Components
    struct cpu cpu;
    struct ppu ppu;
    struct apu apu;

    uint8_t *membus;

    // Internal registers
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
    struct
    {
        int (*get_queued_audio_sample_count)(void);
        int (*queue_audio)(void *);
        void (*handle_events)(struct gb_core *);
        int (*render_frame)(void);
    } callbacks;
};

static inline uint8_t is_apu_on(struct gb_core *gb)
{
    return gb->membus[NR52] >> 7;
}

static inline uint8_t is_dac_on(struct gb_core *gb, uint8_t number)
{
    if (number == 3)
        return gb->membus[NR30] >> 7;
    unsigned int nrx2 = NR12 + ((NR22 - NR12) * (number - 1));
    return gb->membus[nrx2] & 0xF8;
}

static inline uint8_t is_channel_on(struct gb_core *gb, uint8_t number)
{
    return (gb->membus[NR52] >> (number - 1)) & 0x01;
}

static inline void turn_channel_off(struct gb_core *gb, uint8_t number)
{
    gb->membus[NR52] &= ~(1 << (number - 1));
}

static inline void turn_channel_on(struct gb_core *gb, uint8_t number)
{
    gb->membus[NR52] |= 1 << (number - 1);
}

void init_gb_core(struct gb_core *gb);

void init_gb_core_post_boot(struct gb_core *gb, int checksum);

void free_gb_core(struct gb_core *gb);

#endif
