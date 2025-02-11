#include "serial.h"

#include <stddef.h>

#include "gb_core.h"
#include "interrupts.h"

static void transfer_complete(struct gb_core *gb)
{
    gb->membus[SC] &= ~0x80;
}

void update_serial(struct gb_core *gb)
{
    uint8_t previous_serial_clock = gb->serial_clock;
    gb->serial_clock += 4;
    /* Bit 7 falling edge detection */
    if (previous_serial_clock >> 7 && !(gb->serial_clock >> 7))
    {
        if (get_clock_select(gb))
        {
            gb->membus[SB] <<= 1; /* Shift bit out */
            gb->membus[SB] |= 1;  /* Simulate no slave GameBoy connected, receiving $FF */
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
