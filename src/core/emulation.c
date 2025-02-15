#include "emulation.h"

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "display.h"
#include "mbc_base.h"
#include "serial.h"
#include "sync.h"
#include "timers.h"

struct global_settings settings = {.audio_volume = 0.4f};

struct global_settings *get_global_settings(void)
{
    return &settings;
}

void reset_gb(struct gb_core *gb)
{
    memset(gb->membus, 0, MEMBUS_SIZE * sizeof(uint8_t));

    ppu_init(gb);
    apu_init(&gb->apu);
    mbc_reset(gb->mbc);

    gb->halt = 0;
    gb->stop = 0;

    gb->previous_div = 0;
    gb->internal_div = 0;

    gb->serial_clock = 0;
    gb->serial_acc = 0;

    gb->joyp_a = 0xF;
    gb->joyp_d = 0xF;

    gb->disabling_timer = 0;
    gb->schedule_tima_overflow = 0;

    gb->tcycles_since_sync = 0;
    gb->last_sync_timestamp = get_nanoseconds();

    init_gb_core_post_boot(gb, gb->mbc->rom[0x014d]);
}

static int load_boot_rom(struct gb_core *gb, char *boot_rom_path)
{
    if (!gb)
        return EXIT_FAILURE;
    if (!boot_rom_path)
        return EXIT_SUCCESS;

    // Enable bootrom
    gb->memory.io[IO_OFFSET(BOOT)] = 0xFE;

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

    lcd_off(gb);

    if (!boot_rom_path)
        init_gb_core_post_boot(gb, checksum);

    return EXIT_SUCCESS;
}

void tick_m(struct gb_core *gb)
{
    gb->tcycles_since_sync += 4;

    if (gb->cpu.ime == 2)
        gb->cpu.ime = 1;

    apu_tick_m(gb);

    update_timers(gb);
    update_serial(gb);

    if (get_lcdc(gb->membus, LCDC_LCD_PPU_ENABLE))
        ppu_tick_m(gb);
}
