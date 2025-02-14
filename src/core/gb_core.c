#include "gb_core.h"

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "cpu.h"
#include "mbc_base.h"
#include "sync.h"

static void init_membus_post_boot(uint8_t *membus)
{
    membus[JOYP] = 0xCF;
    membus[SB] = 0x00;
    membus[SC] = 0x7E;
    membus[DIV] = 0xAB;
    membus[TIMA] = 0x00;
    membus[TMA] = 0x00;
    membus[TAC] = 0xF8;
    membus[IF] = 0xE1;
    membus[NR10] = 0x80;
    membus[NR11] = 0xBF;
    membus[NR12] = 0xF3;
    membus[NR13] = 0xFF;
    membus[NR14] = 0xBF;
    membus[NR21] = 0x3F;
    membus[NR22] = 0x00;
    membus[NR23] = 0xFF;
    membus[NR24] = 0xBF;
    membus[NR30] = 0x7F;
    membus[NR31] = 0xFF;
    membus[NR32] = 0x9F;
    membus[NR33] = 0xFF;
    membus[NR34] = 0xBF;
    membus[NR41] = 0xFF;
    membus[NR42] = 0x00;
    membus[NR43] = 0x00;
    membus[NR44] = 0xBF;
    membus[NR50] = 0x77;
    membus[NR51] = 0xF3;
    membus[NR52] = 0xF1;
    membus[LCDC] = 0x91;
    membus[STAT] = 0x85;
    membus[SCY] = 0x00;
    membus[SCX] = 0x00;
    membus[LY] = 0x00;
    membus[LYC] = 0x00;
    membus[DMA] = 0xFF;
    membus[BGP] = 0xFC;
    membus[OBP0] = 0xFF;
    membus[OBP1] = 0xFF;
    membus[WY] = 0x00;
    membus[WX] = 0x00;
    membus[BOOT] = 0xFF;
    membus[HDMA1] = 0xFF;
    membus[HDMA2] = 0xFF;
    membus[HDMA3] = 0xFF;
    membus[HDMA4] = 0xFF;
    membus[HDMA5] = 0xFF;
    membus[RP] = 0xFF;
    membus[BCPS] = 0xFF;
    membus[BCPD] = 0xFF;
    membus[OCPS] = 0xFF;
    membus[OCPD] = 0xFF;
    membus[SVBK] = 0xFF;
    membus[IE] = 0x00;
}

void init_gb_core_post_boot(struct gb_core *gb, int checksum)
{
    cpu_set_registers_post_boot(&gb->cpu, checksum);
    init_membus_post_boot(gb->membus);

    gb->internal_div = 0xAB00;

    gb->ppu.current_mode = 1;
    gb->ppu.line_dot_count = 400;
    gb->ppu.mode1_153th = 1;
}

void init_gb_core(struct gb_core *gb)
{
    memset(&gb->cpu, 0, sizeof(struct cpu));
    gb->membus = calloc(MEMBUS_SIZE, sizeof(uint8_t));

    ppu_init(gb);
    apu_init(&gb->apu);

    gb->halt = 0;
    gb->stop = 0;

    gb->previous_div = 0;
    gb->internal_div = 0;

    gb->serial_clock = 0;
    gb->serial_acc = 0;

    gb->joyp_a = 0xF;
    gb->joyp_d = 0xF;

    gb->mbc = NULL;

    gb->disabling_timer = 0;
    gb->schedule_tima_overflow = 0;

    gb->tcycles_since_sync = 0;
    gb->last_sync_timestamp = get_nanoseconds();
}

void free_gb_core(struct gb_core *gb)
{
    free(gb->membus);
    mbc_free(gb->mbc);
}

int serialize_gb_to_file(char *output_path, struct gb_core *gb)
{
    FILE *file;
    if ((file = fopen(output_path, "wb")) == NULL)
        return EXIT_FAILURE;

    fwrite(&gb->cpu, sizeof(uint8_t), sizeof(struct cpu), file);
    // serialize_cpu_to_stream(file, &gb->cpu);

    fwrite(&gb->ppu, sizeof(uint8_t), sizeof(struct apu), file);
    fwrite(&gb->apu, sizeof(uint8_t), sizeof(struct apu), file);

    // fwrite(gb->membus, sizeof(uint8_t), MEMBUS_SIZE, file);
    fwrite(gb->membus, sizeof(uint8_t), 0x100, file);
    fwrite(gb->membus + 0x8000, sizeof(uint8_t), 0x2000, file);
    fwrite(gb->membus + 0xC000, sizeof(uint8_t), 0x2000, file);
    fwrite(gb->membus + 0xFE00, sizeof(uint8_t), 0x200, file);

    fwrite(&gb->previous_div, sizeof(uint16_t), 2, file);
    fwrite(&gb->disabling_timer, sizeof(uint8_t), 6, file);

    fwrite(&gb->disabling_timer, sizeof(uint8_t), 6, file);

    fwrite(gb->mbc->ram, sizeof(uint8_t), gb->mbc->ram_total_size, file);
    // TODO: each MBC type needs to have its own serialize routine

    fwrite(&gb->tcycles_since_sync, sizeof(size_t), 1, file);

    fwrite(&gb->last_sync_timestamp, sizeof(int64_t), 1, file);

    mbc_serialize(gb->mbc, file);

    fclose(file);

    return EXIT_SUCCESS;
}

int load_gb_from_file(char *input_path, struct gb_core *gb)
{
    FILE *file;
    if ((file = fopen(input_path, "rb")) == NULL)
        return EXIT_FAILURE;

    fread(&gb->cpu, sizeof(uint8_t), sizeof(struct cpu), file);
    // load_cpu_from_stream(file, &gb->cpu);
    fread(&gb->ppu, sizeof(uint8_t), sizeof(struct apu), file);
    fread(&gb->apu, sizeof(uint8_t), sizeof(struct apu), file);

    // fread(gb->membus, sizeof(uint8_t), MEMBUS_SIZE, file);

    fread(gb->membus, sizeof(uint8_t), 0x100, file);
    fread(gb->membus + 0x8000, sizeof(uint8_t), 0x2000, file);
    fread(gb->membus + 0xC000, sizeof(uint8_t), 0x2000, file);
    fread(gb->membus + 0xFE00, sizeof(uint8_t), 0x200, file);

    fread(&gb->previous_div, sizeof(uint16_t), 2, file);
    fread(&gb->disabling_timer, sizeof(uint8_t), 6, file);

    fread(&gb->disabling_timer, sizeof(uint8_t), 6, file);

    fread(gb->mbc->ram, sizeof(uint8_t), gb->mbc->ram_total_size, file);

    fread(&gb->tcycles_since_sync, sizeof(size_t), 1, file);

    fread(&gb->last_sync_timestamp, sizeof(int64_t), 1, file);

    mbc_load_from_stream(gb->mbc, file);

    fclose(file);

    return EXIT_SUCCESS;
}
