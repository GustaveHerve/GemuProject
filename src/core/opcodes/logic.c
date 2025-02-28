#include <err.h>

#include "emulation.h"
#include "gb_core.h"
#include "read.h"
#include "utils.h"
#include "write.h"

// inc r (8 bit)
// x(0-3)(4 or C)	1 MCycle
int inc_r(struct gb_core *gb, uint8_t *dest)
{
    set_n(&gb->cpu, 0);
    hflag_add_set(&gb->cpu, *dest, 1);
    *dest += 1;
    set_z(&gb->cpu, *dest == 0);
    return 1;
}

// inc (HL) (8 bit)
// x34	3 MCycle
int inc_hl(struct gb_core *gb)
{
    set_n(&gb->cpu, 0);
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    uint8_t value = read_mem_tick(gb, address);
    hflag_add_set(&gb->cpu, value, 1);
    ++value;
    write_mem(gb, address, value);
    set_z(&gb->cpu, value == 0);
    return 3;
}

// inc rr
// x(0-3)3	2 MCycle
int inc_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo)
{
    // During fetch of the opcode probably writes to lo
    uint16_t convert = convert_8to16(hi, lo);
    ++convert;
    tick_m(gb);
    *lo = regist_lo(&convert);
    *hi = regist_hi(&convert);
    return 2;
}

// inc SP
// x33	2 MCycle
int inc_sp(struct gb_core *gb)
{
    // During fetch of the opcode probably writes to lo
    ++gb->cpu.sp;
    tick_m(gb);
    return 2;
}

// dec r (8 bit)
// x(0-3)(5 or D)	1 MCycle
int dec_r(struct gb_core *gb, uint8_t *dest)
{
    set_n(&gb->cpu, 1);
    hflag_sub_set(&gb->cpu, *dest, 1);
    *dest -= 1;
    set_z(&gb->cpu, *dest == 0);
    return 1;
}

// dec (HL) (8 bit)
// x35	3 MCycle
int dec_hl(struct gb_core *gb)
{
    set_n(&gb->cpu, 1);
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    uint8_t value = read_mem_tick(gb, address);
    hflag_sub_set(&gb->cpu, value, 1);
    --value;
    write_mem(gb, address, value);
    set_z(&gb->cpu, value == 0);
    return 3;
}

// dec rr
// x(0-2)B	2MCycle
int dec_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo)
{
    // During fetch of the opcode probably writes to lo
    uint16_t temp = convert_8to16(hi, lo);
    --temp;
    tick_m(gb);
    *lo = regist_lo(&temp);
    *hi = regist_hi(&temp);
    return 2;
}

// dec sp
// x3B	2 MCycle
int dec_sp(struct gb_core *gb)
{
    // During fetch of the opcode probably writes to lo
    --gb->cpu.sp;
    tick_m(gb);
    return 2;
}

// add A,r
// x8(0-7)   1 MCycle
int add_a_r(struct gb_core *gb, uint8_t *src)
{
    set_n(&gb->cpu, 0);
    hflag_add_set(&gb->cpu, gb->cpu.a, *src);
    cflag_add_set(&gb->cpu, gb->cpu.a, *src);
    gb->cpu.a += *src;
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 1;
}

// add A,(HL)
// x86   2 MCycle
int add_a_hl(struct gb_core *gb)
{
    set_n(&gb->cpu, 0);
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    hflag_add_set(&gb->cpu, gb->cpu.a, val);
    cflag_add_set(&gb->cpu, gb->cpu.a, val);
    gb->cpu.a += val;
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 2;
}

// add A,n
// xC6   2 MCycle
int add_a_n(struct gb_core *gb)
{
    uint8_t n = read_mem_tick(gb, gb->cpu.pc);
    set_n(&gb->cpu, 0);
    hflag_add_set(&gb->cpu, gb->cpu.a, n);
    cflag_add_set(&gb->cpu, gb->cpu.a, n);
    gb->cpu.a += n;
    set_z(&gb->cpu, gb->cpu.a == 0);
    ++gb->cpu.pc;
    return 2;
}

// adc A,r
// x     1 MCycle
int adc_a_r(struct gb_core *gb, uint8_t *src)
{
    uint8_t a_copy = gb->cpu.a;
    set_n(&gb->cpu, 0);
    uint8_t val = *src;
    int temp = gb->cpu.a + val;
    gb->cpu.a += val;
    if (get_c(&gb->cpu))
    {
        ++temp;
        ++gb->cpu.a;
        set_h(&gb->cpu, (((a_copy & 0xF) + (val & 0xF) + (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(&gb->cpu, (((a_copy & 0xF) + (val & 0xF)) & 0x10) == 0x10);
    set_c(&gb->cpu, temp > 0xFF);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 1;
}

// adc A,(HL)
// x8E   2 MCycle
int adc_a_hl(struct gb_core *gb)
{
    uint8_t a_copy = gb->cpu.a;
    set_n(&gb->cpu, 0);
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    int temp = gb->cpu.a + val;
    gb->cpu.a += val;
    if (get_c(&gb->cpu))
    {
        ++temp;
        ++gb->cpu.a;
        set_h(&gb->cpu, (((a_copy & 0xF) + (val & 0xF) + (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(&gb->cpu, (((a_copy & 0xF) + (val & 0xF)) & 0x10) == 0x10);
    set_c(&gb->cpu, temp > 0xFF);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 2;
}

// adc A,n
// xCE   2 MCycle
int adc_a_n(struct gb_core *gb)
{
    uint8_t a_copy = gb->cpu.a;
    uint8_t val = read_mem_tick(gb, gb->cpu.pc);
    set_n(&gb->cpu, 0);
    int temp = gb->cpu.a + val;
    gb->cpu.a += val;
    if (get_c(&gb->cpu))
    {
        ++temp;
        ++gb->cpu.a;
        set_h(&gb->cpu, (((a_copy & 0xF) + (val & 0xF) + (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(&gb->cpu, (((a_copy & 0xF) + (val & 0xF)) & 0x10) == 0x10);
    set_c(&gb->cpu, temp > 0xFF);
    set_z(&gb->cpu, gb->cpu.a == 0);
    ++gb->cpu.pc;
    return 2;
}

// add HL,rr
// x(0-2)9	2 MCycle
int add_hl_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo)
{
    // During fetch of the opcode probably writes to lo
    set_n(&gb->cpu, 0);
    uint16_t hl = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    uint16_t rr = convert_8to16(hi, lo);
    hflag16_add_set(&gb->cpu, hl, rr);
    cflag16_add_set(&gb->cpu, hl, rr);
    uint16_t sum = hl + rr;
    gb->cpu.h = regist_hi(&sum);
    gb->cpu.l = regist_lo(&sum);
    tick_m(gb);
    return 2;
}

// add HL,SP
// x39	2 MCycle
int add_hl_sp(struct gb_core *gb)
{
    // During fetch of the opcode probably writes to lo
    set_n(&gb->cpu, 0);
    uint16_t hl = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    hflag16_add_set(&gb->cpu, hl, gb->cpu.sp);
    cflag16_add_set(&gb->cpu, hl, gb->cpu.sp);
    uint16_t sum = hl + gb->cpu.sp;
    gb->cpu.h = regist_hi(&sum);
    gb->cpu.l = regist_lo(&sum);
    tick_m(gb);
    return 2;
}

int add_sp_e8(struct gb_core *gb)
{
    int8_t offset = read_mem_tick(gb, gb->cpu.pc);
    uint8_t lo = regist_lo(&gb->cpu.sp);
    tick_m(gb);
    cflag_add_set(&gb->cpu, lo, offset);
    hflag_add_set(&gb->cpu, lo, offset);
    set_z(&gb->cpu, 0);
    set_n(&gb->cpu, 0);
    gb->cpu.sp += offset;
    tick_m(gb);
    ++gb->cpu.pc;
    return 4;
}
// sub A,r
// x(0-7)9   1 MCycle
int sub_a_r(struct gb_core *gb, uint8_t *src)
{
    set_n(&gb->cpu, 1);
    cflag_sub_set(&gb->cpu, gb->cpu.a, *src);
    hflag_sub_set(&gb->cpu, gb->cpu.a, *src);
    gb->cpu.a -= *src;
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 1;
}

// sub A,(HL)
// x69   2 MCycle
int sub_a_hl(struct gb_core *gb)
{
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    set_n(&gb->cpu, 1);
    uint8_t val = read_mem_tick(gb, address);
    cflag_sub_set(&gb->cpu, gb->cpu.a, val);
    hflag_sub_set(&gb->cpu, gb->cpu.a, val);
    gb->cpu.a -= val;
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 2;
}

int sub_a_n(struct gb_core *gb)
{
    uint8_t n = read_mem_tick(gb, gb->cpu.pc);
    set_n(&gb->cpu, 1);
    cflag_sub_set(&gb->cpu, gb->cpu.a, n);
    hflag_sub_set(&gb->cpu, gb->cpu.a, n);
    gb->cpu.a -= n;
    set_z(&gb->cpu, gb->cpu.a == 0);
    gb->cpu.pc++;
    return 2;
}

// sbc A,r
// x9(8-F)   1 MCycle
int sbc_a_r(struct gb_core *gb, uint8_t *src)
{
    uint8_t a_copy = gb->cpu.a;
    set_n(&gb->cpu, 1);
    gb->cpu.a -= *src;
    if (get_c(&gb->cpu))
    {
        --gb->cpu.a;
        set_h(&gb->cpu, (((a_copy & 0xF) - (*src & 0xF) - (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(&gb->cpu, (((a_copy & 0xF) - (*src & 0xF)) & 0x10) == 0x10);
    set_c(&gb->cpu, *src + get_c(&gb->cpu) > a_copy);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 1;
}

// sbc A,(HL)
// x9E   2 MCycle
int sbc_a_hl(struct gb_core *gb)
{
    uint8_t a_copy = gb->cpu.a;
    set_n(&gb->cpu, 1);
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    uint8_t val = read_mem_tick(gb, address);
    gb->cpu.a -= val;
    if (get_c(&gb->cpu))
    {
        --gb->cpu.a;
        set_h(&gb->cpu, (((a_copy & 0xF) - (val & 0xF) - (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(&gb->cpu, (((a_copy & 0xF) - (val & 0xF)) & 0x10) == 0x10);
    set_c(&gb->cpu, val + get_c(&gb->cpu) > a_copy);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 2;
}

// sbc A,n
// xDE   2 MCycle
int sbc_a_n(struct gb_core *gb)
{
    uint8_t a_copy = gb->cpu.a;
    set_n(&gb->cpu, 1);
    uint8_t val = read_mem_tick(gb, gb->cpu.pc);
    gb->cpu.a -= val;
    if (get_c(&gb->cpu))
    {
        --gb->cpu.a;
        set_h(&gb->cpu, (((a_copy & 0xF) - (val & 0xF) - (1 & 0xF)) & 0x10) == 0x10);
    }
    else
        set_h(&gb->cpu, (((a_copy & 0xF) - (val & 0xF)) & 0x10) == 0x10);
    set_c(&gb->cpu, val + get_c(&gb->cpu) > a_copy);
    set_z(&gb->cpu, gb->cpu.a == 0);
    ++gb->cpu.pc;
    return 2;
}

// and a,r
// xA(0-7)   1 MCycle
int and_a_r(struct gb_core *gb, uint8_t *src)
{
    gb->cpu.a = gb->cpu.a & *src;
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 1);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 1;
}

// and a,(HL)
// x6A   2 MCycle
int and_a_hl(struct gb_core *gb)
{
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    gb->cpu.a = gb->cpu.a & read_mem_tick(gb, address);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 1);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 2;
}

// and A,n
// xE6   2 MCycle
int and_a_n(struct gb_core *gb)
{
    uint8_t n = read_mem_tick(gb, gb->cpu.pc);
    gb->cpu.a = gb->cpu.a & n;
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 1);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    ++gb->cpu.pc;
    return 2;
}

// xor A,r
// xA(8-F)   1 MCycle
int xor_a_r(struct gb_core *gb, uint8_t *src)
{
    gb->cpu.a = gb->cpu.a ^ *src;
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 1;
}

// xor A,(HL)
// xAE   2 MCycle
int xor_a_hl(struct gb_core *gb)
{
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    gb->cpu.a = gb->cpu.a ^ read_mem_tick(gb, address);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 2;
}

// xor A,n
// xEE   2 MCycle
int xor_a_n(struct gb_core *gb)
{
    uint8_t n = read_mem_tick(gb, gb->cpu.pc);
    gb->cpu.a = gb->cpu.a ^ n;
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    ++gb->cpu.pc;
    return 2;
}

// or A,r
// xB(0-7)   1 MCycle
int or_a_r(struct gb_core *gb, uint8_t *src)
{
    gb->cpu.a = gb->cpu.a | *src;
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 1;
}

// or A,(HL)
// xB6   2 MCycle
int or_a_hl(struct gb_core *gb)
{
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    gb->cpu.a = gb->cpu.a | read_mem_tick(gb, address);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    return 2;
}

// or A,n
// xF6   2 MCycle
int or_a_n(struct gb_core *gb)
{
    uint8_t n = read_mem_tick(gb, gb->cpu.pc);
    gb->cpu.a = gb->cpu.a | n;
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    set_c(&gb->cpu, 0);
    set_z(&gb->cpu, gb->cpu.a == 0);
    ++gb->cpu.pc;
    return 2;
}

// cp A,r
// xB(8-F)   1 MCycle
int cp_a_r(struct gb_core *gb, uint8_t *src)
{
    set_n(&gb->cpu, 1);
    cflag_sub_set(&gb->cpu, gb->cpu.a, *src);
    hflag_sub_set(&gb->cpu, gb->cpu.a, *src);
    set_z(&gb->cpu, gb->cpu.a == *src);
    return 1;
}

// cp A,(HL)
// xBE   2 MCycle
int cp_a_hl(struct gb_core *gb)
{
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    set_n(&gb->cpu, 1);
    uint8_t val = read_mem_tick(gb, address);
    cflag_sub_set(&gb->cpu, gb->cpu.a, val);
    hflag_sub_set(&gb->cpu, gb->cpu.a, val);
    set_z(&gb->cpu, gb->cpu.a == val);
    return 2;
}

// cp A,n
// xFE   2 MCycle
int cp_a_n(struct gb_core *gb)
{
    uint8_t n = read_mem_tick(gb, gb->cpu.pc);
    ++gb->cpu.pc;
    set_n(&gb->cpu, 1);
    cflag_sub_set(&gb->cpu, gb->cpu.a, n);
    hflag_sub_set(&gb->cpu, gb->cpu.a, n);
    set_z(&gb->cpu, gb->cpu.a == n);
    return 2;
}

// daa A
// x27	1 MCycle
int daa(struct gb_core *gb)
{
    if (!get_n(&gb->cpu)) // Addition case
    {
        if (get_c(&gb->cpu) || gb->cpu.a > 0x99) // check high nibble
        {
            gb->cpu.a += 0x60;
            set_c(&gb->cpu, 1);
        }

        if (get_h(&gb->cpu) || (gb->cpu.a & 0x0F) > 0x09) // check low nibble
        {
            gb->cpu.a += 0x6;
        }
    }

    else // Subtraction case
    {
        if (get_c(&gb->cpu))
            gb->cpu.a -= 0x60;
        if (get_h(&gb->cpu))
            gb->cpu.a -= 0x6;
    }
    set_z(&gb->cpu, gb->cpu.a == 0x00);
    set_h(&gb->cpu, 0);

    return 1;
}

// cpl
// x2F	1 MCycle
int cpl(struct gb_core *gb)
{
    gb->cpu.a = ~gb->cpu.a;
    set_n(&gb->cpu, 1);
    set_h(&gb->cpu, 1);
    return 1;
}
