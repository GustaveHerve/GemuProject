#include "serial.h"

#include <stddef.h>

#include "gb_core.h"
#include "interrupts.h"

#define SC_TRANSFER_ENABLE (1 << 7)
#define SC_INTERNAL_CLK (1 << 0)

void update_serial(struct gb_core *gb)
{
    uint16_t new_clk = (gb->serial_clock + 1) & 0x1FF;

    uint8_t transfer_enabled = (gb->memory.io[IO_OFFSET(SC)] & SC_INTERNAL_CLK);
    uint8_t clk_bit = new_clk >> 8;

    uint8_t new_serial_AND = transfer_enabled & clk_bit;

    /* SC_TRANSFER_ENABLE & SERIAL_CLK bit 8 falling edge detection */
    if (get_transfer_enable(gb))
    {
        if (gb->prev_serial_AND && !new_serial_AND)
        {
            gb->memory.io[IO_OFFSET(SB)] <<= 1; /* Shift bit out */
            gb->memory.io[IO_OFFSET(SB)] |= 1;  /* Simulate no slave GameBoy connected, receiving $FF */
            ++gb->serial_acc;
        }

        if (gb->serial_acc == 8)
        {
            gb->memory.io[IO_OFFSET(SC)] &= ~0x80;
            set_if(gb, INTERRUPT_SERIAL);
        }
    }

    gb->serial_clock = new_clk;
    gb->prev_serial_AND = new_serial_AND;
    gb->serial_acc %= 8;
}
