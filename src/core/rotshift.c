#include <err.h>

#include "gb_core.h"
#include "utils.h"

// rlca A
// x07	1 MCycle
int rlca(struct gb_core *gb)
{
    rotl(&gb->cpu.a);
    set_z(&gb->cpu, 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    cflag_rotl_set(&gb->cpu, gb->cpu.a);
    return 1;
}

// rla A
// x17	1 MCycle
int rla(struct gb_core *gb)
{
    rotl_carry(&gb->cpu, &gb->cpu.a);
    set_z(&gb->cpu, 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 1;
}

// rrca A
// x0F	1 MCycle
int rrca(struct gb_core *gb)
{
    rotr(&gb->cpu.a);
    set_z(&gb->cpu, 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    cflag_rotr_set(&gb->cpu, gb->cpu.a);
    return 1;
}

// rra A
// x1F	1 MCycle
int rra(struct gb_core *gb)
{
    rotr_carry(&gb->cpu, &gb->cpu.a);
    set_z(&gb->cpu, 0);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 1;
}
