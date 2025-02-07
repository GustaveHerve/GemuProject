#include "interrupts.h"

#include <assert.h>
#include <stddef.h>

#include "cpu.h"
#include "emulation.h"
#include "memory.h"
#include "utils.h"

int check_interrupt(struct cpu *cpu)
{
    if (!cpu->halt && !cpu->ime)
        return 0;

    // Joypad check
    if (((cpu->membus[0xFF00] >> 5 & 0x01) == 0x00) || (cpu->membus[0xFF00] >> 4 & 0x01) == 0x00)
    {
        for (size_t i = 0; i < 4; ++i)
        {
            if (((cpu->membus[0xFF00] >> i) & 0x01) == 0x00)
                set_if(cpu, INTERRUPT_JOYPAD);
        }
    }

    for (int i = 0; i < 5; ++i)
    {
        if (get_if(cpu, i) && get_ie(cpu, i))
        {
            cpu->halt = 0;
            if (!cpu->ime) // Wake up from halt with IME = 0
                return 0;
            handle_interrupt(cpu, i);
        }
    }
    return 1;
}

/* VBlank, LCD STAT, Timer, Serial, Joypad */
static unsigned int handlers_vector[] = {0x40, 0x48, 0x50, 0x58, 0x60};

int handle_interrupt(struct cpu *cpu, unsigned int bit)
{
    assert(bit < sizeof(handlers_vector) / sizeof(unsigned int));
    clear_if(cpu, bit);
    cpu->ime = 0;
    tick_m(cpu);
    tick_m(cpu);
    uint8_t lo = regist_lo(&cpu->regist->pc);
    uint8_t hi = regist_hi(&cpu->regist->pc);
    --cpu->regist->sp;
    write_mem(cpu, cpu->regist->sp, hi);
    --cpu->regist->sp;
    write_mem(cpu, cpu->regist->sp, lo);
    uint16_t handler = handlers_vector[bit];
    cpu->regist->pc = handler;
    tick_m(cpu);
    return 1;
}
