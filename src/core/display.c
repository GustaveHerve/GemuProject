#include "display.h"

#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include "emulation.h"
#include "gb_core.h"
#include "sync.h"

struct pixel_data
{
    uint8_t _unused; /* padding */
    struct color values;
};

#define DEFAULT_PALETTE                                                                                                \
    {                                                                                                                  \
        COLOR_FROM_HEX(0xe0f8d0),                                                                                      \
        COLOR_FROM_HEX(0x88c070),                                                                                      \
        COLOR_FROM_HEX(0x346856),                                                                                      \
        COLOR_FROM_HEX(0x081820),                                                                                      \
        COLOR_FROM_HEX(0xe5f5da),                                                                                      \
    }

static const struct color default_palette[5] = DEFAULT_PALETTE;

static struct color color_palette[5] = DEFAULT_PALETTE;

static struct pixel_data frame_buffer[SCREEN_RESOLUTION];

void *get_frame_buffer(void)
{
    return frame_buffer;
}

void draw_pixel(struct gb_core *gb, struct pixel p)
{
    unsigned int palette_address = p.obj > -1 ? (p.palette ? OBP1 : OBP0) : BGP;
    unsigned int color_index = (gb->memory.io[IO_OFFSET(palette_address)] >> (p.color * 2)) & 0x03;

    frame_buffer[gb->memory.io[IO_OFFSET(LY)] * SCREEN_WIDTH + (gb->ppu.lx - 8)].values = color_palette[color_index];

    /*  A whole frame is ready, render it, handle inputs, synchronize */
    if (gb->memory.io[IO_OFFSET(LY)] == SCREEN_HEIGHT - 1 && gb->ppu.lx == SCREEN_WIDTH + 7)
        gb->callbacks.frame_ready();
}

void lcd_off(struct gb_core *gb)
{
    for (size_t i = 0; i < SCREEN_RESOLUTION; ++i)
    {
        frame_buffer[i].values = color_palette[4];
    };
    gb->callbacks.frame_ready();
}

void reset_palette(void)
{
    for (size_t i = 0; i < 5; ++i)
    {
        color_palette[i] = default_palette[i];
    }
}

struct color get_color_index(unsigned int index)
{
    assert(index < 5);
    return color_palette[index];
}

void set_color_index(struct color new_color, unsigned int index)
{
    assert(index < 5);
    color_palette[index] = new_color;
}
