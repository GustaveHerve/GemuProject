#ifndef CPU_H
#define CPU_H

#include <stddef.h>
#include <stdint.h>

struct cpu
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

    int ime;
};

void cpu_start(struct cpu *cpu);

void cpu_set_registers_post_boot(struct cpu *cpu, int checksum);

#endif
