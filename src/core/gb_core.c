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
    membus[IE] = 0xFF;
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
