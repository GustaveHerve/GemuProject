#include <err.h>

#include "gb_core.h"
#include "utils.h"

// nop
// x00	1 MCycle
int nop(void)
{
    return 1;
}

// stop
// x10	1 MCycle
int stop(struct gb_core *gb)
{
    gb->stop = 1;
    /* Reset DIV Timer */
    gb->internal_div = 0;
    return 1;
}

// halt
int halt(struct gb_core *gb)
{
    gb->halt = 1;
    return 1;
}

// ccf
// x3F	1 MCycle
int ccf(struct gb_core *gb)
{
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    gb->cpu.f ^= 0x10;
    return 1;
}

// scf
// x37	1 MCycle
int scf(struct gb_core *gb)
{
    set_c(&gb->cpu, 1);
    set_n(&gb->cpu, 0);
    set_h(&gb->cpu, 0);
    return 1;
}

// di
// xF3  1 MCycle
int di(struct gb_core *gb)
{
    gb->cpu.ime = 0;
    return 1;
}

// ei
// xFB 1 MCycle
int ei(struct gb_core *gb)
{
    // Schedule a IME enable
    gb->cpu.ime = 2;
    return 1;
}
