#include "emulation.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"
#include "display.h"
#include "logger.h"
#include "mbc_base.h"
#include "serial.h"
#include "sync.h"
#include "timers.h"

struct global_settings settings = {
    .audio_volume = 1.0f,
    .render_period_ns = 1e9 / 165,
    .apu_channels_enable = {true, true, true, true},
};

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
    gb->halt_bug = 0;
    gb->stop = 0;

    gb->internal_div = -24;
    gb->prev_tac_AND = 0;

    gb->prev_serial_AND = 0;

    gb->serial_clock = -24;
    gb->serial_acc = 0;

    gb->joyp_a = 0xF;
    gb->joyp_d = 0xF;

    gb->schedule_tima_overflow = 0;

    gb->tcycles_since_sync = 0;
    gb->last_sync_timestamp = get_nanoseconds();

    init_gb_core_post_boot(gb, gb->mbc->rom[CHECKSUM_ADDR]);
}

static int is_regular_file(char *path)
{
    if (!path)
        return 0;
    struct stat st_buf;
    if (stat(path, &st_buf))
        return 0;
    return S_ISREG(st_buf.st_mode);
}

static int load_boot_rom(struct gb_core *gb, char *boot_rom_path)
{
    assert(boot_rom_path);

    int err_code = EXIT_SUCCESS;
    FILE *fptr = NULL;

    if (!is_regular_file(boot_rom_path))
    {
        LOG_ERROR("Invalid BOOT ROM path (not a regular file): %s", boot_rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }

    /* Load bootrom */
    fptr = fopen(boot_rom_path, "rb");
    if (!fptr)
    {
        LOG_ERROR("Invalid BOOT ROM path (error opening the file): %s", boot_rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }

    /* Enable bootrom by clearing bit 0 of BOOT */
    gb->memory.io[IO_OFFSET(BOOT)] = 0xFE;

    /* Get total size of file */
    if (fseek(fptr, 0, SEEK_END))
    {
        LOG_ERROR("Error reading file: %s\n", boot_rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }
    long fsize = 0;
    if ((fsize = ftell(fptr)) == -1)
    {
        LOG_ERROR("Error reading file: %s\n", boot_rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }
    rewind(fptr);

    if (fsize == -1)
    {
        LOG_ERROR("Could not load BOOT ROM, file is empty or damaged: %s", boot_rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }

    if (fsize > (uint32_t)-1)
    {
        LOG_ERROR("Could not load BOOT ROM, file is too big: %s", boot_rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }

    gb->memory.boot_rom_size = fsize;
    if (!(gb->memory.boot_rom = malloc(sizeof(uint8_t) * fsize)))
    {
        LOG_ERROR("Could not load BOOT ROM, error allocating memory");
        err_code = EXIT_FAILURE;
        goto exit;
    }

    fread(gb->memory.boot_rom, 1, fsize, fptr);
    if (ferror(fptr))
    {
        LOG_ERROR("ERROR: error loading BOOT ROM: %s", boot_rom_path);
        err_code = EXIT_FAILURE;
    }

exit:
    if (fptr)
        fclose(fptr);
    return err_code;
}

int load_rom(struct gb_core *gb, char *rom_path, char *boot_rom_path)
{
    assert(gb && rom_path);

    int err_code = EXIT_SUCCESS;
    FILE *fptr = NULL;
    uint8_t *rom = NULL;

    if (boot_rom_path && load_boot_rom(gb, boot_rom_path))
    {
        err_code = EXIT_FAILURE;
        goto exit;
    }

    if (!is_regular_file(rom_path))
    {
        LOG_ERROR("Invalid rom path (not a regular file): %s", rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }

    fptr = fopen(rom_path, "rb");
    if (!fptr)
    {
        LOG_ERROR("No such file: %s", rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }

    /* Get total size of file */
    if (fseek(fptr, 0, SEEK_END))
    {
        LOG_ERROR("Error reading file: %s", rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }
    long fsize = 0;
    if ((fsize = ftell(fptr)) == -1)
    {
        LOG_ERROR("Error reading file: %s", rom_path);
        err_code = EXIT_FAILURE;
        goto exit;
    }
    rewind(fptr);

    /* Init MBC / cartridge info and fill rom in buffer */
    if (!(rom = malloc(sizeof(uint8_t) * fsize)))
    {
        LOG_ERROR("Error allocating memory for loading ROM file");
        err_code = EXIT_FAILURE;
        goto exit;
    }

    fread(rom, 1, fsize, fptr);
    if (ferror(fptr))
    {
        LOG_ERROR("Error loading ROM: %s\n", rom_path);
        err_code = EXIT_FAILURE;
        free(rom);
        goto exit;
    }

    uint8_t checksum = rom[CHECKSUM_ADDR];
    if (set_mbc(&gb->mbc, rom, rom_path))
    {
        err_code = EXIT_FAILURE;
        goto exit;
    }

    lcd_off(gb);
    if (!boot_rom_path)
        init_gb_core_post_boot(gb, checksum);

exit:
    if (fptr)
        fclose(fptr);
    return err_code;
}

void tick_m(struct gb_core *gb)
{
    if (gb->cpu.ime > 1)
        --gb->cpu.ime;

    for (size_t i = 0; i < 4; ++i)
    {
        gb->tcycles_since_sync += 1;

        update_timers(gb); /* TODO: DIV is actually increasing every M-cycles */
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
