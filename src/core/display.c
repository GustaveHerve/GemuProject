#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include "emulation.h"
#include "gb_core.h"
#include "sync.h"

struct color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct pixel_data
{
    uint8_t _unused;
    struct color values;
};

static struct color color_palette[5] = {{224, 248, 208}, {136, 192, 112}, {52, 104, 86}, {8, 24, 32}, {229, 245, 218}};

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
    {
        // gb->callbacks.handle_events(gb);
        // gb->callbacks.render_frame();
        gb->callbacks.frame_ready();
        // synchronize(gb); /* Tying sync to PPU output seems like a bad idea, what if PPU is off? */
    }
}

void lcd_off(struct gb_core *gb)
{
    for (size_t i = 0; i < SCREEN_RESOLUTION; ++i)
    {
        frame_buffer[i].values = color_palette[4];
    };
    // gb->callbacks.render_frame();
    gb->callbacks.frame_ready();
}
