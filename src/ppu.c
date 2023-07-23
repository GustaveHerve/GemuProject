#include <stdlib.h>
#include <err.h>
#include "cpu.h"
#include "utils.h"
#include "ppu.h"
#include "ppu_utils.h"
#include "emulation.h"
#include "queue.h"

void ppu_init(struct ppu *ppu, struct cpu *cpu)
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

    ppu->bg_fifo = queue_init();
    ppu->obj_fifo = queue_init();

    ppu->bg_fetcher = malloc(sizeof(fetcher));
    ppu->obj_fetcher = malloc(sizeof(fetcher));

    fetcher_init(ppu, ppu->bg_fetcher, 0);
    fetcher_init(ppu, ppu->obj_fetcher, 1);
}

void ppu_free(struct ppu *ppu)
{
    free(ppu->bg_fetcher);
    free(ppu->obj_fetcher);
    queue_free(ppu->bg_fifo);
    queue_free(ppu->obj_fifo);
    free(ppu);
}

//Tick 4 dots
void ppu_tick_m(struct ppu *ppu)
{
    int dots = 4;
    while (dots > 0)
    {
        switch (ppu->current_mode)
        {
            case 2: //Mode 2 - OAM scan
            {
                oam_scan_m(ppu);
                dots -= 4;
                break;
            }
            case 3: //Mode 3 - Drawing pixels and FIFOs fetcher activity
            {
                //BG/Win Fetcher
                if (in_window(ppu)) // Window mode -> clear BG FIFO, reset f
                {
                    ppu->win_mode = 1;
                    queue_clear(ppu->bg_fifo);
                    ppu->f
                }
                fetcher_step()
                break;
            }
            case 0: //Mode 0 - HBlank
            {
                break;
            }
            case 1: //Mode 1 - VBlank
            {
                break;
            }
        }
    }
}

//Mode 2
int oam_scan(struct ppu *ppu)
{
    ppu->oam_locked = 1;
    ppu->vram_locked = 0;

    uint8_t *obj_y = ppu->oam + 2 * (ppu->line_dot_count);
    if (ppu->obj_count < 10)
    {
        if (*(obj_y + 1) != 0 && *ppu->ly + 16 >= *obj_y
                && *ppu->ly + 16 < *obj_y + 8)
        {
            ppu->obj_slots[ppu->obj_count].y = *obj_y;
            ppu->obj_slots[ppu->obj_count].x = *(obj_y + 1);
            ppu->obj_slots[ppu->obj_count].oam_address = obj_y;
            ppu->obj_count++;
        }
    }

    ppu->frame_dot_count += 2;
    ppu->line_dot_count += 2;
    if (ppu->line_dot_count >= 80)
        ppu->current_mode = 3;

    return 2;
}

int oam_scan_m(struct ppu *ppu)
{
    oam_scan(ppu);
    oam_scan(ppu);
    return 4;
}

//obj_index == -1 means BG/Win Mode
uint8_t get_tileid(struct ppu *ppu, int obj_index)
{
    uint8_t tileid = 0;
    if (obj_index == -1)
    {
        //Decide if BG or Window Mode
        if (!get_lcdc(ppu, 0))
        {
            //TODO BG and Window disabled -> blank pixels
            return 0;
        }
        else if (!get_lcdc(ppu, 5))
            ppu->win_mode = 0;
        /*
        else
        {
            if (in_window(ppu))
                ppu->win_mode = 1;
            else
                ppu->win_mode = 0;
        }
        */

        uint8_t x_part = 0;
        uint8_t y_part = 0;
        int bit = 0;
        if (ppu->win_mode)
        {
            x_part = ppu->lx / 8;
            y_part = *ppu->wy / 8;
            bit = 6;
        }
        else
        {
            x_part =  ((uint8_t) (ppu->lx + *ppu->scx)) / 8;
            y_part =  ((uint8_t) (*ppu->ly + *ppu->scy)) / 8;
            bit = 3;
        }

        uint16_t address = (0x13 << 11) | (get_lcdc(ppu, bit) << 10)
            | (y_part << 5) | x_part;
        tileid = ppu->cpu->membus[address];
    }

    else
    {
        if (ppu->dma_oam_locked)
            tileid = 0xFF;
        else
            tileid = *ppu->obj_slots[obj_index].oam_address;
    }

    return tileid;
}

uint8_t get_tile_lo(struct ppu *ppu, uint8_t tileid, int obj_index)
{
    uint8_t y_part = 0;
    int bit_12 = 0;
    if (obj_index != -1)
        y_part = (*ppu->ly - ppu->obj_slots[obj_index].y) % 8;
    else if (ppu->win_mode)
    {
        y_part = *ppu->wy % 8;
        bit_12 = !(get_lcdc(ppu, 4) | (tileid & 0x80));
    }
    else
    {
        y_part = (uint8_t)(((*ppu->ly + *ppu->scy)) % 8);
        bit_12 = !(get_lcdc(ppu, 4) | (tileid & 0x80));
    }

    uint16_t address_low = (0x4 << 13) | (bit_12 << 12) |
        (tileid << 4) | (y_part << 1) | 0;

    uint8_t slice_low = ppu->cpu->membus[address_low];

    return slice_low;
}

uint8_t get_tile_hi(struct ppu *ppu, uint8_t tileid, int obj_index)
{
    //Get tile low
    uint8_t y_part = 0;
    int bit_12 = 0;
    if (obj_index != -1)
        y_part = (*ppu->ly - ppu->obj_slots[obj_index].y) % 8;
    else if (ppu->win_mode)
    {
        y_part = *ppu->wy % 8;
        bit_12 = !(get_lcdc(ppu, 4) | (tileid & 0x80));
    }
    else
    {
        y_part = (uint8_t)(((*ppu->ly + *ppu->scy)) % 8);
        bit_12 = !(get_lcdc(ppu, 4) | (tileid & 0x80));
    }

    uint16_t address_high = (0x4 << 13) | (bit_12 << 12) |
        (tileid << 4) | (y_part << 1) | 1;

    uint8_t slice_high = ppu->cpu->membus[address_high];

    return slice_high;
}

int push_pixel(queue *target, struct pixel p)
{
    queue_push(target, p);
    return 1;
}

int push_slice(struct ppu *ppu, queue *q, uint8_t hi, uint8_t lo, int obj_i)
{
    uint8_t *attributes = NULL;
    if (obj_i != -1)
        attributes = ppu->obj_slots[obj_i].oam_address + 2;
    for (int i = 0; i < 8; i++)
    {
        struct pixel p = make_pixel(hi, lo, i, attributes) ;
        push_pixel(q, p);
    }
    return 2;
}

//Mode 0 Horizontal Blank
//PPU sleeps for time dots
int hblank(struct ppu *ppu, int prev_time)
{
    int time = 456 - prev_time;
    ppu->oam_locked = 0;
    ppu->vram_locked = 0;
    for (int i = 0; i < time; i++)
        tick(ppu->cpu);
    return time;
}

int get_lcdc(struct ppu *ppu, int bit)
{
    uint8_t lcdc = *ppu->lcdc;
    return (lcdc >> bit & 0x01);
}

//in_window: read LX and LY and check if drawing in Window or BG
int in_window(struct ppu *ppu)
{
    return *ppu->ly >= *ppu->wy && ppu->lx >= *ppu->wx;
}

//in_object: read LX and LY and check if drawing a selected object
//returns object index in obj_slots, -1 if no object
int in_object(struct ppu *ppu)
{
    //TODO handle 8x16 object mode
    for (int i = 0; i < ppu->obj_count; i++)
    {
        if (ppu->obj_slots[i].x == ppu->lx + 8 &&
            *ppu->ly + 16 >= ppu->obj_slots[i].y &&
            *ppu->ly + 16 < ppu->obj_slots[i].y + 8)
            return i;
    }
    return -1;
}

void fetcher_init(struct ppu *ppu, fetcher *f, uint8_t obj)
{
    f->current_step = 0;
    f->hi = 0;
    f->lo = 0;
    f->tileid = 0;
    if (obj)
    {
        f->target = ppu->obj_fifo;
        f->obj_fetcher = 1;
    }
    else
    {
        f->target = ppu->bg_fifo;
        f->obj_fetcher = 0;
    }
}

//Does one fetcher step (2 dots)
int fetcher_step(fetcher *f, struct ppu *ppu, int obj_index)
{
    //BG/Win fetcher
    if (!f->obj_fetcher)
    {
        switch (f->current_step)
        {
            case 0:
                f->tileid = get_tileid(ppu, -1);
                break;
            case 1:
                f->lo = get_tile_lo(ppu, f->tileid, -1);
                break;
            case 2:
                f->hi = get_tile_hi(ppu, f->tileid, -1);
                break;
            case 3:
                {
                    if (queue_isempty(ppu->bg_fifo))
                        push_slice(ppu, ppu->bg_fifo, f->hi, f->lo, NULL);
                    break;
                }
        }
    }
    //OBJ fetcher
    else
    {
        switch (f->current_step)
        {
            case 0:
                f->tileid = get_tileid(ppu, -1);
                break;
            case 1:
                f->lo = get_tile_lo(ppu, f->tileid, -1);
                break;
            case 2:
                f->hi = get_tile_hi(ppu, f->tileid, -1);
                break;
            case 3:
                {
                    //TODO implement merging if OBJ FIFO not empty
                    queue_clear(ppu->obj_fifo);
                    push_slice(ppu, ppu->obj_fifo, f->hi, f->lo, obj_index);
                    break;
                }
        }
    }

    return 2;
}
