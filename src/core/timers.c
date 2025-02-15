#include "gb_core.h"
#include "interrupts.h"

#define TAC_TIMER_ENABLED (0x1 << 2)
#define TAC_CLOCK_SELECT 0x3

static unsigned int clock_masks[] = {1 << 9, 1 << 3, 1 << 5, 1 << 7};

void update_timers(struct gb_core *gb)
{
    if (gb->schedule_tima_overflow)
    {
        set_if(gb, INTERRUPT_TIMER);
        gb->memory.io[IO_OFFSET(TIMA)] = gb->memory.io[IO_OFFSET(TMA)];
        gb->schedule_tima_overflow = 0;
    }

    if (!gb->stop)
    {
        gb->internal_div += 4;
        gb->memory.io[IO_OFFSET(DIV)] = gb->internal_div >> 8;
    }

    uint8_t previous_tima = gb->memory.io[IO_OFFSET(TIMA)];
    unsigned int selected_clock = gb->memory.io[IO_OFFSET(TAC)] & TAC_CLOCK_SELECT;
    unsigned int clock_mask = clock_masks[selected_clock];
    if (gb->memory.io[IO_OFFSET(TAC)] & TAC_TIMER_ENABLED || gb->disabling_timer)
    {
        /* Increase TIMA on falling edge */
        if (gb->disabling_timer)
        {
            /* Handle TIMA increment quirk when disabling timer in TAC */
            if (gb->previous_div & clock_mask)
                ++gb->memory.io[IO_OFFSET(TIMA)];
            gb->disabling_timer = 0;
        }
        else if ((gb->previous_div & clock_mask) && !(gb->internal_div & clock_mask))
            ++gb->memory.io[IO_OFFSET(TIMA)];

        /* TIMA Overflow */
        if (previous_tima > gb->memory.io[IO_OFFSET(TIMA)])
            gb->schedule_tima_overflow = 1; /* Schedule an interrupt for next Mcycle */
    }

    gb->previous_div = gb->internal_div;
}
