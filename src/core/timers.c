#include "gb_core.h"
#include "interrupts.h"

#define TAC_TIMER_ENABLED (1 << 2)
#define TAC_CLOCK_SELECT 0x3

static unsigned int clock_shifts[] = {9, 3, 5, 7};

void update_timers(struct gb_core *gb)
{
    /* TIMA overflow consequences are delayed by 4 TCycles and can be aborted before */
    if (gb->schedule_tima_overflow)
    {
        if (!gb->if_written) // TODO: if IF register not written to during the current TCycle
            set_if(gb, INTERRUPT_TIMER);
        gb->memory.io[IO_OFFSET(TIMA)] = gb->memory.io[IO_OFFSET(TMA)];
        gb->schedule_tima_overflow = 0;
    }

    uint16_t new_div = gb->internal_div;
    if (!gb->stop)
        new_div += 1;

    uint8_t previous_tima = gb->memory.io[IO_OFFSET(TIMA)];

    unsigned int clock_shift = clock_shifts[gb->memory.io[IO_OFFSET(TAC)] & TAC_CLOCK_SELECT];

    uint8_t tac_enabled = (gb->memory.io[IO_OFFSET(TAC)] & TAC_TIMER_ENABLED) >> 2;
    uint8_t selected_div_bit = (new_div >> clock_shift) & 1;

    uint8_t new_tac_AND = tac_enabled & selected_div_bit;

    /* TAC_ENABLED & CLOCK_SELECT falling edge detection */
    if (gb->prev_tac_AND && !new_tac_AND)
        ++gb->memory.io[IO_OFFSET(TIMA)];

    /* TIMA overflow consequences are delayed by 4 TCycles */
    if (previous_tima > gb->memory.io[IO_OFFSET(TIMA)])
    {
        /* Writing to TIMA in the same MCycle as the overflow will abort the overflow */
        gb->schedule_tima_overflow = !gb->tima_written;
    }

    gb->prev_tac_AND = new_tac_AND;
    gb->internal_div = new_div;
}
