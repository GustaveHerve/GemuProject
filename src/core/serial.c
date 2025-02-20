#include "serial.h"

#include <stddef.h>

#include "gb_core.h"
#include "interrupts.h"

static void transfer_complete(struct gb_core *gb)
{
    gb->memory.io[IO_OFFSET(SC)] &= ~0x80;
}

void update_serial(struct gb_core *gb)
{
    uint16_t previous_serial_clock = gb->serial_clock;
    gb->serial_clock += 1;
    /* Bit 9 falling edge detection (if we were in M-Cycle it would be bit 7) */
    if (previous_serial_clock & 0x0200 && !(gb->serial_clock & 0x0200))
    {
        if (get_clock_select(gb))
        {
            gb->memory.io[IO_OFFSET(SB)] <<= 1; /* Shift bit out */
            gb->memory.io[IO_OFFSET(SB)] |= 1;  /* Simulate no slave GameBoy connected, receiving $FF */
            ++gb->serial_acc;
        }
    }

    if (get_transfer_enable(gb) && gb->serial_acc == 8)
    {
        transfer_complete(gb);
        set_if(gb, INTERRUPT_SERIAL);
    }

    gb->serial_acc %= 8;
}
