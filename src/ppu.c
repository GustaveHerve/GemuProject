#include <stdlib.h>
#include <err.h>
#include "cpu.h"
#include "ppu.h"
#include "ppu_utils.h"
#include "emulation.h"
#include "queue.h"
#include "rendering.h"

// obj_index == -1 means BG/Win Mode
uint8_t get_tileid(struct ppu *ppu, int obj_index, int bottom_part)
{
    uint8_t tileid = 0;
    if (obj_index == -1)
    {
        uint8_t x_part = 0;
        uint8_t y_part = 0;
        int bit = 0;
        if (ppu->win_mode)
        {
            x_part = ppu->win_lx / 8;
            ppu->win_lx += 8;
            y_part = ppu->win_ly / 8;
            bit = LCDC_WINDOW_TILE_MAP;
        }
        else
        {
            x_part =  ((uint8_t) (ppu->bg_fetcher->lx_save + *ppu->scx)) / 8;
            y_part =  ((uint8_t) (*ppu->ly + *ppu->scy)) / 8;
            bit = LCDC_BG_TILE_MAP;
        }

        uint16_t address = (0x13 << 11) | (get_lcdc(ppu, bit) << 10)
            | (y_part << 5) | x_part;

        tileid = ppu->cpu->membus[address];
    }

    else
    {
        if (ppu->dma)
        {
            tileid = 0xFF;
            ppu->obj_fetcher->attributes = 0xFF;
        }
        else
        {
            tileid = *(ppu->obj_slots[obj_index].oam_address + 2);
            ppu->obj_fetcher->attributes = *(ppu->obj_slots[obj_index].oam_address + 3);
        }

        if (get_lcdc(ppu, LCDC_OBJ_SIZE))
        {
            uint8_t cond = !bottom_part;
            if ((ppu->obj_fetcher->attributes >> 6) & 0x01)
                cond = bottom_part;

            if (cond)
                tileid &= 0xFE;
            else
                tileid |= 0x01;
        }
    }

    return tileid;
}

uint8_t get_tile_lo(struct ppu *ppu, uint8_t tileid, int obj_index)
{
    uint8_t y_part = 0;
    int bit_12 = 0;
    uint8_t attributes = ppu->obj_fetcher->attributes;
    if (obj_index != -1)
    {
        y_part = (*ppu->ly - (ppu->obj_slots[obj_index].y - 16)) % 8;
        // Y flip
        if ((attributes >> 6) & 0x01)
        {
            y_part = ~y_part;
            y_part &= 0x07;
        }
    }
    else if (ppu->win_mode)
    {
        y_part = ppu->win_ly % 8;
        bit_12 = !(get_lcdc(ppu, LCDC_BG_WINDOW_TILES) | (tileid & 0x80));
    }
    else
    {
        y_part = (uint8_t)(((*ppu->ly + *ppu->scy)) % 8);
        bit_12 = !(get_lcdc(ppu, LCDC_BG_WINDOW_TILES) | (tileid & 0x80));
    }


    uint16_t address_low = (0x4 << 13) | (bit_12 << 12) |
        (tileid << 4) | (y_part << 1) | 0;

    uint8_t slice_low = ppu->cpu->membus[address_low];

    if (obj_index != -1)
    {
        // X flip
        if ((attributes >> 5) & 0x01)
        {
            slice_low = slice_xflip(slice_low);
        }
    }

    return slice_low;
}

// TODO optimize this ? (address is same as low + 1)
uint8_t get_tile_hi(struct ppu *ppu, uint8_t tileid, int obj_index)
{
    uint8_t y_part = 0;
    int bit_12 = 0;
    uint8_t attributes = ppu->obj_fetcher->attributes;
    if (obj_index != -1)
    {
        y_part = (*ppu->ly - (ppu->obj_slots[obj_index].y - 16)) % 8;
        // Y flip
        if ((attributes >> 6) & 0x01)
        {
            y_part = ~y_part;
            y_part &= 0x07;
        }
    }
    else if (ppu->win_mode)
    {
        y_part = ppu->win_ly % 8;
        bit_12 = !(get_lcdc(ppu, LCDC_BG_WINDOW_TILES) | (tileid & 0x80));
    }
    else
    {
        y_part = (uint8_t)(((*ppu->ly + *ppu->scy)) % 8);
        bit_12 = !(get_lcdc(ppu, LCDC_BG_WINDOW_TILES) | (tileid & 0x80));
    }

    uint16_t address_high = (0x4 << 13) | (bit_12 << 12) |
        (tileid << 4) | (y_part << 1) | 1;

    uint8_t slice_high = ppu->cpu->membus[address_high];

    if (obj_index != -1)
    {
        // X flip
        if ((attributes >> 5) & 0x01)
        {
            slice_high = slice_xflip(slice_high);
        }
    }

    return slice_high;
}

// Fetcher functions
void fetcher_reset(struct fetcher *f)
{
    f->attributes = 0;
    f->tileid = 0;
    f->lo = 0;
    f->hi = 0;
    f->current_step = 0;
    f->obj_index = -1;
    f->bottom_part = 0;
    f->tick = 0;
    f->lx_save = 0;
}

// Does one fetcher step (2 dots)
int bg_fetcher_step(struct ppu *ppu)
{
    struct fetcher *f = ppu->bg_fetcher;
    // BG/Win fetcher

    // Each step must take 2 dots
    if (!f->tick && f->current_step != 3)
    {
        f->tick = 1;
        // Save the state of lx for next fetch
        f->lx_save = ppu->lx;
        return 1;
    }

    switch (f->current_step)
    {
        case 0:
            f->tileid = get_tileid(ppu, -1, 0);
            f->current_step = 1;
            break;
        case 1:
            f->lo = get_tile_lo(ppu, f->tileid, -1);
            f->current_step = 2;
            break;
        case 2:
            f->hi = get_tile_hi(ppu, f->tileid, -1);
            f->current_step = 3;
            break;
        case 3:
            {
                f->tick = 0;

                //If BG empty, refill it
                if (queue_isempty(ppu->bg_fifo))
                {
                    push_slice(ppu, ppu->bg_fifo, f->hi, f->lo, -1);
                    f->current_step = 0;
                    return bg_fetcher_step(ppu);
                }
                return 0;
            }
    }

    f->tick = 0;
    return 1;
}

int obj_fetcher_step(struct ppu *ppu)
{
    struct fetcher *f = ppu->obj_fetcher;

    if (!f->tick && f->current_step != 3)
    {
        f->tick = 1;
        // Save the state of lx during first wait dot
        f->lx_save = ppu->lx;
        return 1;
    }

    switch (f->current_step)
    {
        case 0:
            f->tileid = get_tileid(ppu, f->obj_index, f->bottom_part);
            f->current_step = 1;
            break;
        case 1:
            f->lo = get_tile_lo(ppu, f->tileid, f->obj_index);
            f->current_step = 2;
            break;
        case 2:
            f->hi = get_tile_hi(ppu, f->tileid, f->obj_index);
            f->current_step = 3;
            break;
        case 3:
            {
                if (queue_isempty(ppu->obj_fifo))
                    push_slice(ppu, ppu->obj_fifo, f->hi, f->lo, f->obj_index);
                else
                    merge_obj(ppu, f->hi, f->lo, f->obj_index);

                // Fetch is done, we can reset the index
                // so that we can detect other (overlapped or not) objects
                // also mark it done
                ppu->obj_slots[f->obj_index].done = 1;
                f->obj_index = -1;
                f->current_step = 0;
                f->tick = 0;
                return 0;
            }
    }

    f->tick = 0;
    return 2;
}

void ppu_init(struct ppu *ppu, struct cpu *cpu, struct renderer *renderer)
{
    ppu->cpu = cpu;
    cpu->ppu = ppu;

    ppu->oam = cpu->membus + 0xFE00;
    ppu->lcdc = cpu->membus + 0xFF40;
    ppu->lx = 0;
    ppu->ly = cpu->membus + 0xFF44;
    ppu->lyc = cpu->membus + 0xFF45;
    ppu->scy = cpu->membus + 0xFF42;
    ppu->scx = cpu->membus + 0xFF43;
    ppu->wy = cpu->membus + 0xFF4A;
    ppu->wx = cpu->membus + 0xFF4B;
    ppu->stat = cpu->membus + 0xFF41;
    //TODO set bit 7 of stat to 1 (unused bit)

    ppu->bgp = cpu->membus + 0xFF47;
    ppu->obp0 = cpu->membus + 0xFF48;
    ppu->obp1 = cpu->membus + 0xFF49;

    ppu->obj_count = 0;

    ppu->bg_fifo = queue_init();
    ppu->obj_fifo = queue_init();

    ppu->bg_fetcher = malloc(sizeof(struct fetcher));
    ppu->obj_fetcher = malloc(sizeof(struct fetcher));

    fetcher_reset(ppu->bg_fetcher);
    fetcher_reset(ppu->obj_fetcher);

    ppu->renderer = renderer;

    ppu->current_mode = 0;
    ppu->oam_locked = 0;
    ppu->vram_locked = 0;

    ppu->dma = 0;
    ppu->dma_acc = 0;

    ppu->line_dot_count = 0;
    ppu->mode1_153th = 0;
    ppu->first_tile = 1;

    ppu->win_mode = 0;
    ppu->win_ly = 0;
    ppu->win_lx = 7;
    ppu->wy_trigger = 0;
    
    ppu->obj_mode = 0;

    *ppu->lcdc = 0x00;
    *ppu->stat = 0x84;
    *ppu->scy = 0x00;
    *ppu->scx = 0x00;
    *ppu->ly = 0x00;
    *ppu->lyc = 0x00;
    *ppu->bgp = 0xFC;
    *ppu->obp0 = 0xFF;
    *ppu->obp1 = 0xFF;
    *ppu->wx = 0x00;
    *ppu->wy = 0x00;
}

void ppu_free(struct ppu *ppu)
{
    free(ppu->bg_fetcher);
    free(ppu->obj_fetcher);
    queue_free(ppu->bg_fifo);
    queue_free(ppu->obj_fifo);
    free(ppu);
}

// Sets back PPU to default state when turned off
void ppu_reset(struct ppu *ppu)
{
    *ppu->ly = 0;
    ppu->lx = 0;
    ppu->current_mode = 0;
    ppu->mode1_153th = 0;
    ppu->line_dot_count = 0;

    ppu->oam_locked = 0;
    ppu->vram_locked = 0;

    *ppu->stat &= ~0x03;
    check_lyc(ppu, 0);

    fetcher_reset(ppu->bg_fetcher);
    fetcher_reset(ppu->obj_fetcher);

    queue_clear(ppu->obj_fifo);
    queue_clear(ppu->bg_fifo);

    lcd_off(ppu->cpu);
}

// Mode 2
int oam_scan(struct ppu *ppu)
{
    ppu->oam_locked = 1;
    ppu->vram_locked = 0;

    uint8_t *obj_y = ppu->oam + 2 * (ppu->line_dot_count);
    if (ppu->obj_count < 10)
    {
        // 8x16 (LCDC bit 2 = 1) or 8x8 (LCDC bit 2 = 0)
        // TODO: obj_y + 1 != 0 is weird ? check this
        int y_max_offset = get_lcdc(ppu, LCDC_OBJ_SIZE) ? 16 : 8;
        if (*(obj_y + 1) != 0 && *ppu->ly + 16 >= *obj_y
                && *ppu->ly + 16 < *obj_y + y_max_offset)
        {
            ppu->obj_slots[ppu->obj_count].y = *obj_y;
            ppu->obj_slots[ppu->obj_count].x = *(obj_y + 1);
            ppu->obj_slots[ppu->obj_count].oam_address = obj_y;
            ppu->obj_slots[ppu->obj_count].done = 0;
            ++ppu->obj_count;
        }
    }

    ppu->line_dot_count += 2;

    // Switch to mode 3 and reset FIFOs
    if (ppu->line_dot_count >= 80)
    {
        ppu->current_mode = 3;
        queue_clear(ppu->bg_fifo);
        queue_clear(ppu->obj_fifo);
    }

    return 2;
}

uint8_t mode2_handler(struct ppu *ppu)
{
    if (ppu->line_dot_count == 0)
    {
        set_stat(ppu, 1);
        clear_stat(ppu, 0);
        // Check the WY trigger
        if (*ppu->ly == *ppu->wy)
            ppu->wy_trigger = 1;
    }

    check_lyc(ppu, 0);
    
    ppu->first_tile = 1;
    oam_scan(ppu);
    return 2;
}

// Do one iteration of both fetchers step (2 dots)
uint8_t fetchers_step(struct ppu *ppu)
{
    // There is a new object to render
    if (ppu->obj_fetcher->obj_index != -1)
    {
        // If OBJ mode is already enabled, it means either we are fetching the
        // obj_index, or a new object came up overlapping the currently rendering one
        if (!ppu->obj_mode)
        {
            // BG FIFO must not be empty and BG fetcher must have a slice ready to be pushed
            if (!queue_isempty(ppu->bg_fifo) && ppu->bg_fetcher->current_step == 3)
            {
                // Once we enter OBJ mode, we have at least 8+ BG pixels fetched
                ppu->obj_mode = 1;
            }
        }
    }

    if (ppu->obj_mode)
        obj_fetcher_step(ppu);
    else
        bg_fetcher_step(ppu);

    return 1;
}

// Send one pixel to the LCD (1 dot)
uint8_t send_pixel(struct ppu *ppu)
{
    if (queue_isempty(ppu->bg_fifo))
        return 0;


    // Don't draw BG prefetch + shift SCX for first BG tile
    if (!ppu->win_mode && ppu->first_tile && ppu->lx > 7)
    {
        int discard = *ppu->scx % 8;
        if (ppu->bg_fifo->count <= 8 - discard)
        {
            struct pixel p = select_pixel(ppu);
            draw_pixel(ppu->cpu, p);
        }
        else if (!queue_isempty(ppu->bg_fifo))
        {
            queue_pop(ppu->bg_fifo);
            --ppu->lx;
        }
    }
    else if (ppu->lx > 7 && ppu->lx <= 167)
    {
        struct pixel p = select_pixel(ppu);
        draw_pixel(ppu->cpu, p);
    }

    if (ppu->first_tile && queue_isempty(ppu->bg_fifo))
        ppu->first_tile = 0;

    return 1;
}

uint8_t mode3_handler(struct ppu *ppu)
{
    // End of mode 3, go to HBlank (mode 0)
    if (ppu->lx > 167)
    {
        // Update WIN internal LY and reset internal LX
        if (ppu->win_mode)
        {
            ++ppu->win_ly;
            ppu->win_lx = 7;
        }
        ppu->current_mode = 0;
        return 0;
    }

    // Start of mode 3
    if (ppu->line_dot_count == 80)
    {
        set_stat(ppu, 1);
        set_stat(ppu, 0);

        // Lock OAM and VRAM read (return FF)
        ppu->oam_locked = 1;
        ppu->vram_locked = 1;

        // Reset FIFOs and Fetchers
        queue_clear(ppu->bg_fifo);
        queue_clear(ppu->obj_fifo);

        fetcher_reset(ppu->bg_fetcher);
        fetcher_reset(ppu->obj_fetcher);
    }

    // Check if window triggers are fulfilled
    if (!ppu->win_mode && on_window(ppu))
    {
        // Reset BG Fetcher and FIFO to current step 0 and enable win mode
        queue_clear(ppu->bg_fifo);
        ppu->bg_fetcher->current_step = 0;
        ppu->bg_fetcher->tick = 0;
        ppu->win_mode = 1;
    }

    // Check for object if we are not already treating one
    int obj = -1;
    int bottom_part = 0;
    if (ppu->obj_fetcher->obj_index == -1 && get_lcdc(ppu, LCDC_OBJ_ENABLE))
    {
        obj = on_object(ppu,  &bottom_part);
        if (obj > -1)
        {
            ppu->obj_fetcher->obj_index = obj;
            ppu->obj_fetcher->bottom_part = bottom_part;
        }
    }

    fetchers_step(ppu);

    // OBJ mode = 1 and OBJ index = -1 means we just ended fetching an object
    // if so check again for remaining not done object on the same LX and LY
    if (ppu->obj_mode && ppu->obj_fetcher->obj_index == -1)
    {
        ppu->obj_mode = 0;
        // Force a new iteration of the loop in case of another object
        if (on_object(ppu, NULL) > -1)
            return 0;
    }

    // If we are in OBJ mode, we are fetching an object (FIFOs stall)
    if (ppu->obj_fetcher->obj_index == -1 && !ppu->obj_mode)
    {
        // Prefetch pixels must be discarded
        // wait for the BG FIFO to be filled before popping
        if (ppu->lx < 8)
        {
            if (!queue_isempty(ppu->bg_fifo))
            {
                // Pop current pixel and do nothing with it
                select_pixel(ppu);
                ++ppu->lx;
            }
        }
        else
        {
            // Attempt to draw current pixel on the LCD
            if (send_pixel(ppu))
                ++ppu->lx;
        }
    }

    ++ppu->line_dot_count;
    return 1;
}

// Mode 0
uint8_t mode0_handler(struct ppu *ppu)
{
    if (get_stat(ppu, 1) || get_stat(ppu, 0))
    {
        clear_stat(ppu, 1);
        clear_stat(ppu, 0);
        if (get_stat(ppu, 3) && !get_if(ppu->cpu, 1))
            set_if(ppu->cpu, 1);
    }

    ppu->obj_fetcher->obj_index = -1;
    ppu->oam_locked = 0;
    ppu->vram_locked = 0;
    // TODO verify dma lock
    if (ppu->line_dot_count < 456)
    {
        ++ppu->line_dot_count;
        return 1;
    }

    // Exit HBlank
    ppu->lx = 0;
    *ppu->ly += 1;

    ppu->win_mode = 0;

    ppu->line_dot_count = 0;
    ppu->current_mode = 2;
    ppu->obj_count = 0;

    set_stat(ppu, 1);
    clear_stat(ppu, 0);
    if (get_stat(ppu, 5) && !get_stat(ppu, 4))
        set_if(ppu->cpu, 1);

    // Start VBlank
    if (*ppu->ly > 143)
    {
        ppu->wy_trigger = 0;
        ppu->current_mode = 1;
        ppu->mode1_153th = 0;
    }

    return 0;
}

uint8_t mode1_handler(struct ppu *ppu)
{
    if (*ppu->ly == 144 && ppu->line_dot_count == 0)
    {
        clear_stat(ppu, 1);
        set_stat(ppu, 0);
        set_if(ppu->cpu, 0); // VBlank Interrupt
        if (get_stat(ppu, 4) && !get_stat(ppu, 3)) // STAT VBlank Interrupt
            set_if(ppu->cpu, 1);
    }

    check_lyc(ppu, ppu->mode1_153th);

    if (ppu->line_dot_count < 456)
    {
        // LY = 153, special case with LY = 0 after 1 MCycle
        if (*ppu->ly == 153 && !ppu->mode1_153th && ppu->line_dot_count >= 4)
        {
            *ppu->ly = 0;
            ppu->mode1_153th = 1;
        }
        ++ppu->line_dot_count;
        return 1;
    }

    if (!ppu->mode1_153th && *ppu->ly < 153)    // Go to next VBlank line
    {
        *ppu->ly += 1;
        ppu->line_dot_count = 0;
        return 0;
    }

    // Else exit VBlank, enter OAM Scan
    ppu->line_dot_count = 0;
    ppu->mode1_153th = 0;
    ppu->current_mode = 2;
    *ppu->ly = 0;
    ppu->win_ly = 0;
    ppu->win_lx = 7;
    return 0;
}

void ppu_tick_m(struct ppu *ppu)
{
    int dots = 0;
    while (dots < 4)
    {
        switch (ppu->current_mode)
        {
            case 2:
                dots += mode2_handler(ppu);
                break;
            case 3:
                dots += mode3_handler(ppu);
                break;
            case 0:
                dots += mode0_handler(ppu);
                break;
            case 1:
                dots += mode1_handler(ppu);
                break;
        }
    }

    // DMA handling
    // DMA first setup MCycle
    if (ppu->dma == 2)
        ppu->dma = 1;
    else if (ppu->dma == 1)
    {
        ppu->cpu->membus[0xFE00 + ppu->dma_acc] =
            read_mem(ppu->cpu, (ppu->dma_source << 8) + ppu->dma_acc);
        ++ppu->dma_acc;
        if (ppu->dma_acc >= 160)
            ppu->dma = 0;
    }
}
