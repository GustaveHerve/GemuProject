#include <err.h>
#include <stdint.h>

#include "emulation.h"
#include "gb_core.h"
#include "read.h"
#include "utils.h"
#include "write.h"

// rlc
// x0(0-7)   2 MCycle
int rlc(struct gb_core *gb, uint8_t *dest)
{
    rotl(dest);
    set_z(&gb->cpu, *dest == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    cflag_rotl_set(&gb->cpu, *dest);
    return 2;
}

// rlc (HL)
// x06   4 MCycle
int rlc_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    rotl(&val);
    write_mem(gb, address, val);
    set_z(&gb->cpu, val == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    cflag_rotl_set(&gb->cpu, val);
    return 4;
}

// rrc
// x0(8-F)   2 MCycle
int rrc(struct gb_core *gb, uint8_t *dest)
{
    rotr(dest);
    set_z(&gb->cpu, *dest == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    cflag_rotr_set(&gb->cpu, *dest);
    return 2;
}

// rrc (HL)
// x0E   4 MCycle
int rrc_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    rotr(&val);
    write_mem(gb, address, val);
    set_z(&gb->cpu, val == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    cflag_rotr_set(&gb->cpu, val);
    return 4;
}

// rl
// x1(0-7)   2 MCycle
int rl(struct gb_core *gb, uint8_t *dest)
{
    rotl_carry(&gb->cpu, dest);
    set_z(&gb->cpu, *dest == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 2;
}

// rl (HL)
// x16   4 MCycle
int rl_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    rotl_carry(&gb->cpu, &val);
    write_mem(gb, address, val);
    set_z(&gb->cpu, val == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 4;
}

// rr
// x1(8-F)   2 MCycle
int rr(struct gb_core *gb, uint8_t *dest)
{
    rotr_carry(&gb->cpu, dest);
    set_z(&gb->cpu, *dest == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 2;
}

// rr (HL)
// x1E   4 MCycle
int rr_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    rotr_carry(&gb->cpu, &val);
    write_mem(gb, address, val);
    set_z(&gb->cpu, val == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 4;
}

// sla
// x2(0-7)   2 MCycle
int sla(struct gb_core *gb, uint8_t *dest)
{
    set_c(&gb->cpu, (*dest & 0x80) == 0x80);
    *dest = *dest << 1;
    set_z(&gb->cpu, *dest == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 2;
}

// sla (HL)
// x26   4 MCycle
int sla_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    set_c(&gb->cpu, (val & 0x80) == 0x80);
    val = val << 1;
    write_mem(gb, address, val);
    set_z(&gb->cpu, val == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 4;
}

// sra
// x2(8-F)   2 MCycle
int sra(struct gb_core *gb, uint8_t *dest)
{
    set_c(&gb->cpu, *dest & 0x01);
    uint8_t temp = 0x80 & *dest;
    *dest = *dest >> 1;
    *dest = *dest | temp;
    set_z(&gb->cpu, *dest == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 2;
}

// sra (HL)
// x2E   4 MCycle
int sra_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    uint8_t temp = 0x80 & val;
    set_c(&gb->cpu, val & 0x01);
    val = val >> 1;
    val = val | temp;
    write_mem(gb, address, val);
    set_z(&gb->cpu, val == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 4;
}

// swap
// 0x3(0-7)  2 MCycle
int swap(struct gb_core *gb, uint8_t *dest)
{
    *dest = *dest >> 4 | *dest << 4;
    set_z(&gb->cpu, *dest == 0);
    set_n(&gb->cpu, 0);
    set_c(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 2;
}

// swap (HL)
// 0x36  4 MCycle
int swap_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    val = val >> 4 | val << 4;
    write_mem(gb, address, val);
    set_z(&gb->cpu, val == 0);
    set_n(&gb->cpu, 0);
    set_c(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 4;
}

// srl
// x3(8-F)   2 MCycle
int srl(struct gb_core *gb, uint8_t *dest)
{
    set_c(&gb->cpu, *dest & 0x01);
    *dest = *dest >> 1;
    set_z(&gb->cpu, *dest == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 2;
}

// srl (HL)
// x3E   4 MCycle
int srl_hl(struct gb_core *gb)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    set_c(&gb->cpu, (val & 0x01) == 0x01);
    val = val >> 1;
    write_mem(gb, address, val);
    set_z(&gb->cpu, val == 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 4;
}

// bit
// x
int bit(struct gb_core *gb, uint8_t *dest, int n)
{
    uint8_t bit = (*dest >> n) & 0x01;
    set_z(&gb->cpu, bit == 0x00);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 1);
    return 2;
}

// bit (HL)
// x
int bit_hl(struct gb_core *gb, int n)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    uint8_t bit = (val >> n) & 0x01;
    set_z(&gb->cpu, bit == 0x00);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 1);
    return 3;
}

// res
// x
int res(uint8_t *dest, int n)
{

    *dest &= ~(0x01 << n);
    return 2;
}

// res (HL)
// x
int res_hl(struct gb_core *gb, int n)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    val &= ~(0x01 << n);
    write_mem(gb, address, val);
    return 4;
}

// set
// x
int set(uint8_t *dest, int n)
{
    *dest |= (0x01 << n);
    return 2;
}

// set (HL)
// x
int set_hl(struct gb_core *gb, int n)
{
    uint16_t address = unpack16(gb->cpu.h, gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    val |= (0x01 << n);
    write_mem(gb, address, val);
    return 4;
}
