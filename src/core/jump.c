#include <err.h>

#include "cpu.h"
#include "emulation.h"
#include "memory.h"
#include "utils.h"

// jr e (signed 8 bit)
// x18	3 MCycle
int jr_e8(struct cpu *cpu)
{
    int8_t e = read_mem_tick(cpu, cpu->regist->pc);
    ++cpu->regist->pc;
    tick_m(cpu);
    cpu->regist->pc = cpu->regist->pc + e;
    return 3;
}

// jr cc e (signed 8 bit)
int jr_cc_e8(struct cpu *cpu, int cc)
{
    int8_t e = read_mem_tick(cpu, cpu->regist->pc);
    ++cpu->regist->pc;
    if (cc)
    {
        tick_m(cpu);
        cpu->regist->pc += e;
        return 3;
    }
    return 2;
}

// ret
// xC9   4 MCycles
int ret(struct cpu *cpu)
{
    uint8_t lo = read_mem_tick(cpu, cpu->regist->sp);
    ++cpu->regist->sp;
    uint8_t hi = read_mem_tick(cpu, cpu->regist->sp);
    ++cpu->regist->sp;
    tick_m(cpu);
    cpu->regist->pc = convert_8to16(&hi, &lo);
    return 4;
}

// ret cc
//
int ret_cc(struct cpu *cpu, int cc)
{
    tick_m(cpu);
    if (cc)
    {
        uint8_t lo = read_mem_tick(cpu, cpu->regist->sp);
        ++cpu->regist->sp;
        uint8_t hi = read_mem_tick(cpu, cpu->regist->sp);
        ++cpu->regist->sp;
        tick_m(cpu);
        cpu->regist->pc = convert_8to16(&hi, &lo);
        return 5;
    }
    return 2;
}

// reti
// xD9   4 MCycle
int reti(struct cpu *cpu)
{
    ret(cpu);
    cpu->ime = 1;
    return 4;
}

// jp HL
// 0xE9 1 MCycle
int jp_hl(struct cpu *cpu)
{
    uint16_t address = convert_8to16(&cpu->regist->h, &cpu->regist->l);
    cpu->regist->pc = address;
    return 1;
}

int jp_nn(struct cpu *cpu)
{
    uint8_t lo = read_mem_tick(cpu, cpu->regist->pc);
    ++cpu->regist->pc;
    uint8_t hi = read_mem_tick(cpu, cpu->regist->pc);
    uint16_t address = convert_8to16(&hi, &lo);
    tick_m(cpu);
    cpu->regist->pc = address;
    return 4;
}

int jp_cc_nn(struct cpu *cpu, int cc)
{
    uint8_t lo = read_mem_tick(cpu, cpu->regist->pc);
    ++cpu->regist->pc;
    uint8_t hi = read_mem_tick(cpu, cpu->regist->pc);
    ++cpu->regist->pc;
    uint16_t address = convert_8to16(&hi, &lo);
    if (cc)
    {
        tick_m(cpu);
        cpu->regist->pc = address;
        return 4;
    }
    return 3;
}

int call_nn(struct cpu *cpu)
{
    uint8_t lo = read_mem_tick(cpu, cpu->regist->pc);
    ++cpu->regist->pc;
    uint8_t hi = read_mem_tick(cpu, cpu->regist->pc);
    uint16_t nn = convert_8to16(&hi, &lo);
    ++cpu->regist->pc;
    tick_m(cpu);
    --cpu->regist->sp;
    write_mem(cpu, cpu->regist->sp, regist_hi(&cpu->regist->pc));
    --cpu->regist->sp;
    write_mem(cpu, cpu->regist->sp, regist_lo(&cpu->regist->pc));
    cpu->regist->pc = nn;
    return 6;
}

int call_cc_nn(struct cpu *cpu, int cc)
{
    uint8_t lo = read_mem_tick(cpu, cpu->regist->pc);
    ++cpu->regist->pc;
    uint8_t hi = read_mem_tick(cpu, cpu->regist->pc);
    uint16_t nn = convert_8to16(&hi, &lo);
    ++cpu->regist->pc;
    if (cc)
    {
        tick_m(cpu);
        --cpu->regist->sp;
        write_mem(cpu, cpu->regist->sp, regist_hi(&cpu->regist->pc));
        --cpu->regist->sp;
        write_mem(cpu, cpu->regist->sp, regist_lo(&cpu->regist->pc));
        cpu->regist->pc = nn;
        return 6;
    }
    return 3;
}

int rst(struct cpu *cpu, uint8_t vec)
{
    tick_m(cpu);
    --cpu->regist->sp;
    write_mem(cpu, cpu->regist->sp, regist_hi(&cpu->regist->pc));
    --cpu->regist->sp;
    write_mem(cpu, cpu->regist->sp, regist_lo(&cpu->regist->pc));
    uint8_t lo = 0x00;
    uint16_t newpc = convert_8to16(&lo, &vec);
    cpu->regist->pc = newpc;
    return 4;
}
