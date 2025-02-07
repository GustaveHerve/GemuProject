#include <stdlib.h>

#include "cpu.h"
#include "emulation.h"
#include "ppu.h"
#include "ppu_utils.h"
#include "sync.h"

#define WIDTH 160
#define HEIGHT 144
#define SCREEN_RESOLUTION (WIDTH * HEIGHT)

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

static struct color color_palette[4] = {{224, 248, 208}, {136, 192, 112}, {52, 104, 86}, {8, 24, 32}};

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

    frame_buffer[*cpu->ppu->ly * WIDTH + (cpu->ppu->lx - 8)] = {
        ._unused = 0,
        .values = color_palette[color_index],
    };

    // uint32_t pixel = SDL_MapRGB(rend->format, NULL, color->r, color->g, color->b);
    //  A whole frame is ready, render it, handle inputs, synchronize
    if (*cpu->ppu->ly == HEIGHT - 1 && cpu->ppu->lx == WIDTH + 7)
    {
        handle_events(cpu);
        render_frame(rend);
        synchronize(cpu);
    }
}

void lcd_off(struct cpu *cpu)
{
    struct renderer *rend = cpu->ppu->renderer;
    uint32_t color = SDL_MapRGB(rend->format, NULL, 229, 245, 218);

    for (size_t i = 0; i < SCREEN_RESOLUTION; ++i)
    {
        frame_buffer[i] = color;
    }

    render_frame(rend);
}
