#include "interrupts.h"

#include <assert.h>
#include <stddef.h>

#include "cpu.h"
#include "emulation.h"
#include "gb_core.h"
#include "read.h"
#include "write.h"

// clang-format off
                                        /* VBlank   LCD     Timer   Serial  Joypad */
static unsigned int handlers_vector[] = {   0x40,   0x48,   0x50,   0x58,   0x60    };

// clang-format on

static int handle_interrupt(struct gb_core *gb)
{
    gb->cpu.ime = 0;
    tick_m(gb);
    tick_m(gb);
    uint16_t pc = gb->cpu.pc - gb->halt_bug;
    gb->halt_bug = 0;

    uint8_t hi = pc >> 8;
    uint8_t lo = pc & 0xFF;
    write_mem(gb, --gb->cpu.sp, hi);

    /* Interrupt may be aborted from the previous upper SP push writing in IE */
    uint16_t handler = 0;
    for (size_t i = 0; i < 5; ++i)
    {
        if (!get_ie(gb, i) || !get_if(gb, i))
            continue;
        clear_if(gb, i);
        handler = handlers_vector[i];
        break;
    }

    /* Lower SP push is too late to cancel even if it modifies IE */
    write_mem(gb, --gb->cpu.sp, lo);

    gb->cpu.pc = handler;
    tick_m(gb);
    return 1;
}

int check_interrupt(struct gb_core *gb)
{
    if (gb->cpu.ime == 2)
        gb->cpu.ime = 1;

    if (!gb->halt && gb->cpu.ime != 1)
        return 0;

    if (gb->memory.io[IO_OFFSET(IF)] & gb->memory.ie & 0x1F)
    {
        gb->halt = 0;
        if (!gb->cpu.ime) // Wake up from halt with IME = 0
            return 1;
        handle_interrupt(gb);
    }
    return 1;
}

void check_joyp_int(struct gb_core *gb, uint8_t prev_joyp)
{
    uint8_t curr_joyp = read_mem(gb, JOYP);
    for (size_t i = 0; i < 4; ++i)
    {
        if (((prev_joyp >> i) & 1) && !((curr_joyp >> i) & 1))
        {
            set_if(gb, INTERRUPT_JOYPAD);
            break;
        }
    }
}
