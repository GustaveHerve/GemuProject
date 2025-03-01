#include "ppu.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "display.h"
#include "emulation.h"
#include "gb_core.h"
#include "interrupts.h"
#include "read.h"
#include "serialization.h"

static uint8_t get_tileid(struct gb_core *gb, int obj_index, int bottom_part)
{
    uint8_t tileid = 0;
    /* obj_index == -1 means BG/Win Mode */
    if (obj_index == -1)
    {
        uint8_t x_part = 0;
        uint8_t y_part = 0;
        int bit = 0;
        if (gb->ppu.win_mode)
        {
            x_part = gb->ppu.win_lx / 8;
            gb->ppu.win_lx += 8;
            y_part = gb->ppu.win_ly / 8;
            bit = LCDC_WINDOW_TILE_MAP;
        }
        else
        {
            x_part = ((uint8_t)(gb->ppu.bg_fetcher.lx_save + gb->memory.io[IO_OFFSET(SCX)])) / 8;
            y_part = ((uint8_t)(gb->memory.io[IO_OFFSET(LY)] + gb->memory.io[IO_OFFSET(SCY)])) / 8;
            bit = LCDC_BG_TILE_MAP;
        }

        uint16_t address = (0x13 << 11) | (get_lcdc(gb->memory.io, bit) << 10) | (y_part << 5) | x_part;

        tileid = gb->memory.vram[VRAM_OFFSET(address)];
    }

    else
    {
        if (gb->ppu.dma)
        {
            tileid = 0xFF;
            gb->ppu.obj_fetcher.attributes = 0xFF;
        }
        else
        {
            tileid = gb->memory.oam[gb->ppu.obj_slots[obj_index].oam_offset + 2];
            gb->ppu.obj_fetcher.attributes = gb->memory.oam[gb->ppu.obj_slots[obj_index].oam_offset + 3];
        }

        if (get_lcdc(gb->memory.io, LCDC_OBJ_SIZE))
        {
            uint8_t cond = !bottom_part;
            if ((gb->ppu.obj_fetcher.attributes >> 6) & 1)
                cond = bottom_part;

            if (cond)
                tileid &= 0xFE;
            else
                tileid |= 0x01;
        }
    }

    return tileid;
}

static uint8_t get_tile_lo(struct gb_core *gb, uint8_t tileid, int obj_index)
{
    uint8_t y_part = 0;
    int bit_12 = 0;
    uint8_t attributes = gb->ppu.obj_fetcher.attributes;
    if (obj_index != -1)
    {
        y_part = (gb->memory.io[IO_OFFSET(LY)] - (gb->ppu.obj_slots[obj_index].y - 16)) % 8;
        /* Y flip */
        if ((attributes >> 6) & 1)
        {
            y_part = ~y_part;
            y_part &= 0x07;
        }
    }
    else if (gb->ppu.win_mode)
    {
        y_part = gb->ppu.win_ly % 8;
        bit_12 = !(get_lcdc(gb->memory.io, LCDC_BG_WINDOW_TILES) | (tileid & 0x80));
    }
    else
    {
        y_part = (uint8_t)(((gb->memory.io[IO_OFFSET(LY)] + gb->memory.io[IO_OFFSET(SCY)])) % 8);
        bit_12 = !(get_lcdc(gb->memory.io, LCDC_BG_WINDOW_TILES) | (tileid & 0x80));
    }

    uint16_t address_low = (0x4 << 13) | (bit_12 << 12) | (tileid << 4) | (y_part << 1) | 0;

    uint8_t slice_low = gb->memory.vram[VRAM_OFFSET(address_low)];

    if (obj_index != -1)
    {
        /* X flip */
        if ((attributes >> 5) & 0x01)
            slice_low = slice_xflip(slice_low);
    }

    return slice_low;
}

/* TODO optimize this ? (address is same as low + 1) */
static uint8_t get_tile_hi(struct gb_core *gb, uint8_t tileid, int obj_index)
{
    uint8_t y_part = 0;
    int bit_12 = 0;
    uint8_t attributes = gb->ppu.obj_fetcher.attributes;
    if (obj_index != -1)
    {
        y_part = (gb->memory.io[IO_OFFSET(LY)] - (gb->ppu.obj_slots[obj_index].y - 16)) % 8;
        /* Y flip */
        if ((attributes >> 6) & 0x01)
            y_part = (~y_part) & 0x07;
    }
    else if (gb->ppu.win_mode)
    {
        y_part = gb->ppu.win_ly % 8;
        bit_12 = !(get_lcdc(gb->memory.io, LCDC_BG_WINDOW_TILES) | (tileid & 0x80));
    }
    else
    {
        y_part = (uint8_t)(((gb->memory.io[IO_OFFSET(LY)] + gb->memory.io[IO_OFFSET(SCY)])) % 8);
        bit_12 = !(get_lcdc(gb->memory.io, LCDC_BG_WINDOW_TILES) | (tileid & 0x80));
    }

    uint16_t address_high = (0x4 << 13) | (bit_12 << 12) | (tileid << 4) | (y_part << 1) | 1;

    uint8_t slice_high = gb->memory.vram[VRAM_OFFSET(address_high)];

    if (obj_index != -1)
    {
        /* X flip */
        if ((attributes >> 5) & 1)
            slice_high = slice_xflip(slice_high);
    }

    return slice_high;
}

// Fetcher functions
static void fetcher_reset(struct fetcher *f)
{
    memset(f, 0, sizeof(struct fetcher));
    f->obj_index = -1;
}

// Does one fetcher step (2 dots)
static int bg_fetcher_step(struct gb_core *gb)
{
    // BG/Win fetcher
    struct fetcher *f = &gb->ppu.bg_fetcher;

    // Each step must take 2 dots
    if (!f->tick && f->current_step != 3)
    {
        f->tick = 1;
        // Save the state of lx for next fetch
        f->lx_save = gb->ppu.lx;
        return 1;
    }

    switch (f->current_step)
    {
    case 0:
        f->tileid = get_tileid(gb, -1, 0);
        f->current_step = 1;
        break;
    case 1:
        f->lo = get_tile_lo(gb, f->tileid, -1);
        f->current_step = 2;
        break;
    case 2:
        f->hi = get_tile_hi(gb, f->tileid, -1);
        f->current_step = 3;
        break;
    case 3:
    {
        f->tick = 0;

        // If BG empty, refill it
        if (RING_BUFFER_IS_EMPTY(pixel, &gb->ppu.bg_fifo))
        {
            push_slice(gb, &gb->ppu.bg_fifo, f->hi, f->lo, -1);
            f->current_step = 0;
            return bg_fetcher_step(gb);
        }
        return 0;
    }
    }

    f->tick = 0;
    return 1;
}

static int obj_fetcher_step(struct gb_core *gb)
{
    struct fetcher *f = &gb->ppu.obj_fetcher;

    if (!f->tick && f->current_step != 3)
    {
        f->tick = 1;
        // Save the state of lx during first wait dot
        f->lx_save = gb->ppu.lx;
        return 1;
    }

    switch (f->current_step)
    {
    case 0:
        f->tileid = get_tileid(gb, f->obj_index, f->bottom_part);
        f->current_step = 1;
        break;
    case 1:
        f->lo = get_tile_lo(gb, f->tileid, f->obj_index);
        f->current_step = 2;
        break;
    case 2:
        f->hi = get_tile_hi(gb, f->tileid, f->obj_index);
        f->current_step = 3;
        break;
    case 3:
    {
        if (RING_BUFFER_IS_EMPTY(pixel, &gb->ppu.obj_fifo))
            push_slice(gb, &gb->ppu.obj_fifo, f->hi, f->lo, f->obj_index);
        else
            merge_obj(gb, f->hi, f->lo, f->obj_index);

        // Fetch is done, we can reset the index
        // so that we can detect other (overlapped or not) objects
        // also mark it done
        gb->ppu.obj_slots[f->obj_index].done = 1;
        f->obj_index = -1;
        f->current_step = 0;
        f->tick = 0;
        return 0;
    }
    }

    f->tick = 0;
    return 2;
}

void ppu_init(struct gb_core *gb)
{
    gb->ppu.mode2_tick = 0;

    gb->ppu.lx = 0;
    gb->ppu.obj_count = 0;

    RING_BUFFER_INIT(pixel, &gb->ppu.bg_fifo);
    RING_BUFFER_INIT(pixel, &gb->ppu.obj_fifo);

    fetcher_reset(&gb->ppu.bg_fetcher);
    fetcher_reset(&gb->ppu.obj_fetcher);

    gb->ppu.current_mode = 0;
    gb->ppu.oam_locked = 0;
    gb->ppu.vram_locked = 0;

    gb->ppu.dma = 0;
    gb->ppu.dma_acc = 0;

    RING_BUFFER_INIT(dma_request, &gb->ppu.dma_requests);

    gb->ppu.line_dot_count = 0;
    gb->ppu.mode1_153th = 0;
    gb->ppu.first_tile = 1;

    gb->ppu.win_mode = 0;
    gb->ppu.win_ly = 0;
    gb->ppu.win_lx = 7;
    gb->ppu.wy_trigger = 0;

    gb->ppu.obj_mode = 0;

    gb->memory.io[IO_OFFSET(LCDC)] = 0x00;
    gb->memory.io[IO_OFFSET(STAT)] = 0x84;
    gb->memory.io[IO_OFFSET(SCY)] = 0x00;
    gb->memory.io[IO_OFFSET(SCX)] = 0x00;
    gb->memory.io[IO_OFFSET(LY)] = 0x00;
    gb->memory.io[IO_OFFSET(LYC)] = 0x00;
    gb->memory.io[IO_OFFSET(BGP)] = 0xFC;
    gb->memory.io[IO_OFFSET(OBP0)] = 0xFF;
    gb->memory.io[IO_OFFSET(OBP1)] = 0xFF;
    gb->memory.io[IO_OFFSET(WX)] = 0x00;
    gb->memory.io[IO_OFFSET(WY)] = 0x00;
}

// Sets back PPU to default state when turned off
void ppu_reset(struct gb_core *gb)
{
    gb->memory.io[IO_OFFSET(LY)] = 0;
    gb->ppu.lx = 0;
    gb->ppu.current_mode = 0;
    gb->ppu.mode1_153th = 0;
    gb->ppu.line_dot_count = 0;

    gb->ppu.oam_locked = 0;
    gb->ppu.vram_locked = 0;

    gb->memory.io[IO_OFFSET(STAT)] &= ~0x03;
    check_lyc(gb, 0);

    fetcher_reset(&gb->ppu.bg_fetcher);
    fetcher_reset(&gb->ppu.obj_fetcher);

    lcd_off(gb);
}

// Mode 2
static int oam_scan(struct gb_core *gb)
{
    gb->ppu.oam_locked = 1;
    gb->ppu.vram_locked = 0;

    uint8_t oam_offset = 2 * (gb->ppu.line_dot_count);
    if (gb->ppu.obj_count < 10)
    {
        // 8x16 (LCDC bit 2 = 1) or 8x8 (LCDC bit 2 = 0)
        // TODO: obj_y + 1 != 0 condition is weird ? check this
        int y_max_offset = get_lcdc(gb->memory.io, LCDC_OBJ_SIZE) ? 16 : 8;
        if (gb->memory.oam[OAM_OFFSET(oam_offset + 1)] != 0 &&
            gb->memory.io[IO_OFFSET(LY)] + 16 >= gb->memory.oam[oam_offset] &&
            gb->memory.io[IO_OFFSET(LY)] + 16 < gb->memory.oam[oam_offset] + y_max_offset)
        {
            gb->ppu.obj_slots[gb->ppu.obj_count].y = gb->memory.oam[oam_offset];
            gb->ppu.obj_slots[gb->ppu.obj_count].x = gb->memory.oam[oam_offset + 1];
            gb->ppu.obj_slots[gb->ppu.obj_count].oam_offset = oam_offset;
            gb->ppu.obj_slots[gb->ppu.obj_count].done = 0;
            ++gb->ppu.obj_count;
        }
    }

    gb->ppu.line_dot_count += 2;

    // Switch to mode 3 and reset FIFOs
    if (gb->ppu.line_dot_count >= 80)
    {
        gb->ppu.current_mode = 3;
        RING_BUFFER_INIT(pixel, &gb->ppu.bg_fifo);
        RING_BUFFER_INIT(pixel, &gb->ppu.obj_fifo);
    }

    return 2;
}

static uint8_t mode2_handler(struct gb_core *gb)
{
    if (!gb->ppu.mode2_tick)
    {
        gb->ppu.mode2_tick = 1;
        return 1;
    }

    gb->ppu.mode2_tick = 0;
    if (gb->ppu.line_dot_count == 0)
    {
        set_stat(gb->memory.io, 1);
        clear_stat(gb->memory.io, 0);
        // Check the WY trigger
        if (gb->memory.io[IO_OFFSET(LY)] == gb->memory.io[IO_OFFSET(WY)])
            gb->ppu.wy_trigger = 1;
    }

    check_lyc(gb, 0);

    gb->ppu.first_tile = 1;
    oam_scan(gb);
    return 2;
}

static uint8_t fetchers_step(struct gb_core *gb)
{
    // There is a new object to render
    if (gb->ppu.obj_fetcher.obj_index != -1)
    {
        // If OBJ mode is already enabled, it means either we are fetching the
        // obj_index, or a new object came up overlapping the currently rendering one
        if (!gb->ppu.obj_mode)
        {
            // BG FIFO must not be empty and BG fetcher must have a slice ready to be pushed
            if (!RING_BUFFER_IS_EMPTY(pixel, &gb->ppu.bg_fifo) && gb->ppu.bg_fetcher.current_step == 3)
            {
                // Once we enter OBJ mode, we have at least 8+ BG pixels fetched
                gb->ppu.obj_mode = 1;
            }
        }
    }

    if (gb->ppu.obj_mode)
        obj_fetcher_step(gb);
    else
        bg_fetcher_step(gb);

    return 1;
}

// Send one pixel to the LCD (1 dot)
static uint8_t send_pixel(struct gb_core *gb)
{
    if (RING_BUFFER_IS_EMPTY(pixel, &gb->ppu.bg_fifo))
        return 0;

    // Don't draw BG prefetch + shift SCX for first BG tile
    if (!gb->ppu.win_mode && gb->ppu.first_tile && gb->ppu.lx > 7)
    {
        size_t discard = gb->memory.io[IO_OFFSET(SCX)] % 8;
        if (RING_BUFFER_GET_COUNT(pixel, &gb->ppu.bg_fifo) <= 8 - discard)
        {
            struct pixel p = select_pixel(gb);
            draw_pixel(gb, p);
        }
        else if (!RING_BUFFER_IS_EMPTY(pixel, &gb->ppu.bg_fifo))
        {
            RING_BUFFER_DEQUEUE(pixel, &gb->ppu.bg_fifo, NULL);
            --gb->ppu.lx;
        }
    }
    else if (gb->ppu.lx > 7 && gb->ppu.lx <= 167)
    {
        struct pixel p = select_pixel(gb);
        draw_pixel(gb, p);
    }

    if (gb->ppu.first_tile && RING_BUFFER_IS_EMPTY(pixel, &gb->ppu.bg_fifo))
        gb->ppu.first_tile = 0;

    return 1;
}

static uint8_t mode3_handler(struct gb_core *gb)
{
    // End of mode 3, go to HBlank (mode 0)
    if (gb->ppu.lx > 167)
    {
        // Update WIN internal LY and reset internal LX
        if (gb->ppu.win_mode)
        {
            ++gb->ppu.win_ly;
            gb->ppu.win_lx = 7;
        }
        gb->ppu.current_mode = 0;
        return 0;
    }

    // Start of mode 3
    if (gb->ppu.line_dot_count == 80)
    {
        set_stat(gb->memory.io, 1);
        set_stat(gb->memory.io, 0);

        // Lock OAM and VRAM read (return FF)
        gb->ppu.oam_locked = 1;
        gb->ppu.vram_locked = 1;

        // Reset FIFOs and Fetchers
        RING_BUFFER_INIT(pixel, &gb->ppu.bg_fifo);
        RING_BUFFER_INIT(pixel, &gb->ppu.obj_fifo);

        fetcher_reset(&gb->ppu.bg_fetcher);
        fetcher_reset(&gb->ppu.obj_fetcher);
    }

    // Check if window triggers are fulfilled
    if (!gb->ppu.win_mode && on_window(gb))
    {
        // Reset BG Fetcher and FIFO to current step 0 and enable win mode
        RING_BUFFER_INIT(pixel, &gb->ppu.bg_fifo);
        gb->ppu.bg_fetcher.current_step = 0;
        gb->ppu.bg_fetcher.tick = 0;
        gb->ppu.win_mode = 1;
    }

    // Check for object if we are not already treating one
    if (gb->ppu.obj_fetcher.obj_index == -1 && get_lcdc(gb->memory.io, LCDC_OBJ_ENABLE))
    {
        int obj = -1;
        int bottom_part = 0;
        obj = on_object(gb, &bottom_part);
        if (obj > -1)
        {
            gb->ppu.obj_fetcher.obj_index = obj;
            gb->ppu.obj_fetcher.bottom_part = bottom_part;
        }
    }

    fetchers_step(gb);

    // OBJ mode = 1 and OBJ index = -1 means we just ended fetching an object
    // if so check again for remaining not done object on the same LX and LY
    if (gb->ppu.obj_mode && gb->ppu.obj_fetcher.obj_index == -1)
    {
        gb->ppu.obj_mode = 0;
        // Force a new iteration of the loop in case of another object
        if (on_object(gb, NULL) > -1)
            return 0;
    }

    // If we are in OBJ mode, we are fetching an object (FIFOs stall)
    if (gb->ppu.obj_fetcher.obj_index == -1 && !gb->ppu.obj_mode)
    {
        // Prefetch pixels must be discarded
        // wait for the BG FIFO to be filled before popping
        if (gb->ppu.lx < 8)
        {
            if (!RING_BUFFER_IS_EMPTY(pixel, &gb->ppu.bg_fifo))
            {
                // Pop current pixel and discard it
                select_pixel(gb);
                ++gb->ppu.lx;
            }
        }
        else
        {
            // Attempt to draw current pixel on the LCD
            if (send_pixel(gb))
                ++gb->ppu.lx;
        }
    }

    ++gb->ppu.line_dot_count;
    return 1;
}

// Mode 0
static uint8_t mode0_handler(struct gb_core *gb)
{
    if (get_stat(gb->memory.io, 1) || get_stat(gb->memory.io, 0))
    {
        clear_stat(gb->memory.io, 1);
        clear_stat(gb->memory.io, 0);
        if (get_stat(gb->memory.io, 3) && !get_if(gb, INTERRUPT_LCD))
            set_if(gb, INTERRUPT_LCD);
    }

    gb->ppu.obj_fetcher.obj_index = -1;
    gb->ppu.oam_locked = 0;
    gb->ppu.vram_locked = 0;
    // TODO verify dma lock
    if (gb->ppu.line_dot_count < 456)
    {
        ++gb->ppu.line_dot_count;
        return 1;
    }

    // Exit HBlank
    gb->ppu.lx = 0;
    gb->memory.io[IO_OFFSET(LY)] += 1;

    gb->ppu.win_mode = 0;

    gb->ppu.line_dot_count = 0;
    gb->ppu.current_mode = 2;
    gb->ppu.obj_count = 0;

    set_stat(gb->memory.io, 1);
    clear_stat(gb->memory.io, 0);
    if (get_stat(gb->memory.io, 5) && !get_stat(gb->memory.io, 4))
        set_if(gb, INTERRUPT_LCD);

    // Start VBlank
    if (gb->memory.io[IO_OFFSET(LY)] > 143)
    {
        gb->ppu.wy_trigger = 0;
        gb->ppu.current_mode = 1;
        gb->ppu.mode1_153th = 0;
    }

    return 0;
}

static uint8_t mode1_handler(struct gb_core *gb)
{
    if (gb->memory.io[IO_OFFSET(LY)] == 144 && gb->ppu.line_dot_count == 0)
    {
        clear_stat(gb->memory.io, 1);
        set_stat(gb->memory.io, 0);
        set_if(gb, INTERRUPT_VBLANK);                                  // VBlank Interrupt
        if (get_stat(gb->memory.io, 4) && !get_stat(gb->memory.io, 3)) // STAT VBlank Interrupt
            set_if(gb, INTERRUPT_LCD);
    }

    check_lyc(gb, gb->ppu.mode1_153th);

    if (gb->ppu.line_dot_count < 456)
    {
        // LY = 153, special case with LY = 0 after 1 MCycle
        if (gb->memory.io[IO_OFFSET(LY)] == 153 && !gb->ppu.mode1_153th && gb->ppu.line_dot_count >= 4)
        {
            gb->memory.io[IO_OFFSET(LY)] = 0;
            gb->ppu.mode1_153th = 1;
        }
        ++gb->ppu.line_dot_count;
        return 1;
    }

    if (!gb->ppu.mode1_153th && gb->memory.io[IO_OFFSET(LY)]) // Go to next VBlank line
    {
        ++gb->memory.io[IO_OFFSET(LY)];
        gb->ppu.line_dot_count = 0;
        return 0;
    }

    // Else exit VBlank, enter OAM Scan
    gb->ppu.line_dot_count = 0;
    gb->ppu.mode1_153th = 0;
    gb->ppu.current_mode = 2;
    gb->memory.io[IO_OFFSET(LY)] = 0;
    gb->ppu.win_ly = 0;
    gb->ppu.win_lx = 7;
    return 0;
}

void dma_handle(struct gb_core *gb)
{
    uint8_t dequeue = 0;
    for (size_t i = 0; i < RING_BUFFER_GET_COUNT(dma_request, &gb->ppu.dma_requests); ++i)
    {
        struct dma_request *req = gb->ppu.dma_requests.buffer + ((gb->ppu.dma_requests.head + i) % 3);
        switch (req->status)
        {
        case DMA_REQUESTED:
            --req->status;
            break;
        case DMA_SETUP:
            --req->status;
            gb->ppu.dma = 1;
            gb->ppu.dma_acc = 0;
            if (i > 0)
                dequeue = 1; /* This DMA request overrides the currently active one */
            break;
        case DMA_ACTIVE:
            gb->memory.oam[gb->ppu.dma_acc] = read_mem(gb, (req->source << 8) + gb->ppu.dma_acc);
            ++gb->ppu.dma_acc;
            if (gb->ppu.dma_acc >= 160)
            {
                gb->ppu.dma = 0;
                gb->ppu.dma_acc = 0;
                dequeue = 1;
            }
            break;
        }
    }

    if (dequeue)
        RING_BUFFER_DEQUEUE(dma_request, &gb->ppu.dma_requests, NULL);
}

void ppu_tick(struct gb_core *gb)
{
    if (!get_lcdc(gb->memory.io, LCDC_LCD_PPU_ENABLE))
        return;

    uint8_t dot = 0;
    while (!dot)
    {
        switch (gb->ppu.current_mode)
        {
        case 2:
            dot += mode2_handler(gb);
            break;
        case 3:
            dot += mode3_handler(gb);
            break;
        case 0:
            dot += mode0_handler(gb);
            break;
        case 1:
            dot += mode1_handler(gb);
            break;
        }
    }
}

void serialize_ppu_to_stream(FILE *stream, struct ppu *ppu)
{
    fwrite(&ppu->mode2_tick, sizeof(uint8_t), 1, stream);
    fwrite(&ppu->lx, sizeof(uint8_t), 1, stream);

    for (size_t i = 0; i < sizeof(ppu->obj_slots) / sizeof(struct obj); ++i)
    {
        fwrite(&ppu->obj_slots[i].x, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->obj_slots[i].y, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->obj_slots[i].oam_offset, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->obj_slots[i].x, sizeof(uint8_t), 1, stream);
    }

    fwrite(&ppu->obj_count, sizeof(uint8_t), 1, stream);
    fwrite(&ppu->obj_count, sizeof(uint8_t), 1, stream);

    fwrite_le_64(stream, ppu->bg_fifo.element_count);
    for (size_t i = 0; i < ppu->bg_fifo.element_count; ++i)
    {
        fwrite(&ppu->bg_fifo.buffer[i].obj, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->bg_fifo.buffer[i].color, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->bg_fifo.buffer[i].palette, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->bg_fifo.buffer[i].priority, sizeof(uint8_t), 1, stream);
    }
    fwrite_le_64(stream, ppu->bg_fifo.head);
    fwrite_le_64(stream, ppu->bg_fifo.tail);

    fwrite_le_64(stream, ppu->obj_fifo.element_count);
    for (size_t i = 0; i < ppu->obj_fifo.element_count; ++i)
    {
        fwrite(&ppu->obj_fifo.buffer[i].obj, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->obj_fifo.buffer[i].color, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->obj_fifo.buffer[i].palette, sizeof(uint8_t), 1, stream);
        fwrite(&ppu->obj_fifo.buffer[i].priority, sizeof(uint8_t), 1, stream);
    }
    fwrite_le_64(stream, ppu->obj_fifo.head);
    fwrite_le_64(stream, ppu->obj_fifo.tail);
}

void load_ppu_from_stream(FILE *stream, struct cpu *cpu)
{
    fread(&cpu->a, sizeof(uint8_t), 8, stream);

    fread_le_16(stream, &cpu->sp);
    fread_le_16(stream, &cpu->pc);

    fread(&cpu->ime, sizeof(uint8_t), 1, stream);
}
