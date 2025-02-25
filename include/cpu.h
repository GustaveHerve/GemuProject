#ifndef CORE_CPU_H
#define CORE_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

    uint8_t ime;
};

void cpu_set_registers_post_boot(struct cpu *cpu, int checksum);

void serialize_cpu_to_stream(FILE *stream, struct cpu *cpu);

void load_cpu_from_stream(FILE *stream, struct cpu *cpu);

#endif
