#ifndef CORE_INTERRUPTS_H
#define CORE_INTERRUPTS_H

#include "gb_core.h"

// clang-format off
#define INTERRUPT_VBLANK    0
#define INTERRUPT_LCD       1
#define INTERRUPT_TIMER     2
#define INTERRUPT_SERIAL    3
#define INTERRUPT_JOYPAD    4
// clang-format on

static inline int get_if(struct gb_core *gb, int bit)
{
    return (gb->membus[IF] >> bit) & 0x01;
}

static inline void set_if(struct gb_core *gb, int bit)
{
    gb->membus[IF] |= (0x01 << bit);
}

static inline void clear_if(struct gb_core *gb, int bit)
{
    gb->membus[IF] &= ~(0x01 << bit);
}

static inline int get_ie(struct gb_core *gb, int bit)
{
    return (gb->membus[IE] >> bit) & 0x01;
}

static inline void set_ie(struct gb_core *gb, int bit)
{
    gb->membus[IE] |= (0x01 << bit);
}

static inline void clear_ie(struct gb_core *gb, int bit)
{
    gb->membus[IE] &= ~(0x01 << bit);
}

int check_interrupt(struct gb_core *gb);

int handle_interrupt(struct gb_core *gb, unsigned int bit);

#endif
