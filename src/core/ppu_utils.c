#include "ppu_utils.h"

#include <stdlib.h>

#include "gb_core.h"
#include "interrupts.h"
#include "ring_buffer.h"

// Pixel and slice utils
struct pixel make_pixel(uint8_t hi, uint8_t lo, int i, uint8_t *attributes, int obj_i)
{
    struct pixel res;
    uint8_t hi_bit = (hi >> (7 - i)) & 0x01;
    uint8_t lo_bit = (lo >> (7 - i)) & 0x01;
    res.color = (hi_bit << 1) | lo_bit;
    res.obj = -1;
    if (attributes != NULL)
    {
        res.palette = (*attributes >> 4) & 0x01;
        res.priority = (*attributes >> 7) & 0x01;
        res.obj = obj_i;
    }
    return res;
}

uint8_t slice_xflip(uint8_t slice)
{
    uint8_t res = 0x00;
    res |= ((slice << 7) & 0x80);
    res |= ((slice << 5) & 0x40);
    res |= ((slice << 3) & 0x20);
    res |= ((slice << 1) & 0x10);
    res |= ((slice >> 1) & 0x08);
    res |= ((slice >> 3) & 0x04);
    res |= ((slice >> 5) & 0x02);
    res |= ((slice >> 7) & 0x01);
    return res;
}

int on_window(struct gb_core *gb)
{
    return get_lcdc(gb->membus, LCDC_BG_WINDOW_ENABLE) && get_lcdc(gb->membus, LCDC_WINDOW_ENABLE) &&
           gb->ppu.wy_trigger && gb->ppu.lx == gb->memory.io[IO_OFFSET(WX)] + 1;
}

int on_object(struct gb_core *gb, int *bottom_part)
{
    for (int i = 0; i < gb->ppu.obj_count; ++i)
    {
        // Ignore already fetched objects
        if (gb->ppu.obj_slots[i].done)
            continue;

        // 8x16 (LCDC bit 2 = 1) or 8x8 (LCDC bit 2 = 0)
        int y_max_offset = get_lcdc(gb->membus, LCDC_OBJ_SIZE) ? 16 : 8;
        if (gb->ppu.obj_slots[i].x == gb->ppu.lx && gb->membus[LY] + 16 >= gb->ppu.obj_slots[i].y &&
            gb->membus[LY] + 16 < gb->ppu.obj_slots[i].y + y_max_offset)
        {
            if (bottom_part != NULL && y_max_offset == 16 && gb->membus[LY] + 16 >= gb->ppu.obj_slots[i].y + 8)
                *bottom_part = 1;
            return i;
        }
    }
    return -1;
}

struct pixel select_pixel(struct gb_core *gb)
{
    struct pixel bg_p;
    RING_BUFFER_DEQUEUE(pixel, &gb->ppu.bg_fifo, &bg_p);

    if (RING_BUFFER_IS_EMPTY(pixel, &gb->ppu.obj_fifo))
        return bg_p;

    struct pixel obj_p;
    RING_BUFFER_DEQUEUE(pixel, &gb->ppu.obj_fifo, &obj_p);

    if (!get_lcdc(gb->membus, LCDC_BG_WINDOW_ENABLE))
        return obj_p;
    if (!get_lcdc(gb->membus, LCDC_OBJ_ENABLE))
        return bg_p;
    if (obj_p.priority && bg_p.color != 0)
        return bg_p;
    if (obj_p.color == 0)
        return bg_p;
    return obj_p;
}

int push_slice(struct gb_core *gb, RING_BUFFER(pixel) * q, uint8_t hi, uint8_t lo, int obj_i)
{
    uint8_t *attributes = NULL;
    if (obj_i != -1)
        attributes = &gb->ppu.obj_fetcher.attributes;
    for (int i = 0; i < 8; ++i)
    {
        struct pixel p = make_pixel(hi, lo, i, attributes, obj_i);
        // TODO verify this
        if (!get_lcdc(gb->membus, LCDC_BG_WINDOW_ENABLE))
            p.color = 0;
        RING_BUFFER_ENQUEUE(pixel, q, &p);
    }
    return 2;
}

int merge_obj(struct gb_core *gb, uint8_t hi, uint8_t lo, int obj_i)
{
    uint8_t attributes = gb->ppu.obj_fetcher.attributes;

    // Merge the pending pixels in the OBJ FIFO
    size_t i = 0;
    for (; i < RING_BUFFER_GET_COUNT(pixel, &gb->ppu.obj_fifo); ++i)
    {
        struct pixel *q = gb->ppu.obj_fifo.buffer + ((gb->ppu.obj_fifo.head + i) % 8);
        int same_x = gb->ppu.obj_slots[q->obj].x == gb->ppu.obj_slots[obj_i].x;
        // Replace only transparent pixels of object already pending
        struct pixel p = make_pixel(hi, lo, i, &attributes, obj_i);
        if (q->color == 0)
            *q = p;
        else if (same_x && p.color != 0)
        {
            // If both objects have the same X, use OAM index to choose pixel
            if (obj_i < q->obj)
                *q = p;
        }
    }

    // Add the remaining pixels that don't need merging in the FIFO
    for (; i < 8; ++i)
    {
        struct pixel p = make_pixel(hi, lo, i, &attributes, obj_i);
        RING_BUFFER_ENQUEUE(pixel, &gb->ppu.obj_fifo, &p);
    }
    return 2;
}

void check_lyc(struct gb_core *gb, int line_153)
{
    if (gb->membus[LY] == gb->membus[LYC])
    {
        set_stat(gb->membus, STAT_LYC_EQUAL_LY);
        if (line_153 && gb->ppu.line_dot_count == 12 && get_stat(gb->membus, 6))
            set_if(gb, INTERRUPT_LCD);
        else if (gb->ppu.line_dot_count == 4 && get_stat(gb->membus, 6)) //&& !get_if(ppu->cpu, INTERRUPT_LCD))
            set_if(gb, INTERRUPT_LCD);
    }
    else
        clear_stat(gb->membus, 2);
}
