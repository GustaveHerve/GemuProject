#include <err.h>
#include <stdint.h>

#include "cpu.h"
#include "emulation.h"
#include "memory.h"
#include "utils.h"

// rlc
// x0(0-7)   2 MCycle
int rlc(struct cpu *cpu, uint8_t *dest)
{
    rotl(dest);
    set_z(cpu->regist, *dest == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    cflag_rotl_set(cpu->regist, *dest);
    return 2;
}

// rlc (HL)
// x06   4 MCycle
int rlc_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    rotl(&val);
    write_mem(cpu, address, val);
    set_z(cpu->regist, val == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    cflag_rotl_set(cpu->regist, val);
    return 4;
}

// rrc
// x0(8-F)   2 MCycle
int rrc(struct cpu *cpu, uint8_t *dest)
{
    rotr(dest);
    set_z(cpu->regist, *dest == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    cflag_rotr_set(cpu->regist, *dest);
    return 2;
}

// rrc (HL)
// x0E   4 MCycle
int rrc_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    rotr(&val);
    write_mem(cpu, address, val);
    set_z(cpu->regist, val == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    cflag_rotr_set(cpu->regist, val);
    return 4;
}

// rl
// x1(0-7)   2 MCycle
int rl(struct cpu *cpu, uint8_t *dest)
{
    rotl_carry(cpu->regist, dest);
    set_z(cpu->regist, *dest == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 2;
}

// rl (HL)
// x16   4 MCycle
int rl_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    rotl_carry(cpu->regist, &val);
    write_mem(cpu, address, val);
    set_z(cpu->regist, val == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 4;
}

// rr
// x1(8-F)   2 MCycle
int rr(struct cpu *cpu, uint8_t *dest)
{
    rotr_carry(cpu->regist, dest);
    set_z(cpu->regist, *dest == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 2;
}

// rr (HL)
// x1E   4 MCycle
int rr_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    rotr_carry(cpu->regist, &val);
    write_mem(cpu, address, val);
    set_z(cpu->regist, val == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 4;
}

// sla
// x2(0-7)   2 MCycle
int sla(struct cpu *cpu, uint8_t *dest)
{
    set_c(cpu->regist, (*dest & 0x80) == 0x80);
    *dest = *dest << 1;
    set_z(cpu->regist, *dest == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 2;
}

// sla (HL)
// x26   4 MCycle
int sla_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    set_c(cpu->regist, (val & 0x80) == 0x80);
    val = val << 1;
    write_mem(cpu, address, val);
    set_z(cpu->regist, val == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 4;
}

// sra
// x2(8-F)   2 MCycle
int sra(struct cpu *cpu, uint8_t *dest)
{
    set_c(cpu->regist, *dest & 0x01);
    uint8_t temp = 0x80 & *dest;
    *dest = *dest >> 1;
    *dest = *dest | temp;
    set_z(cpu->regist, *dest == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 2;
}

// sra (HL)
// x2E   4 MCycle
int sra_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    uint8_t temp = 0x80 & val;
    set_c(cpu->regist, val & 0x01);
    val = val >> 1;
    val = val | temp;
    write_mem(cpu, address, val);
    set_z(cpu->regist, val == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 4;
}

// swap
// 0x3(0-7)  2 MCycle
int swap(struct cpu *cpu, uint8_t *dest)
{
    uint8_t val = get_msb_nibble(*dest) | (get_lsb_nibble(*dest) << 4);
    *dest = val;
    set_z(cpu->regist, *dest == 0);
    set_n(cpu->regist, 0);
    set_c(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 2;
}

// swap (HL)
// 0x36  4 MCycle
int swap_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    val = get_msb_nibble(val) | (get_lsb_nibble(val) << 4);
    write_mem(cpu, address, val);
    set_z(cpu->regist, val == 0);
    set_n(cpu->regist, 0);
    set_c(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 4;
}

// srl
// x3(8-F)   2 MCycle
int srl(struct cpu *cpu, uint8_t *dest)
{
    set_c(cpu->regist, *dest & 0x01);
    *dest = *dest >> 1;
    set_z(cpu->regist, *dest == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 2;
}

// srl (HL)
// x3E   4 MCycle
int srl_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    set_c(cpu->regist, (val & 0x01) == 0x01);
    val = val >> 1;
    write_mem(cpu, address, val);
    set_z(cpu->regist, val == 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 4;
}

// bit
// x
int bit(struct cpu *cpu, uint8_t *dest, int n)
{
    uint8_t bit = (*dest >> n) & 0x01;
    set_z(cpu->regist, bit == 0x00);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 1);
    return 2;
}

// bit (HL)
// x
int bit_hl(struct cpu *cpu, int n)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    uint8_t bit = (val >> n) & 0x01;
    set_z(cpu->regist, bit == 0x00);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 1);
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
int res_hl(struct cpu *cpu, int n)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    val &= ~(0x01 << n);
    write_mem(cpu, address, val);
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
int set_hl(struct cpu *cpu, int n)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    val |= (0x01 << n);
    write_mem(cpu, address, val);
    return 4;
}
