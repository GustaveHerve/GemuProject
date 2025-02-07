#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#include "display.h"
#include "ppu_utils.h"
#include "ring_buffer.h"

// clang-format off
#define LCDC_BG_WINDOW_ENABLE   0
#define LCDC_OBJ_ENABLE         1
#define LCDC_OBJ_SIZE           2
#define LCDC_BG_TILE_MAP        3
#define LCDC_BG_WINDOW_TILES    4
#define LCDC_WINDOW_ENABLE      5
#define LCDC_WINDOW_TILE_MAP    6
#define LCDC_LCD_PPU_ENABLE     7

#define STAT_LYC_EQUAL_LY       2
#define STAT_MODE_0_SELECT      3
#define STAT_MODE_1_SELECT      4
#define STAT_MODE_2_SELECT      5
#define STAT_LYC_SELECT         6
// clang-format on

struct fetcher
{
    int obj_index; // Used if PPU in OBJ mode
    int bottom_part;

    uint8_t attributes;
    uint8_t tileid; // Variables to Save state between dots
    uint8_t lo;
    uint8_t hi;

    uint8_t current_step; // 0 = get_tile_id
                          // 1 = get_tile_lo
                          // 2 = get_tile_hi
                          // 3 = push_pixels
    uint8_t tick;

    uint8_t lx_save;
};

struct ppu
{
    uint8_t lx;

    struct obj obj_slots[10];
    int8_t obj_count;

    RING_BUFFER(pixel) bg_fifo;
    RING_BUFFER(pixel) obj_fifo;

    struct fetcher bg_fetcher;
    struct fetcher obj_fetcher;

    uint8_t oam_locked;
    uint8_t vram_locked;

    uint8_t dma;
    uint8_t dma_acc;
    uint8_t dma_source;

    int line_dot_count; // Dot count for current scanline
    uint8_t mode1_153th;
    uint8_t first_tile;
    uint8_t current_mode;

    uint8_t win_mode;
    uint8_t win_ly; // Internal ly specific for the window which tells which allow us to "remember" which line of the
                    // window we were rendering
    uint8_t win_lx; // same for x
    uint8_t wy_trigger;

    uint8_t obj_mode;
};

static inline int get_lcdc(struct ppu *ppu, int bit)
{
    return *ppu->lcdc >> bit & 0x01;
}

static inline void set_stat(struct ppu *ppu, int bit)
{
    *ppu->stat |= 0x01 << bit;
}

static inline int get_stat(struct ppu *ppu, int bit)
{
    return (*ppu->stat >> bit) & 0x01;
}

static inline void clear_stat(struct ppu *ppu, int bit)
{
    *ppu->stat &= ~(0x01 << bit);
}

void ppu_init(struct ppu *ppu, struct cpu *cpu, struct renderer *renderer);

void ppu_reset(struct ppu *ppu);

void ppu_tick_m(struct ppu *ppu);

#endif
