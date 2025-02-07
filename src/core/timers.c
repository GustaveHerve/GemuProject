#include "cpu.h"
#include "interrupts.h"

#define TAC_TIMER_ENABLED (0x1 << 2)
#define TAC_CLOCK_SELECT 0x3

static unsigned int clock_masks[] = {1 << 9, 1 << 3, 1 << 5, 1 << 7};

void update_timers(struct cpu *cpu)
{
    if (cpu->schedule_tima_overflow)
    {
        set_if(cpu, INTERRUPT_TIMER);
        *cpu->tima = *cpu->tma;
        cpu->schedule_tima_overflow = 0;
    }

    if (!cpu->stop)
    {
        cpu->internal_div += 4;
        *cpu->div = cpu->internal_div >> 8;
    }

    uint8_t previous_tima = *cpu->tima;
    unsigned int selected_clock = *cpu->tac & TAC_CLOCK_SELECT;
    unsigned int clock_mask = clock_masks[selected_clock];
    if (*cpu->tac & TAC_TIMER_ENABLED || cpu->disabling_timer)
    {
        /* Increase TIMA on falling edge */
        if (cpu->disabling_timer)
        {
            /* Handle TIMA increment quirk when disabling timer in TAC */
            if (cpu->previous_div & clock_mask)
                ++(*cpu->tima);
            cpu->disabling_timer = 0;
        }
        else if ((cpu->previous_div & clock_mask) && !(cpu->internal_div & clock_mask))
            ++(*cpu->tima);

        /* TIMA Overflow */
        if (previous_tima > *cpu->tima)
            cpu->schedule_tima_overflow = 1; /* Schedule an interrupt for next Mcycle */
    }

    cpu->previous_div = cpu->internal_div;
}
