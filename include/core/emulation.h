#ifndef CORE_EMULATION_H
#define CORE_EMULATION_H

#include <stdbool.h>
struct gb_core;
struct cpu;

struct global_settings
{
    bool quit_signal;
    bool paused;
    bool turbo;
    bool reset_signal;
    unsigned char save_state;
    unsigned char load_state;
    float audio_volume;
    char *open_rom;
};

void reset_gb(struct gb_core *gb);

struct global_settings *get_global_settings(void);

int load_rom(struct gb_core *gb, char *rom_path, char *boot_rom_path);

void tick_m(struct gb_core *gb);

#endif
