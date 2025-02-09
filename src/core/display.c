#include <stdlib.h>

#include "common.h"
#include "cpu.h"
#include "emulation.h"
#include "ppu.h"
#include "ppu_utils.h"
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
    return &frame_buffer;
}

void draw_pixel(struct cpu *cpu, struct pixel p)
{
    struct renderer *rend = cpu->ppu->renderer;

    uint8_t *c_regist = p.obj > -1 ? (p.palette ? cpu->ppu->obp1 : cpu->ppu->obp0) : cpu->ppu->bgp;

    unsigned int color_index = (*c_regist >> (p.color * 2)) & 0x03;

    struct pixel_data pixel = {
        ._unused = 0,
        .values = color_palette[color_index],
    };

    frame_buffer[*cpu->ppu->ly * SCREEN_WIDTH + (cpu->ppu->lx - 8)] = pixel;

    // uint32_t pixel = SDL_MapRGB(rend->format, NULL, color->r, color->g, color->b);
    //  A whole frame is ready, render it, handle inputs, synchronize
    if (*cpu->ppu->ly == SCREEN_HEIGHT - 1 && cpu->ppu->lx == SCREEN_WIDTH + 7)
    {
        handle_events(cpu);
        // render_frame(rend); TODO: callback render frame
        synchronize(cpu);
    }
}

void lcd_off(void)
{
    for (size_t i = 0; i < SCREEN_RESOLUTION; ++i)
    {
        struct pixel_data pixel = {
            ._unused = 0,
            .values = color_palette[4],
        };
        frame_buffer[i] = pixel;
    };
    // render_frame(rend); TODO: callback render frame
}
