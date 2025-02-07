#ifndef CPU_H
#define CPU_H

#include <stddef.h>
#include <stdint.h>

struct ppu;
struct renderer;
struct mbc_generic;

#define MEMBUS_SIZE 65536
#define CPU_FREQUENCY 4194304
#define CPU_FREQUENCY_MCYCLE (4194304 / 4)

struct cpu
{
    struct
    {
        uint8_t a;
        uint8_t f;
        uint8_t b;
        uint8_t c;
        uint8_t d;
        uint8_t e;
        uint8_t h;
        uint8_t l;

        uint16_t sp;
        uint16_t pc;
    } regist;

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
};

void cpu_init(struct cpu *cpu, struct renderer *rend);

void cpu_start(struct cpu *cpu);

void cpu_free(struct cpu *cpu);

void cpu_set_registers_post_boot(struct cpu *cpu, int checksum);

#endif
