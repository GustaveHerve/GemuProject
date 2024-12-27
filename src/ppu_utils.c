#include "ppu_utils.h"

#include <stdlib.h>

#include "cpu.h"
#include "interrupts.h"
#include "ppu.h"
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

int on_window(struct ppu *ppu)
{
    return get_lcdc(ppu, LCDC_BG_WINDOW_ENABLE) && get_lcdc(ppu, LCDC_WINDOW_ENABLE) && ppu->wy_trigger &&
           ppu->lx == *ppu->wx + 1;
}

int on_object(struct ppu *ppu, int *bottom_part)
{
    for (int i = 0; i < ppu->obj_count; ++i)
    {
        // Ignore already fetched objects
        if (ppu->obj_slots[i].done)
            continue;

        // 8x16 (LCDC bit 2 = 1) or 8x8 (LCDC bit 2 = 0)
        int y_max_offset = get_lcdc(ppu, LCDC_OBJ_SIZE) ? 16 : 8;
        if (ppu->obj_slots[i].x == ppu->lx && *ppu->ly + 16 >= ppu->obj_slots[i].y &&
            *ppu->ly + 16 < ppu->obj_slots[i].y + y_max_offset)
        {
            if (bottom_part != NULL && y_max_offset == 16 && *ppu->ly + 16 >= ppu->obj_slots[i].y + 8)
                *bottom_part = 1;
            return i;
        }
    }
    return -1;
}

struct pixel select_pixel(struct ppu *ppu)
{
    struct pixel bg_p;
    RING_BUFFER_DEQUEUE(pixel, &ppu->bg_fifo, &bg_p);

    if (RING_BUFFER_IS_EMPTY(pixel, &ppu->obj_fifo))
        return bg_p;

    struct pixel obj_p;
    RING_BUFFER_DEQUEUE(pixel, &ppu->obj_fifo, &obj_p);

    if (!get_lcdc(ppu, LCDC_BG_WINDOW_ENABLE))
        return obj_p;
    if (!get_lcdc(ppu, LCDC_OBJ_ENABLE))
        return bg_p;
    if (obj_p.priority && bg_p.color != 0)
        return bg_p;
    if (obj_p.color == 0)
        return bg_p;
    return obj_p;
}

int push_slice(struct ppu *ppu, RING_BUFFER(pixel) * q, uint8_t hi, uint8_t lo, int obj_i)
{
    uint8_t *attributes = NULL;
    if (obj_i != -1)
        attributes = &ppu->obj_fetcher->attributes;
    for (int i = 0; i < 8; ++i)
    {
        struct pixel p = make_pixel(hi, lo, i, attributes, obj_i);
        // TODO verify this
        if (!get_lcdc(ppu, LCDC_BG_WINDOW_ENABLE))
            p.color = 0;
        RING_BUFFER_ENQUEUE(pixel, q, &p);
    }
    return 2;
}

int merge_obj(struct ppu *ppu, uint8_t hi, uint8_t lo, int obj_i)
{
    uint8_t attributes = ppu->obj_fetcher->attributes;
    struct pixel q;
    RING_BUFFER_GET_FRONT(pixel, &ppu->obj_fifo, &q);

    // Merge the pending pixels in the OBJ FIFO
    size_t i = 0;
    for (; i < RING_BUFFER_GET_COUNT(pixel, &ppu->obj_fifo); ++i)
    {
        struct pixel *q = ppu->obj_fifo.buffer + ((ppu->obj_fifo.head + i) % 8);
        int same_x = ppu->obj_slots[q->obj].x == ppu->obj_slots[obj_i].x;
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
        RING_BUFFER_ENQUEUE(pixel, &ppu->obj_fifo, &p);
    }
    return 2;
}

void check_lyc(struct ppu *ppu, int line_153)
{
    if (*ppu->ly == *ppu->lyc)
    {
        set_stat(ppu, STAT_LYC_EQUAL_LY);
        if (line_153 && ppu->line_dot_count == 12 && get_stat(ppu, 6))
            set_if(ppu->cpu, INTERRUPT_LCD);
        else if (ppu->line_dot_count == 4 && get_stat(ppu, 6)) //&& !get_if(ppu->cpu, INTERRUPT_LCD))
            set_if(ppu->cpu, INTERRUPT_LCD);
    }
    else
        clear_stat(ppu, 2);
}
