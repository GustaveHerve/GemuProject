#ifndef EMULATION_H
#define EMULATION_H

#include <SDL3/SDL_stdinc.h>

struct cpu;
struct ppu;

struct global_settings
{
    bool paused;
    bool turbo;
};

struct global_settings *get_global_settings(void);

void handle_events(struct cpu *cpu);

int main_loop(struct cpu *cpu, char *rom_path, char *boot_rom_path);

void tick_m(struct cpu *cpu);

#endif
