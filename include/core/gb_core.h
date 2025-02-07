#ifndef GB_H
#define GB_H

#include "apu.h"
#include "cpu.h"
#include "ppu.h"

struct gb_core
{
    // Components
    struct cpu cpu;
    struct ppu ppu;
    struct apu apu;

    // Callbacks
};

#endif
