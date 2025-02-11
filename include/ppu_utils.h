#ifndef CORE_PPU_UTILS_H
#define CORE_PPU_UTILS_H

#include <stdint.h>

#include "ring_buffer.h"

struct gb_core;
struct ppu;

typedef struct pixel
{
    int obj;
    uint8_t color;
    uint8_t palette;
    uint8_t priority;
} pixel;

struct obj
{
    uint8_t y;
    uint8_t x;
    uint8_t *oam_address;
    uint8_t done;
};

DEFINE_RING_BUFFER(pixel, 8)

// on_window: read LX and LY and check if drawing in Window or BG
int on_window(struct gb_core *gb);

// on_object: reads the 10 OAM slots and checks if we need to draw an object at current LX LY
// returns object index in obj_slots, -1 if no object
int on_object(struct gb_core *gb, int *bottom_part);

struct pixel select_pixel(struct gb_core *gb);

int push_slice(struct gb_core *gb, RING_BUFFER(pixel) * q, uint8_t hi, uint8_t lo, int obj_i);

uint8_t slice_xflip(uint8_t slice);

// OBJ Merge version of push_slice in case it is not empty, overwrite transparent pixels OBJ FIFO
int merge_obj(struct gb_core *gb, uint8_t hi, uint8_t lo, int obj_i);

void check_lyc(struct gb_core *gb, int line_153);

#endif
