#include <err.h>

#include "gb_core.h"
#include "interrupts.h"
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
    gb->internal_div = 0;
    return 1;
}

static int pending_interrupt(struct gb_core *gb)
{
    /* Joypad check */
    if ((!(gb->memory.io[IO_OFFSET(JOYP)] >> 5 & 0x01)) || !(gb->memory.io[IO_OFFSET(JOYP)] >> 4 & 0x01))
    {
        for (size_t i = 0; i < 4; ++i)
        {
            if (!((gb->memory.io[IO_OFFSET(JOYP)] >> i) & 0x01))
                set_if(gb, INTERRUPT_JOYPAD);
        }
    }

    if (gb->memory.io[IO_OFFSET(IF)] & gb->memory.ie & 0x1F)
        return 1;
    return 0;
}

// halt
int halt(struct gb_core *gb)
{
    if (gb->cpu.ime != 1 && pending_interrupt(gb))
        gb->halt_bug = 1;
    else
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
    if (gb->cpu.ime != 1 && gb->cpu.ime != 2)
        gb->cpu.ime = 3;
    return 1;
}
