#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "cpu.h"

// clang-format off
#define INTERRUPT_VBLANK    0
#define INTERRUPT_LCD       1
#define INTERRUPT_TIMER     2
#define INTERRUPT_SERIAL    3
#define INTERRUPT_JOYPAD    4
// clang-format on

static inline int get_if(struct cpu *cpu, int bit)
{
    return (*cpu->_if >> bit) & 0x01;
}

static inline void set_if(struct cpu *cpu, int bit)
{
    *cpu->_if = *cpu->_if | (0x01 << bit);
}

static inline void clear_if(struct cpu *cpu, int bit)
{
    *cpu->_if = *cpu->_if & ~(0x01 << bit);
}

static inline int get_ie(struct cpu *cpu, int bit)
{
    return (*cpu->ie >> bit) & 0x01;
}

static inline void set_ie(struct cpu *cpu, int bit)
{
    *cpu->ie = *cpu->ie | (0x01 << bit);
}

static inline void clear_ie(struct cpu *cpu, int bit)
{
    *cpu->ie = *cpu->ie & ~(0x01 << bit);
}

int check_interrupt(struct cpu *cpu);

int handle_interrupt(struct cpu *cpu, unsigned int bit);

#endif
