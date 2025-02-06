#ifndef RENDERING_H
#define RENDERING_H

#include <SDL3/SDL_render.h>

struct cpu;
struct ppu;
struct pixel;

struct renderer
{
    const SDL_PixelFormatDetails *format;
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    SDL_Window *window;
};

void draw_pixel(struct cpu *cpu, struct pixel p);
void lcd_off(struct cpu *cpu);
void free_renderer(struct renderer *rend);

#endif
