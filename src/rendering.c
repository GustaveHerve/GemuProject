#include <stdlib.h>

#include "cpu.h"
#include "emulation.h"
#include "ppu.h"
#include "ppu_utils.h"
#include "sync.h"

#define WIDTH 160
#define HEIGHT 144
#define SCREEN_RESOLUTION (WIDTH * HEIGHT)

static uint32_t frame_buffer[SCREEN_RESOLUTION] = {0};

struct color
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
};

static struct color color_palette[4] = {{224, 248, 208}, {136, 192, 112}, {52, 104, 86}, {8, 24, 32}};

void draw_pixel(struct cpu *cpu, struct pixel p)
{
    struct renderer *rend = cpu->ppu->renderer;

    uint8_t *c_regist = p.obj > -1 ? (p.palette ? cpu->ppu->obp1 : cpu->ppu->obp0) : cpu->ppu->bgp;

    unsigned int color_index = (*c_regist >> (p.color * 2)) & 0x03;
    struct color *color = color_palette + color_index;

    uint32_t pixel = SDL_MapRGB(rend->format, NULL, color->r, color->g, color->b);
    frame_buffer[*cpu->ppu->ly * WIDTH + (cpu->ppu->lx - 8)] = pixel;

    // Render a frame and handle inputs
    if (*cpu->ppu->ly == HEIGHT - 1 && cpu->ppu->lx == WIDTH + 7)
    {
        handle_events(cpu);

        SDL_UpdateTexture(rend->texture, NULL, frame_buffer, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(rend->renderer);
        SDL_RenderTexture(rend->renderer, rend->texture, NULL, NULL);
        SDL_RenderPresent(rend->renderer);

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

    SDL_UpdateTexture(rend->texture, NULL, frame_buffer, WIDTH * sizeof(uint32_t));
    SDL_RenderClear(rend->renderer);
    SDL_RenderTexture(rend->renderer, rend->texture, NULL, NULL);
    SDL_RenderPresent(rend->renderer);
}

void free_renderer(struct renderer *rend)
{
    SDL_DestroyWindow(rend->window);
    SDL_DestroyRenderer(rend->renderer);
    SDL_DestroyTexture(rend->texture);
    free(rend);
}
