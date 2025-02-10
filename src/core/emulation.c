#include "emulation.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_stdinc.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "disassembler.h"
#include "display.h"
#include "gb_core.h"
#include "interrupts.h"
#include "mbc_base.h"
#include "serial.h"
#include "sync.h"
#include "timers.h"

struct global_settings settings = {0};

struct global_settings *get_global_settings(void)
{
    return &settings;
}

static int load_boot_rom(struct gb_core *gb, char *boot_rom_path)
{
    if (!gb)
        return EXIT_FAILURE;
    if (!boot_rom_path)
        return EXIT_SUCCESS;

    // Enable bootrom
    gb->membus[BOOT] = 0xFE;

    // Open BOOTROM
    FILE *fptr = fopen(boot_rom_path, "rb");
    if (!fptr)
    {
        fprintf(stderr, "Invalid boot rom path: %s\n", boot_rom_path);
        return EXIT_FAILURE;
    }

    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);

    fread(gb->membus, 1, fsize, fptr);
    fclose(fptr);

    return EXIT_SUCCESS;
}

int load_rom(struct gb_core *gb, char *rom_path, char *boot_rom_path)
{
    // Load boot rom if provided
    if (load_boot_rom(gb, boot_rom_path))
        return EXIT_FAILURE;

    // Open ROM, get its size and and copy its content in MBC struct
    FILE *fptr = fopen(rom_path, "rb");
    if (!fptr)
    {
        fprintf(stderr, "Invalid rom path: %s\n", rom_path);
        return EXIT_FAILURE;
    }
    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);

    uint8_t *rom = malloc(sizeof(uint8_t) * fsize);
    fread(rom, 1, fsize, fptr);
    fclose(fptr);

    uint8_t checksum = rom[0x14d];

    // Init MBC / cartridge info and fill rom in buffer
    set_mbc(&gb->mbc, rom, rom_path);

    lcd_off();

    if (!boot_rom_path)
        init_gb_core_post_boot(gb, checksum);

    return EXIT_SUCCESS;
}

void tick_m(struct gb_core *gb)
{
    gb->tcycles_since_sync += 4;

    if (gb->ime == 2)
        gb->ime = 1;

    apu_tick_m(&gb->apu);

    update_timers(&gb->cpu);
    update_serial(&gb->cpu);

    if (get_lcdc(gb->membus, LCDC_LCD_PPU_ENABLE))
        ppu_tick_m(gb);
}
