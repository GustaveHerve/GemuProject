#include <stdlib.h>

#include "common.h"
#include "emulation.h"
#include "gb_core.h"
#include "sync.h"

struct color
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
};

struct pixel_data
{
    Uint8 _unused;
    struct color values;
};

static struct color color_palette[5] = {{224, 248, 208}, {136, 192, 112}, {52, 104, 86}, {8, 24, 32}, {229, 245, 218}};

static struct pixel_data frame_buffer[SCREEN_RESOLUTION] = {0};

void *get_frame_buffer(void)
{
    return frame_buffer;
}

void draw_pixel(struct gb_core *gb, struct pixel p)
{
    unsigned int palette_address = p.obj > -1 ? (p.palette ? OBP1 : OBP0) : BGP;
    unsigned int color_index = (gb->membus[palette_address] >> (p.color * 2)) & 0x03;

    frame_buffer[gb->membus[LY] * SCREEN_WIDTH + (gb->ppu.lx - 8)] =
        (struct pixel_data){._unused = 0, .values = color_palette[color_index]};

    // uint32_t pixel = SDL_MapRGB(rend->format, NULL, color->r, color->g, color->b);
    //  A whole frame is ready, render it, handle inputs, synchronize
    if (gb->membus[LY] == SCREEN_HEIGHT - 1 && gb->ppu.lx == SCREEN_WIDTH + 7)
    {
        gb->callbacks.handle_events(gb);
        gb->callbacks.render_frame();
        synchronize(gb);
    }
}

void lcd_off(struct gb_core *gb)
{
    for (size_t i = 0; i < SCREEN_RESOLUTION; ++i)
    {
        struct pixel_data pixel = {
            ._unused = 0,
            .values = color_palette[4],
        };
        frame_buffer[i] = pixel;
    };
    gb->callbacks.render_frame();
}
