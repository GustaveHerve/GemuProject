#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#include "cpu.h"

/* CPU flags manipulation */
static inline int get_z(struct cpu *cpu)
{
    return (cpu->f >> 7) & 1UL;
}

static inline void set_z(struct cpu *cpu, int value)
{
    if (value)
        cpu->f = cpu->f | 1UL << 7;
    else
        cpu->f = cpu->f & ~(1UL << 7);
}

static inline int get_n(struct cpu *cpu)
{
    return (cpu->f >> 6) & 1UL;
}

static inline void set_n(struct cpu *cpu, int value)
{
    if (value)
        cpu->f = cpu->f | 1UL << 6;
    else
        cpu->f = cpu->f & ~(1UL << 6);
}

static inline int get_h(struct cpu *cpu)
{
    return (cpu->f >> 5) & 1UL;
}

static inline void set_h(struct cpu *cpu, int value)
{
    if (value)
        cpu->f = cpu->f | 1UL << 5;
    else
        cpu->f = cpu->f & ~(1UL << 5);
}

static inline int get_c(struct cpu *cpu)
{
    return (cpu->f >> 4) & 1UL;
}

static inline void set_c(struct cpu *cpu, int value)
{
    if (value)
        cpu->f = cpu->f | 1UL << 4;
    else
        cpu->f = cpu->f & ~(1UL << 4);
}

// 8 bit and 16 bit manipulations
static inline uint8_t regist_hi(uint16_t *rr)
{
    uint8_t res = (*rr >> 8) & 0xFF;
    return res;
}

static inline uint8_t regist_lo(uint16_t *rr)
{
    uint8_t res = *rr & 0xFF;
    return res;
}

static inline uint8_t get_lsb_nibble(uint8_t b)
{
    return b & 0xF;
}

static inline uint8_t get_msb_nibble(uint8_t b)
{
    return (b >> 4) & 0xF;
}

static inline uint16_t convert_8to16(uint8_t *hi, uint8_t *lo)
{
    uint16_t res = 0;
    res = (res | *hi) << 8;
    res = res | *lo;
    return res;
}

////////////////
// Flags methods
////////////////

static inline int hflag_check(uint8_t result)
{
    return (result & 0x10) == 0x10;
}

static inline int hflag_add_check(uint8_t a, uint8_t b)
{
    return hflag_check(get_lsb_nibble(a) + get_lsb_nibble(b));
}

static inline void hflag_add_set(struct cpu *cpu, uint8_t a, uint8_t b)
{
    set_h(cpu, hflag_add_check(a, b));
}

static inline int hflag_sub_check(uint8_t a, uint8_t b)
{
    return hflag_check(get_lsb_nibble(a) - get_lsb_nibble(b));
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

// Rotations, returns 1 if carry else 0
static inline void rotl(uint8_t *src)
{
    *src = (*src << 1) | (*src >> (8 - 1));
}

static inline void rotl_carry(struct cpu *cpu, uint8_t *src)
{
    rotl(src);
    int a = *src & 1UL; // get lsb
    int b = get_c(cpu);
    if (b)
        *src = *src | 1UL;
    else
        *src = *src & ~(1UL);
    set_c(cpu, a);
}

static inline void rotr(uint8_t *src)
{
    *src = (*src >> 1) | (*src << (8 - 1));
}

static inline void rotr_carry(struct cpu *cpu, uint8_t *src)
{
    rotr(src);
    int a = (*src >> 7) & 1UL; // get msb
    int b = get_c(cpu);
    if (b)
        *src = *src | (1UL << 7);
    else
        *src = *src & ~(1UL << 7);

    set_c(cpu, a);
}

static inline void rotl_16(uint16_t *src)
{
    *src = (*src << 1) | (*src >> (16 - 1));
}

static inline void rotr_16(uint16_t *src)
{
    *src = (*src >> 1) | (*src << (16 - 1));
}

#endif
