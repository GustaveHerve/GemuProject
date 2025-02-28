#include <err.h>

#include "emulation.h"
#include "gb_core.h"
#include "read.h"
#include "utils.h"
#include "write.h"

// jr e (signed 8 bit)
// x18	3 MCycle
int jr_e8(struct gb_core *gb)
{
    int8_t e = read_mem_tick(gb, gb->cpu.pc++);
    tick_m(gb);
    gb->cpu.pc = gb->cpu.pc + e;
    return 3;
}

// jr cc e (signed 8 bit)
int jr_cc_e8(struct gb_core *gb, int cc)
{
    int8_t e = read_mem_tick(gb, gb->cpu.pc++);
    if (cc)
    {
        tick_m(gb);
        gb->cpu.pc += e;
        return 3;
    }
    return 2;
}

// ret
// xC9   4 MCycles
int ret(struct gb_core *gb)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.sp++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.sp++);
    tick_m(gb);
    gb->cpu.pc = convert_8to16(&hi, &lo);
    return 4;
}

// ret cc
//
int ret_cc(struct gb_core *gb, int cc)
{
    tick_m(gb);
    if (cc)
    {
        uint8_t lo = read_mem_tick(gb, gb->cpu.sp++);
        uint8_t hi = read_mem_tick(gb, gb->cpu.sp++);
        tick_m(gb);
        gb->cpu.pc = convert_8to16(&hi, &lo);
        return 5;
    }
    return 2;
}

// reti
// xD9   4 MCycle
int reti(struct gb_core *gb)
{
    ret(gb);
    gb->cpu.ime = 1;
    return 4;
}

// jp HL
// 0xE9 1 MCycle
int jp_hl(struct gb_core *gb)
{
    uint16_t address = convert_8to16(&gb->cpu.h, &gb->cpu.l);
    gb->cpu.pc = address;
    return 1;
}

int jp_nn(struct gb_core *gb)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.pc);
    uint16_t address = convert_8to16(&hi, &lo);
    tick_m(gb);
    gb->cpu.pc = address;
    return 4;
}

int jp_cc_nn(struct gb_core *gb, int cc)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.pc++);
    uint16_t address = convert_8to16(&hi, &lo);
    if (cc)
    {
        tick_m(gb);
        gb->cpu.pc = address;
        return 4;
    }
    return 3;
}

int call_nn(struct gb_core *gb)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.pc++);
    uint16_t nn = convert_8to16(&hi, &lo);
    tick_m(gb);
    write_mem(gb, --gb->cpu.sp, regist_hi(&gb->cpu.pc));
    write_mem(gb, --gb->cpu.sp, regist_lo(&gb->cpu.pc));
    gb->cpu.pc = nn;
    return 6;
}

int call_cc_nn(struct gb_core *gb, int cc)
{
    uint8_t lo = read_mem_tick(gb, gb->cpu.pc++);
    uint8_t hi = read_mem_tick(gb, gb->cpu.pc++);
    uint16_t nn = convert_8to16(&hi, &lo);
    if (cc)
    {
        tick_m(gb);
        write_mem(gb, --gb->cpu.sp, regist_hi(&gb->cpu.pc));
        write_mem(gb, --gb->cpu.sp, regist_lo(&gb->cpu.pc));
        gb->cpu.pc = nn;
        return 6;
    }
    return 3;
}

int rst(struct gb_core *gb, uint8_t vec)
{
    tick_m(gb);
    write_mem(gb, --gb->cpu.sp, regist_hi(&gb->cpu.pc));
    write_mem(gb, --gb->cpu.sp, regist_lo(&gb->cpu.pc));
    gb->cpu.pc = vec;
    return 4;
}
