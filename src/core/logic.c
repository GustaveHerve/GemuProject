#include <err.h>

#include "cpu.h"
#include "emulation.h"
#include "memory.h"
#include "utils.h"

// inc r (8 bit)
// x(0-3)(4 or C)	1 MCycle
int inc_r(struct cpu *cpu, uint8_t *dest)
{
    set_n(cpu->regist, 0);
    hflag_add_set(cpu->regist, *dest, 1);
    *dest += 1;
    set_z(cpu->regist, *dest == 0);
    return 1;
}

// inc (HL) (8 bit)
// x34	3 MCycle
int inc_hl(struct cpu *cpu)
{
    set_n(cpu->regist, 0);
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t value = read_mem_tick(cpu, address);
    hflag_add_set(cpu->regist, value, 1);
    ++value;
    write_mem(cpu, address, value);
    set_z(cpu->regist, value == 0);
    return 3;
}

// inc rr
// x(0-3)3	2 MCycle
int inc_rr(struct cpu *cpu, uint8_t *hi, uint8_t *lo)
{
    // During fetch of the opcode probably writes to lo
    uint16_t convert = convert_8to16(hi, lo);
    ++convert;
    tick_m(cpu);
    *lo = regist_lo(&convert);
    *hi = regist_hi(&convert);
    return 2;
}

// inc SP
// x33	2 MCycle
int inc_sp(struct cpu *cpu)
{
    // During fetch of the opcode probably writes to lo
    ++cpu->regist->sp;
    tick_m(cpu);
    return 2;
}

// dec r (8 bit)
// x(0-3)(5 or D)	1 MCycle
int dec_r(struct cpu *cpu, uint8_t *dest)
{
    set_n(cpu->regist, 1);
    hflag_sub_set(cpu->regist, *dest, 1);
    *dest -= 1;
    set_z(cpu->regist, *dest == 0);
    return 1;
}

// dec (HL) (8 bit)
// x35	3 MCycle
int dec_hl(struct cpu *cpu)
{
    set_n(cpu->regist, 1);
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t value = read_mem_tick(cpu, address);
    hflag_sub_set(cpu->regist, value, 1);
    --value;
    write_mem(cpu, address, value);
    set_z(cpu->regist, value == 0);
    return 3;
}

// dec rr
// x(0-2)B	2MCycle
int dec_rr(struct cpu *cpu, uint8_t *hi, uint8_t *lo)
{
    // During fetch of the opcode probably writes to lo
    uint16_t temp = convert_8to16(hi, lo);
    --temp;
    tick_m(cpu);
    *lo = regist_lo(&temp);
    *hi = regist_hi(&temp);
    return 2;
}

// dec sp
// x3B	2 MCycle
int dec_sp(struct cpu *cpu)
{
    // During fetch of the opcode probably writes to lo
    --cpu->regist->sp;
    tick_m(cpu);
    return 2;
}

// add A,r
// x8(0-7)   1 MCycle
int add_a_r(struct cpu *cpu, uint8_t *src)
{
    set_n(cpu->regist, 0);
    hflag_add_set(cpu->regist, cpu->regist->a, *src);
    cflag_add_set(cpu->regist, cpu->regist->a, *src);
    cpu->regist->a += *src;
    set_z(cpu->regist, cpu->regist->a == 0);
    return 1;
}

// add A,(HL)
// x86   2 MCycle
int add_a_hl(struct cpu *cpu)
{
    set_n(cpu->regist, 0);
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    hflag_add_set(cpu->regist, cpu->regist->a, val);
    cflag_add_set(cpu->regist, cpu->regist->a, val);
    cpu->regist->a += val;
    set_z(cpu->regist, cpu->regist->a == 0);
    return 2;
}

// add A,n
// xC6   2 MCycle
int add_a_n(struct cpu *cpu)
{
    uint8_t n = read_mem_tick(cpu, cpu->regist->pc);
    set_n(cpu->regist, 0);
    hflag_add_set(cpu->regist, cpu->regist->a, n);
    cflag_add_set(cpu->regist, cpu->regist->a, n);
    cpu->regist->a += n;
    set_z(cpu->regist, cpu->regist->a == 0);
    ++cpu->regist->pc;
    return 2;
}

// adc A,r
// x     1 MCycle
int adc_a_r(struct cpu *cpu, uint8_t *src)
{
    uint8_t a_copy = cpu->regist->a;
    set_n(cpu->regist, 0);
    uint8_t val = *src;
    int temp = cpu->regist->a + val;
    cpu->regist->a += val;
    if (get_c(cpu->regist))
    {
        ++temp;
        ++cpu->regist->a;
        set_h(cpu->regist, (((a_copy & 0xF) + (val & 0xF) + (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(cpu->regist, (((a_copy & 0xF) + (val & 0xF)) & 0x10) == 0x10);
    set_c(cpu->regist, temp > 0xFF);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 1;
}

// adc A,(HL)
// x8E   2 MCycle
int adc_a_hl(struct cpu *cpu)
{
    uint8_t a_copy = cpu->regist->a;
    set_n(cpu->regist, 0);
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    int temp = cpu->regist->a + val;
    cpu->regist->a += val;
    if (get_c(cpu->regist))
    {
        ++temp;
        ++cpu->regist->a;
        set_h(cpu->regist, (((a_copy & 0xF) + (val & 0xF) + (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(cpu->regist, (((a_copy & 0xF) + (val & 0xF)) & 0x10) == 0x10);
    set_c(cpu->regist, temp > 0xFF);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 2;
}

// adc A,n
// xCE   2 MCycle
int adc_a_n(struct cpu *cpu)
{
    uint8_t a_copy = cpu->regist->a;
    uint8_t val = read_mem_tick(cpu, cpu->regist->pc);
    set_n(cpu->regist, 0);
    int temp = cpu->regist->a + val;
    cpu->regist->a += val;
    if (get_c(cpu->regist))
    {
        ++temp;
        ++cpu->regist->a;
        set_h(cpu->regist, (((a_copy & 0xF) + (val & 0xF) + (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(cpu->regist, (((a_copy & 0xF) + (val & 0xF)) & 0x10) == 0x10);
    set_c(cpu->regist, temp > 0xFF);
    set_z(cpu->regist, cpu->regist->a == 0);
    ++cpu->regist->pc;
    return 2;
}

// add HL,rr
// x(0-2)9	2 MCycle
int add_hl_rr(struct cpu *cpu, uint8_t *hi, uint8_t *lo)
{
    // During fetch of the opcode probably writes to lo
    set_n(cpu->regist, 0);
    uint16_t hl = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint16_t rr = convert_8to16(hi, lo);
    hflag16_add_set(cpu->regist, hl, rr);
    cflag16_add_set(cpu->regist, hl, rr);
    uint16_t sum = hl + rr;
    cpu->regist->h = regist_hi(&sum);
    cpu->regist->l = regist_lo(&sum);
    tick_m(cpu);
    return 2;
}

// add HL,SP
// x39	2 MCycle
int add_hl_sp(struct cpu *cpu)
{
    // During fetch of the opcode probably writes to lo
    set_n(cpu->regist, 0);
    uint16_t hl = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    hflag16_add_set(cpu->regist, hl, cpu->regist->sp);
    cflag16_add_set(cpu->regist, hl, cpu->regist->sp);
    uint16_t sum = hl + cpu->regist->sp;
    cpu->regist->h = regist_hi(&sum);
    cpu->regist->l = regist_lo(&sum);
    tick_m(cpu);
    return 2;
}

int add_sp_e8(struct cpu *cpu)
{
    int8_t offset = read_mem_tick(cpu, cpu->regist->pc);
    uint8_t lo = regist_lo(&cpu->regist->sp);
    tick_m(cpu);
    cflag_add_set(cpu->regist, lo, offset);
    hflag_add_set(cpu->regist, lo, offset);
    set_z(cpu->regist, 0);
    set_n(cpu->regist, 0);
    cpu->regist->sp += offset;
    tick_m(cpu);
    ++cpu->regist->pc;
    return 4;
}
// sub A,r
// x(0-7)9   1 MCycle
int sub_a_r(struct cpu *cpu, uint8_t *src)
{
    set_n(cpu->regist, 1);
    cflag_sub_set(cpu->regist, cpu->regist->a, *src);
    hflag_sub_set(cpu->regist, cpu->regist->a, *src);
    cpu->regist->a -= *src;
    set_z(cpu->regist, cpu->regist->a == 0);
    return 1;
}

// sub A,(HL)
// x69   2 MCycle
int sub_a_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    set_n(cpu->regist, 1);
    uint8_t val = read_mem_tick(cpu, address);
    cflag_sub_set(cpu->regist, cpu->regist->a, val);
    hflag_sub_set(cpu->regist, cpu->regist->a, val);
    cpu->regist->a -= val;
    set_z(cpu->regist, cpu->regist->a == 0);
    return 2;
}

int sub_a_n(struct cpu *cpu)
{
    uint8_t n = read_mem_tick(cpu, cpu->regist->pc);
    set_n(cpu->regist, 1);
    cflag_sub_set(cpu->regist, cpu->regist->a, n);
    hflag_sub_set(cpu->regist, cpu->regist->a, n);
    cpu->regist->a -= n;
    set_z(cpu->regist, cpu->regist->a == 0);
    cpu->regist->pc++;
    return 2;
}

// sbc A,r
// x9(8-F)   1 MCycle
int sbc_a_r(struct cpu *cpu, uint8_t *src)
{
    uint8_t a_copy = cpu->regist->a;
    set_n(cpu->regist, 1);
    cpu->regist->a -= *src;
    if (get_c(cpu->regist))
    {
        --cpu->regist->a;
        set_h(cpu->regist, (((a_copy & 0xF) - (*src & 0xF) - (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(cpu->regist, (((a_copy & 0xF) - (*src & 0xF)) & 0x10) == 0x10);
    set_c(cpu->regist, *src + get_c(cpu->regist) > a_copy);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 1;
}

// sbc A,(HL)
// x9E   2 MCycle
int sbc_a_hl(struct cpu *cpu)
{
    uint8_t a_copy = cpu->regist->a;
    set_n(cpu->regist, 1);
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    uint8_t val = read_mem_tick(cpu, address);
    cpu->regist->a -= val;
    if (get_c(cpu->regist))
    {
        --cpu->regist->a;
        set_h(cpu->regist, (((a_copy & 0xF) - (val & 0xF) - (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(cpu->regist, (((a_copy & 0xF) - (val & 0xF)) & 0x10) == 0x10);
    set_c(cpu->regist, val + get_c(cpu->regist) > a_copy);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 2;
}

// sbc A,n
// xDE   2 MCycle
int sbc_a_n(struct cpu *cpu)
{
    uint8_t a_copy = cpu->regist->a;
    set_n(cpu->regist, 1);
    uint8_t val = read_mem_tick(cpu, cpu->regist->pc);
    cpu->regist->a -= val;
    if (get_c(cpu->regist))
    {
        --cpu->regist->a;
        set_h(cpu->regist, (((a_copy & 0xF) - (val & 0xF) - (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(cpu->regist, (((a_copy & 0xF) - (val & 0xF)) & 0x10) == 0x10);
    set_c(cpu->regist, val + get_c(cpu->regist) > a_copy);
    set_z(cpu->regist, cpu->regist->a == 0);
    ++cpu->regist->pc;
    return 2;
}

// and a,r
// xA(0-7)   1 MCycle
int and_a_r(struct cpu *cpu, uint8_t *src)
{
    cpu->regist->a = cpu->regist->a & *src;
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 1);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 1;
}

// and a,(HL)
// x6A   2 MCycle
int and_a_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    cpu->regist->a = cpu->regist->a & read_mem_tick(cpu, address);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 1);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 2;
}

// and A,n
// xE6   2 MCycle
int and_a_n(struct cpu *cpu)
{
    uint8_t n = read_mem_tick(cpu, cpu->regist->pc);
    cpu->regist->a = cpu->regist->a & n;
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 1);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    ++cpu->regist->pc;
    return 2;
}

// xor A,r
// xA(8-F)   1 MCycle
int xor_a_r(struct cpu *cpu, uint8_t *src)
{
    cpu->regist->a = cpu->regist->a ^ *src;
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 1;
}

// xor A,(HL)
// xAE   2 MCycle
int xor_a_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    cpu->regist->a = cpu->regist->a ^ read_mem_tick(cpu, address);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 2;
}

// xor A,n
// xEE   2 MCycle
int xor_a_n(struct cpu *cpu)
{
    uint8_t n = read_mem_tick(cpu, cpu->regist->pc);
    cpu->regist->a = cpu->regist->a ^ n;
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    ++cpu->regist->pc;
    return 2;
}

// or A,r
// xB(0-7)   1 MCycle
int or_a_r(struct cpu *cpu, uint8_t *src)
{
    cpu->regist->a = cpu->regist->a | *src;
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 1;
}

// or A,(HL)
// xB6   2 MCycle
int or_a_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    cpu->regist->a = cpu->regist->a | read_mem_tick(cpu, address);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    return 2;
}

// or A,n
// xF6   2 MCycle
int or_a_n(struct cpu *cpu)
{
    uint8_t n = read_mem_tick(cpu, cpu->regist->pc);
    cpu->regist->a = cpu->regist->a | n;
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    set_c(cpu->regist, 0);
    set_z(cpu->regist, cpu->regist->a == 0);
    ++cpu->regist->pc;
    return 2;
}

// cp A,r
// xB(8-F)   1 MCycle
int cp_a_r(struct cpu *cpu, uint8_t *src)
{
    set_n(cpu->regist, 1);
    cflag_sub_set(cpu->regist, cpu->regist->a, *src);
    hflag_sub_set(cpu->regist, cpu->regist->a, *src);
    set_z(cpu->regist, cpu->regist->a == *src);
    return 1;
}

// cp A,(HL)
// xBE   2 MCycle
int cp_a_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    set_n(cpu->regist, 1);
    uint8_t val = read_mem_tick(cpu, address);
    cflag_sub_set(cpu->regist, cpu->regist->a, val);
    hflag_sub_set(cpu->regist, cpu->regist->a, val);
    set_z(cpu->regist, cpu->regist->a == val);
    return 2;
}

// cp A,n
// xFE   2 MCycle
int cp_a_n(struct cpu *cpu)
{
    uint8_t n = read_mem_tick(cpu, cpu->regist->pc);
    ++cpu->regist->pc;
    set_n(cpu->regist, 1);
    cflag_sub_set(cpu->regist, cpu->regist->a, n);
    hflag_sub_set(cpu->regist, cpu->regist->a, n);
    set_z(cpu->regist, cpu->regist->a == n);
    return 2;
}

// daa A
// x27	1 MCycle
int daa(struct cpu *cpu)
{
    if (!get_n(cpu->regist)) // Addition case
    {
        if (get_c(cpu->regist) || cpu->regist->a > 0x99) // check high nibble
        {
            cpu->regist->a += 0x60;
            set_c(cpu->regist, 1);
        }

        if (get_h(cpu->regist) || (cpu->regist->a & 0x0F) > 0x09) // check low nibble
        {
            cpu->regist->a += 0x6;
        }
    }

    else // Subtraction case
    {
        if (get_c(cpu->regist))
            cpu->regist->a -= 0x60;
        if (get_h(cpu->regist))
            cpu->regist->a -= 0x6;
    }
    set_z(cpu->regist, cpu->regist->a == 0x00);
    set_h(cpu->regist, 0);

    return 1;
}

// cpl
// x2F	1 MCycle
int cpl(struct cpu *cpu)
{
    cpu->regist->a = ~cpu->regist->a;
    set_n(cpu->regist, 1);
    set_h(cpu->regist, 1);
    return 1;
}
