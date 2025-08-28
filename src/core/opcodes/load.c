#include <err.h>

#include "emulation.h"
#include "gb_core.h"
#include "read.h"
#include "utils.h"
#include "write.h"

// ld (rr),a
// x(0-1)2	2 MCycle
int ld_rr_a(struct gb_core *gb, uint8_t *hi, uint8_t *lo)
{
    uint16_t address = 0;
    address = unpack16(*hi, *lo);
    write_mem(gb, address, gb->cpu.a);
    return 2;
}

// ld r,r
//		1 MCycle
int ld_r_r(struct gb_core *gb, uint8_t *dest, uint8_t *src)
{
    (void)gb->cpu;
    *dest = *src;
    return 1;
}

// ld r,u8
// x(0-3)(6 or E)	2 MCycle
int ld_r_u8(struct gb_core *gb, uint8_t *dest)
{
    *dest = read_mem_tick(gb, gb->cpu.pc++);
    return 2;
}

// ld (HL),u8
// x36   3 MCycle
int ld_hl_u8(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t n = read_mem_tick(gb, gb->cpu.pc++);
    write_mem(gb, address, n);
    return 3;
}

// ld a,(rr)
// x(0-1)A	2 MCycle
int ld_a_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo)
{
    uint16_t address = unpack16(*hi, *lo);
    uint8_t n = read_mem_tick(gb, address);
    gb->cpu.a = n;
    return 2;
}

// ld (HL),r
// x7(0-5)   2 MCycle
int ld_hl_r(struct gb_core *gb, uint8_t *src)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    write_mem(gb, address, *src);
    return 2;
}

// ld r,(HL)
//		2 MCycle
int ld_r_hl(struct gb_core *gb, uint8_t *dest)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t value = read_mem_tick(gb, address);
    *dest = value;
    return 2;
}

// ld (nn),A
// xEA   4 MCycle
int ld_nn_a(struct gb_core *gb)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.pc++);
    uint16_t address = unpack16(hi, lo);
    write_mem(gb, address, gb->cpu.a);
    return 4;
}

// ld A,(nn)
// xFA   4 MCycle
int ld_a_nn(struct gb_core *gb)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.pc++);
    uint16_t address = unpack16(hi, lo);
    gb->cpu.a = read_mem_tick(gb, address);
    return 4;
}

// ldi (HL+),A
// x22	2 MCycle
int ldi_hl_a(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    write_mem(gb, address, gb->cpu.a);
    ++address;
    gb->cpu.h = address >> 8;
    gb->cpu.l = address & 0xFF;
    return 2;
}

// ldd (HL-),A
// x32	2 MCycle
int ldd_hl_a(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    write_mem(gb, address, gb->cpu.a);
    --address;
    gb->cpu.h = address >> 8;
    gb->cpu.l = address & 0xFF;
    return 2;
}

// ldi A,(HL+)
// x2A	2 MCycle
int ldi_a_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    gb->cpu.a = read_mem_tick(gb, address);
    ++address;
    gb->cpu.h = address >> 8;
    gb->cpu.l = address & 0xFF;
    return 2;
}

// ldd A,(HL-)
// x3A	2 MCycle
int ldd_a_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    gb->cpu.a = read_mem_tick(gb, address);
    --address;
    gb->cpu.h = address >> 8;
    gb->cpu.l = address & 0xFF;
    return 2;
}

////
// 16 bit operations
////

// ld rr,nn
// x(0-2)1	3 MCycle
int ld_rr_nn(struct gb_core *gb, uint8_t *hi, uint8_t *lo)
{
    *lo = read_mem_tick(gb, gb->cpu.pc++);
    *hi = read_mem_tick(gb, gb->cpu.pc++);
    return 3;
}

// ld SP,nn
// x31	3 MCycle
int ld_sp_nn(struct gb_core *gb)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.pc++);
    gb->cpu.sp = unpack16(hi, lo);
    return 3;
}

// ld (nn),SP
// x08	5 MCycle
int ld_nn_sp(struct gb_core *gb)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.pc++);
    uint16_t address = unpack16(hi, lo);
    write_mem(gb, address, gb->cpu.sp & 0xFF);
    write_mem(gb, address + 1, gb->cpu.sp >> 8);
    return 5;
}

// ld HL,SP+e8
// xF8   3 MCycle
int ld_hl_spe8(struct gb_core *gb)
{
    int8_t offset = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t lo = gb->cpu.sp & 0xFF;
    hflag_add_set(&gb->cpu, lo, offset);
    cflag_add_set(&gb->cpu, lo, offset);
    set_z(&gb->cpu, 0);
    set_n(&gb->cpu, 0);
    uint16_t res = gb->cpu.sp + offset;
    tick_m(gb);
    gb->cpu.h = res >> 8;
    gb->cpu.l = res & 0xFF;
    return 3;
}

// ld SP,HL
// xF9   2 MCycle
int ld_sp_hl(struct gb_core *gb)
{
    gb->cpu.sp = unpack16(gb->cpu.h, gb->cpu.l);
    tick_m(gb);
    return 2;
}

int ldh_n_a(struct gb_core *gb)
{
    uint8_t offset = read_mem_tick(gb, gb->cpu.pc++);
    write_mem(gb, 0xFF00 + offset, gb->cpu.a);
    return 3;
}

int ldh_a_n(struct gb_core *gb)
{
    uint8_t offset = read_mem_tick(gb, gb->cpu.pc++);
    gb->cpu.a = read_mem_tick(gb, 0xFF00 + offset);
    return 3;
}

int ldh_a_c(struct gb_core *gb)
{
    gb->cpu.a = read_mem_tick(gb, 0xFF00 + gb->cpu.c);
    return 2;
}

int ldh_c_a(struct gb_core *gb)
{
    write_mem(gb, 0xFF00 + gb->cpu.c, gb->cpu.a);
    return 2;
}

int pop_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo)
{
    uint8_t _lo = read_mem_tick(gb, gb->cpu.sp++);
    uint8_t _hi = read_mem_tick(gb, gb->cpu.sp++);
    *lo = _lo;
    *hi = _hi;
    return 3;
}

int pop_af(struct gb_core *gb)
{
    uint8_t _lo = read_mem_tick(gb, gb->cpu.sp++);
    set_z(&gb->cpu, _lo >> 7 & 0x1);
    set_n(&gb->cpu, _lo >> 6 & 0x1);
    set_h(&gb->cpu, _lo >> 5 & 0x1);
    set_c(&gb->cpu, _lo >> 4 & 0x1);
    uint8_t _hi = read_mem_tick(gb, gb->cpu.sp++);
    gb->cpu.a = _hi;
    return 3;
}

int push_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo)
{
    tick_m(gb);
    write_mem(gb, --gb->cpu.sp, *hi);
    write_mem(gb, --gb->cpu.sp, *lo);
    return 4;
}
