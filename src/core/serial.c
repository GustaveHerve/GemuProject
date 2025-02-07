#include "serial.h"

#include <stddef.h>

#include "cpu.h"
#include "interrupts.h"

static void transfer_complete(struct cpu *cpu)
{
    *cpu->sc &= ~0x80;
}

void update_serial(struct cpu *cpu)
{
    uint8_t previous_serial_clock = cpu->serial_clock;
    cpu->serial_clock += 4;
    /* Bit 7 falling edge detection */
    if (previous_serial_clock >> 7 && !(cpu->serial_clock >> 7))
    {
        if (get_clock_select(cpu))
        {
            *cpu->sb <<= 1; /* Shift bit out */
            *cpu->sb |= 1;  /* Simulate no slave GameBoy connected, receiving $FF */
            ++cpu->serial_acc;
        }
    }

    if (get_transfer_enable(cpu) && cpu->serial_acc == 8)
    {
        transfer_complete(cpu);
        set_if(cpu, INTERRUPT_SERIAL);
    }

    cpu->serial_acc %= 8;
}
