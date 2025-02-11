#include "interrupts.h"

#include <assert.h>
#include <stddef.h>

#include "cpu.h"
#include "emulation.h"
#include "gb_core.h"
#include "memory.h"
#include "utils.h"

int check_interrupt(struct gb_core *gb)
{
    if (!gb->halt && !gb->cpu.ime)
        return 0;

    // Joypad check
    if (((gb->membus[0xFF00] >> 5 & 0x01) == 0x00) || (gb->membus[0xFF00] >> 4 & 0x01) == 0x00)
    {
        for (size_t i = 0; i < 4; ++i)
        {
            if (((gb->membus[0xFF00] >> i) & 0x01) == 0x00)
                set_if(gb, INTERRUPT_JOYPAD);
        }
    }

    for (int i = 0; i < 5; ++i)
    {
        if (get_if(gb, i) && get_ie(gb, i))
        {
            gb->halt = 0;
            if (!gb->cpu.ime) // Wake up from halt with IME = 0
                return 0;
            handle_interrupt(gb, i);
        }
    }
    return 1;
}

/* VBlank, LCD STAT, Timer, Serial, Joypad */
static unsigned int handlers_vector[] = {0x40, 0x48, 0x50, 0x58, 0x60};

int handle_interrupt(struct gb_core *gb, unsigned int bit)
{
    assert(bit < sizeof(handlers_vector) / sizeof(unsigned int));
    clear_if(gb, bit);
    gb->cpu.ime = 0;
    tick_m(gb);
    tick_m(gb);
    uint8_t lo = regist_lo(&gb->cpu.pc);
    uint8_t hi = regist_hi(&gb->cpu.pc);
    --gb->cpu.sp;
    write_mem(gb, gb->cpu.sp, hi);
    --gb->cpu.sp;
    write_mem(gb, gb->cpu.sp, lo);
    uint16_t handler = handlers_vector[bit];
    gb->cpu.pc = handler;
    tick_m(gb);
    return 1;
}
