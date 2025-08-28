#ifndef CORE_CPU_H
#define CORE_CPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "utils.h"

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

void cpu_serialize(FILE *stream, struct cpu *cpu);

void cpu_load_from_stream(FILE *stream, struct cpu *cpu);

static inline bool get_z(struct cpu *cpu)
{
    return cpu->f >> 7;
}

static inline void set_z(struct cpu *cpu, int value)
{
    if (value)
        cpu->f = cpu->f | 1 << 7;
    else
        cpu->f = cpu->f & ~(1 << 7);
}

static inline bool get_n(struct cpu *cpu)
{
    return (cpu->f >> 6) & 1;
}

static inline void set_n(struct cpu *cpu, int value)
{
    if (value)
        cpu->f = cpu->f | 1 << 6;
    else
        cpu->f = cpu->f & ~(1 << 6);
}

static inline bool get_h(struct cpu *cpu)
{
    return (cpu->f >> 5) & 1;
}

static inline void set_h(struct cpu *cpu, int value)
{
    if (value)
        cpu->f = cpu->f | 1 << 5;
    else
        cpu->f = cpu->f & ~(1 << 5);
}

static inline bool get_c(struct cpu *cpu)
{
    return (cpu->f >> 4) & 1;
}

static inline void set_c(struct cpu *cpu, int value)
{
    if (value)
        cpu->f = cpu->f | 1 << 4;
    else
        cpu->f = cpu->f & ~(1 << 4);
}

static inline int hflag_check(uint8_t result)
{
    return (result & 0x10) == 0x10;
}

static inline int hflag_add_check(uint8_t a, uint8_t b)
{
    return hflag_check((a & 0xF) + (b & 0xF));
}

static inline void hflag_add_set(struct cpu *cpu, uint8_t a, uint8_t b)
{
    set_h(cpu, hflag_add_check(a, b));
}

static inline int hflag_sub_check(uint8_t a, uint8_t b)
{
    return hflag_check((a & 0xF) - (b & 0xF));
}

static inline void hflag_sub_set(struct cpu *cpu, uint8_t a, uint8_t b)
{
    set_h(cpu, hflag_sub_check(a, b));
}

static inline int hflag16_add_check(uint16_t a, uint16_t b)
{
    return (((a & 0xFFF) + (b & 0xFFF)) & 0x1000) == 0x1000;
}

static inline void hflag16_add_set(struct cpu *cpu, uint16_t a, uint16_t b)
{
    set_h(cpu, hflag16_add_check(a, b));
}

static inline int cflag_rotl_check(uint8_t src)
{
    return (src & 0x01) == 0x01;
}

static inline void cflag_rotl_set(struct cpu *cpu, uint8_t src)
{
    set_c(cpu, cflag_rotl_check(src));
}

static inline int cflag_rotr_check(uint8_t src)
{
    return (src & 0x80) == 0x80;
}

static inline void cflag_rotr_set(struct cpu *cpu, uint8_t src)
{
    set_c(cpu, cflag_rotr_check(src));
}

static inline int cflag_add_check(uint8_t a, uint8_t b)
{
    return ((a + b) & 0x100) == 0x100;
}

static inline void cflag_add_set(struct cpu *cpu, uint8_t a, uint8_t b)
{
    set_c(cpu, cflag_add_check(a, b));
}

static inline int cflag_sub_check(uint8_t a, uint8_t b)
{
    return ((a - b) & 0x100) == 0x100;
}

static inline void cflag_sub_set(struct cpu *cpu, uint8_t a, uint8_t b)
{
    set_c(cpu, cflag_sub_check(a, b));
}

static inline int cflag16_add_check(uint16_t a, uint16_t b)
{
    return ((a + b) & 0x10000) == 0x10000;
}

static inline void cflag16_add_set(struct cpu *cpu, uint16_t a, uint16_t b)
{
    set_c(cpu, cflag16_add_check(a, b));
}

static inline void rotl_carry(struct cpu *cpu, uint8_t *src)
{
    rotl(src);
    int a = *src & 1;
    int b = get_c(cpu);
    if (b)
        *src |= 1;
    else
        *src &= ~(1);
    set_c(cpu, a);
}

static inline void rotr_carry(struct cpu *cpu, uint8_t *src)
{
    rotr(src);
    int a = (*src >> 7) & 1;
    int b = get_c(cpu);
    if (b)
        *src |= (1 << 7);
    else
        *src &= ~(1 << 7);

    set_c(cpu, a);
}

#endif
