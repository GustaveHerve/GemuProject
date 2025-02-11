#include <stdlib.h>

#include "common.h"
#include "emulation.h"
#include "gb_core.h"
#include "sync.h"

struct pixel_data
{
    Uint8 _unused;
    Uint8 r;
    Uint8 g;
    Uint8 b;
};

static struct pixel_data color_palette[5] = {
    {0, 224, 248, 208}, {0, 136, 192, 112}, {0, 52, 104, 86}, {0, 8, 24, 32}, {0, 229, 245, 218}};

static struct pixel_data frame_buffer[SCREEN_RESOLUTION] = {0};

void *get_frame_buffer(void)
{
    return frame_buffer;
}

void draw_pixel(struct gb_core *gb, struct pixel p)
{
    unsigned int palette_address = p.obj > -1 ? (p.palette ? OBP1 : OBP0) : BGP;
    unsigned int color_index = (gb->membus[palette_address] >> (p.color * 2)) & 0x03;

    frame_buffer[gb->membus[LY] * SCREEN_WIDTH + (gb->ppu.lx - 8)] = color_palette[color_index];

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
        struct pixel_data pixel = color_palette[4];
        frame_buffer[i] = pixel;
    };
    gb->callbacks.render_frame();
}
