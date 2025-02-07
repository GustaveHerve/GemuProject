#include <err.h>

#include "cpu.h"
#include "utils.h"

// rlca A
// x07	1 MCycle
int rlca(struct cpu *cpu)
{
    rotl(&cpu->regist->a);
    set_z(cpu->regist, 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    cflag_rotl_set(cpu->regist, cpu->regist->a);
    return 1;
}

// rla A
// x17	1 MCycle
int rla(struct cpu *cpu)
{
    rotl_carry(cpu->regist, &cpu->regist->a);
    set_z(cpu->regist, 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 1;
}

// rrca A
// x0F	1 MCycle
int rrca(struct cpu *cpu)
{
    rotr(&cpu->regist->a);
    set_z(cpu->regist, 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    cflag_rotr_set(cpu->regist, cpu->regist->a);
    return 1;
}

// rra A
// x1F	1 MCycle
int rra(struct cpu *cpu)
{
    rotr_carry(cpu->regist, &cpu->regist->a);
    set_z(cpu->regist, 0);
    set_n(cpu->regist, 0);
    set_h(cpu->regist, 0);
    return 1;
}
