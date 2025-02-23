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

#define CHECKSUM_ADDR 0x014D

void reset_gb(struct gb_core *gb)
{
    memset(gb->memory.vram, 0, VRAM_SIZE * sizeof(uint8_t));
    memset(gb->memory.wram, 0, WRAM_SIZE * sizeof(uint8_t));
    memset(gb->memory.oam, 0, OAM_SIZE * sizeof(uint8_t));
    memset(gb->memory.unusable_mem, 0, NOT_USABLE_SIZE * sizeof(uint8_t));
    memset(gb->memory.io, 0, IO_SIZE * sizeof(uint8_t));
    memset(gb->memory.hram, 0, HRAM_SIZE * sizeof(uint8_t));

    ppu_init(gb);
    apu_init(&gb->apu);
    mbc_reset(gb->mbc);

    gb->halt = 0;
    gb->stop = 0;

    gb->internal_div = 0;
    gb->prev_tac_AND = 0;

    gb->serial_clock = 0;
    gb->serial_acc = 0;

    gb->joyp_a = 0xF;
    gb->joyp_d = 0xF;

    gb->schedule_tima_overflow = 0;

    gb->tcycles_since_sync = 0;
    gb->last_sync_timestamp = get_nanoseconds();

    init_gb_core_post_boot(gb, gb->mbc->rom[CHECKSUM_ADDR]);
}

static int load_boot_rom(struct gb_core *gb, char *boot_rom_path)
{
    if (!gb)
        return EXIT_FAILURE;
    if (!boot_rom_path)
        return EXIT_SUCCESS;

    // Enable bootrom
    gb->memory.io[IO_OFFSET(BOOT)] = 0xFE;

    // Load bootrom
    FILE *fptr = fopen(boot_rom_path, "rb");
    if (!fptr)
    {
        fprintf(stderr, "Invalid boot rom path: %s\n", boot_rom_path);
        return EXIT_FAILURE;
    }

    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);

    if (fsize == -1)
    {
        fprintf(stderr, "Error reading boot rom file, file is empty or damaged: %s\n", boot_rom_path);
        fclose(fptr);
        return EXIT_FAILURE;
    }

    if (fsize > (uint32_t)-1)
    {
        fprintf(stderr, "Error reading boot rom file, file is too big: %s\n", boot_rom_path);
        fclose(fptr);
        return EXIT_FAILURE;
    }

    gb->memory.boot_rom_size = fsize;
    gb->memory.boot_rom = malloc(sizeof(uint8_t) * fsize);
    fread(gb->memory.boot_rom, 1, fsize, fptr);
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

    uint8_t checksum = rom[CHECKSUM_ADDR];

    // Init MBC / cartridge info and fill rom in buffer
    set_mbc(&gb->mbc, rom, rom_path);

    lcd_off(gb);

    if (!boot_rom_path)
        init_gb_core_post_boot(gb, checksum);

    return EXIT_SUCCESS;
}

void tick_m(struct gb_core *gb)
{
    if (gb->cpu.ime == 2)
        gb->cpu.ime = 1;

    for (size_t i = 0; i < 4; ++i)
    {
        gb->tcycles_since_sync += 1;

        update_timers(gb);
        update_serial(gb);

        apu_tick(gb);
        ppu_tick(gb);
    }

    dma_handle(gb);

    gb->apu.ch1.trigger_request = 0;
    gb->apu.ch2.trigger_request = 0;
    gb->apu.ch3.trigger_request = 0;
    gb->apu.ch4.trigger_request = 0;
}
