#ifndef EMULATION_H
#define EMULATION_H

#include <stdint.h>

#include "SDL_stdinc.h"

struct cpu;
struct ppu;

struct global_settings
{
    SDL_bool paused;
    SDL_bool turbo;
};

struct global_settings *get_global_settings(void);

void handle_events(struct cpu *cpu);

int main_loop(struct cpu *cpu, char *rom_path, char *boot_rom_path);

void tick_m(struct cpu *cpu);

#endif
