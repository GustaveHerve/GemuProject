#include "gb_core.h"

#include <stdlib.h>
#include <string.h>

#include "apu.h"
#include "common.h"
#include "cpu.h"
#include "logger.h"
#include "mbc_base.h"
#include "ppu.h"
#include "serialization.h"
#include "sync.h"

static void init_io_post_boot(struct memory_map *mem)
{
    mem->io[IO_OFFSET(JOYP)] = 0xCF;
    mem->io[IO_OFFSET(SB)] = 0x00;
    mem->io[IO_OFFSET(SC)] = 0x7E;
    mem->io[IO_OFFSET(TIMA)] = 0x00;
    mem->io[IO_OFFSET(TMA)] = 0x00;
    mem->io[IO_OFFSET(TAC)] = 0xF8;
    mem->io[IO_OFFSET(IF)] = 0xE1;
    mem->io[IO_OFFSET(NR10)] = 0x80;
    mem->io[IO_OFFSET(NR11)] = 0xBF;
    mem->io[IO_OFFSET(NR12)] = 0xF3;
    mem->io[IO_OFFSET(NR13)] = 0xFF;
    mem->io[IO_OFFSET(NR14)] = 0xBF;
    mem->io[IO_OFFSET(0xFF15)] = 0xFF;
    mem->io[IO_OFFSET(NR21)] = 0x3F;
    mem->io[IO_OFFSET(NR22)] = 0x00;
    mem->io[IO_OFFSET(NR23)] = 0xFF;
    mem->io[IO_OFFSET(NR24)] = 0xBF;
    mem->io[IO_OFFSET(NR30)] = 0x7F;
    mem->io[IO_OFFSET(NR31)] = 0xFF;
    mem->io[IO_OFFSET(NR32)] = 0x9F;
    mem->io[IO_OFFSET(NR33)] = 0xFF;
    mem->io[IO_OFFSET(NR34)] = 0xBF;
    mem->io[IO_OFFSET(0xFF1F)] = 0xFF;
    mem->io[IO_OFFSET(NR41)] = 0xFF;
    mem->io[IO_OFFSET(NR42)] = 0x00;
    mem->io[IO_OFFSET(NR43)] = 0x00;
    mem->io[IO_OFFSET(NR44)] = 0xBF;
    mem->io[IO_OFFSET(NR50)] = 0x77;
    mem->io[IO_OFFSET(NR51)] = 0xF3;
    mem->io[IO_OFFSET(NR52)] = 0xF1;
    mem->io[IO_OFFSET(LCDC)] = 0x91;
    mem->io[IO_OFFSET(STAT)] = 0x85;
    mem->io[IO_OFFSET(SCY)] = 0x00;
    mem->io[IO_OFFSET(SCX)] = 0x00;
    mem->io[IO_OFFSET(LY)] = 0x00;
    mem->io[IO_OFFSET(LYC)] = 0x00;
    mem->io[IO_OFFSET(DMA)] = 0xFF;
    mem->io[IO_OFFSET(BGP)] = 0xFC;
    mem->io[IO_OFFSET(OBP0)] = 0xFF;
    mem->io[IO_OFFSET(OBP1)] = 0xFF;
    mem->io[IO_OFFSET(WY)] = 0x00;
    mem->io[IO_OFFSET(WX)] = 0x00;
    mem->io[IO_OFFSET(BOOT)] = 0xFF;
    mem->io[IO_OFFSET(HDMA1)] = 0xFF;
    mem->io[IO_OFFSET(HDMA2)] = 0xFF;
    mem->io[IO_OFFSET(HDMA3)] = 0xFF;
    mem->io[IO_OFFSET(HDMA4)] = 0xFF;
    mem->io[IO_OFFSET(HDMA5)] = 0xFF;
    mem->io[IO_OFFSET(RP)] = 0xFF;
    mem->io[IO_OFFSET(BCPS)] = 0xFF;
    mem->io[IO_OFFSET(BCPD)] = 0xFF;
    mem->io[IO_OFFSET(OCPS)] = 0xFF;
    mem->io[IO_OFFSET(OCPD)] = 0xFF;
    mem->io[IO_OFFSET(SVBK)] = 0xFF;

    mem->ie = 0x00;
}

void init_gb_core_post_boot(struct gb_core *gb, int checksum)
{
    cpu_set_registers_post_boot(&gb->cpu, checksum);
    init_io_post_boot(&gb->memory);

    gb->internal_div = 0xABCC;
    gb->serial_clock = 460;

    gb->ppu.current_mode = 1;
    gb->ppu.line_dot_count = 400;
    gb->ppu.mode1_153th = 1;
}

int init_gb_core(struct gb_core *gb)
{
    memset(&gb->cpu, 0, sizeof(struct cpu));

    gb->memory.boot_rom_size = 0;
    gb->memory.boot_rom = NULL;
    gb->memory.vram = calloc(VRAM_SIZE, sizeof(uint8_t));
    gb->memory.wram = malloc(WRAM_SIZE * sizeof(uint8_t));
    gb->memory.unusable_mem = malloc(NOT_USABLE_SIZE * sizeof(uint8_t)); /* TODO: Probably useless */

    if (!gb->memory.vram || !gb->memory.wram || !gb->memory.unusable_mem)
    {
        LOG_ERROR("Couldn't allocate necessary memory for emulation");
        return EXIT_FAILURE;
    }

    ppu_init(gb);
    apu_init(&gb->apu);

    /* Init Wave RAM pattern */
    uint8_t wave_data[] = {
        0x84,
        0x40,
        0x43,
        0xAA,
        0x2D,
        0x78,
        0x92,
        0x3C,
        0x60,
        0x59,
        0x59,
        0xB0,
        0x34,
        0xB8,
        0x2E,
        0xDA,
    };
    memcpy(gb->memory.io + IO_OFFSET(WAVE_RAM), wave_data, WAVE_RAM_SIZE * sizeof(uint8_t));

    gb->halt = 0;
    gb->halt_bug = 0;
    gb->stop = 0;

    gb->internal_div = -24; /* TODO: investigate why this value works */

    gb->prev_tac_AND = 0;
    gb->prev_serial_AND = 0;

    gb->serial_clock = -24; /* TODO: investigate why this value works */
    gb->serial_acc = 0;

    gb->joyp_a = 0xF;
    gb->joyp_d = 0xF;

    gb->mbc = NULL;

    gb->schedule_tima_overflow = 0;

    gb->tcycles_since_sync = 0;
    gb->last_sync_timestamp = get_nanoseconds();

    gb->memory.io[IO_OFFSET(JOYP)] = 0xCF;

    return EXIT_SUCCESS;
}

void free_gb_core(struct gb_core *gb)
{
    free(gb->memory.boot_rom);
    free(gb->memory.vram);
    free(gb->memory.wram);
    free(gb->memory.unusable_mem);
    mbc_free(gb->mbc);
}

int gb_core_serialize(char *output_path, struct gb_core *gb)
{
    FILE *file;
    if (!(file = fopen(output_path, "wb")))
        return EXIT_FAILURE;

    cpu_serialize(file, &gb->cpu);
    ppu_serialize(file, &gb->ppu);
    apu_serialize(file, &gb->apu);

    fwrite_le_32(file, gb->memory.boot_rom_size);
    if (gb->memory.boot_rom_size > 0)
        fwrite(gb->memory.boot_rom, sizeof(uint8_t), gb->memory.boot_rom_size, file);

    fwrite(gb->memory.vram, sizeof(uint8_t), VRAM_SIZE, file);
    fwrite(gb->memory.wram, sizeof(uint8_t), WRAM_SIZE, file);
    fwrite(gb->memory.oam, sizeof(uint8_t), OAM_SIZE, file);
    fwrite(gb->memory.unusable_mem, sizeof(uint8_t), NOT_USABLE_SIZE, file);
    fwrite(gb->memory.io, sizeof(uint8_t), IO_SIZE, file);
    fwrite(gb->memory.hram, sizeof(uint8_t), HRAM_SIZE, file);
    fwrite(&gb->memory.ie, sizeof(uint8_t), 1, file);

    fwrite_le_16(file, gb->internal_div);

    fwrite(&gb->prev_tac_AND, sizeof(uint8_t), 1, file);
    fwrite(&gb->prev_serial_AND, sizeof(uint8_t), 1, file);
    fwrite(&gb->schedule_tima_overflow, sizeof(uint8_t), 1, file);
    fwrite(&gb->halt, sizeof(uint8_t), 1, file);
    fwrite(&gb->halt_bug, sizeof(uint8_t), 1, file);
    fwrite(&gb->stop, sizeof(uint8_t), 1, file);

    fwrite_le_16(file, gb->serial_clock);
    fwrite(&gb->serial_acc, sizeof(uint8_t), 1, file);

    fwrite_le_64(file, gb->tcycles_since_sync);
    fwrite_le_64(file, gb->last_sync_timestamp);

    /* MBC done last because it is of variable size */
    mbc_serialize(gb->mbc, file);

    fclose(file);

    return EXIT_SUCCESS;
}

int gb_core_load_from_file(char *input_path, struct gb_core *gb)
{
    FILE *file;
    if ((file = fopen(input_path, "rb")) == NULL)
        return EXIT_FAILURE;

    cpu_load_from_stream(file, &gb->cpu);
    ppu_load_from_stream(file, &gb->ppu);
    apu_load_from_stream(file, &gb->apu);

    fread_le_32(file, &gb->memory.boot_rom_size);
    if (gb->memory.boot_rom_size > 0)
    {
        if (!(gb->memory.boot_rom = realloc(gb->memory.boot_rom, gb->memory.boot_rom_size)))
        {
            fclose(file);
            return EXIT_FAILURE;
        }
        fread(gb->memory.boot_rom, sizeof(uint8_t), gb->memory.boot_rom_size, file);
    }

    fread(gb->memory.vram, sizeof(uint8_t), VRAM_SIZE, file);
    fread(gb->memory.wram, sizeof(uint8_t), WRAM_SIZE, file);
    fread(gb->memory.oam, sizeof(uint8_t), OAM_SIZE, file);
    fread(gb->memory.unusable_mem, sizeof(uint8_t), NOT_USABLE_SIZE, file);
    fread(gb->memory.io, sizeof(uint8_t), IO_SIZE, file);
    fread(gb->memory.hram, sizeof(uint8_t), HRAM_SIZE, file);
    fread(&gb->memory.ie, sizeof(uint8_t), 1, file);

    fread_le_16(file, &gb->internal_div);

    fread(&gb->prev_tac_AND, sizeof(uint8_t), 1, file);
    fread(&gb->prev_serial_AND, sizeof(uint8_t), 1, file);
    fread(&gb->schedule_tima_overflow, sizeof(uint8_t), 1, file);
    fread(&gb->halt, sizeof(uint8_t), 1, file);
    fread(&gb->halt_bug, sizeof(uint8_t), 1, file);
    fread(&gb->stop, sizeof(uint8_t), 1, file);

    fread_le_16(file, &gb->serial_clock);
    fread(&gb->serial_acc, sizeof(uint8_t), 1, file);

    fread_le_64(file, &gb->tcycles_since_sync);
    fread_le_64(file, (void *)&gb->last_sync_timestamp);

    mbc_load_from_stream(gb->mbc, file);

    fclose(file);

    return EXIT_SUCCESS;
}
