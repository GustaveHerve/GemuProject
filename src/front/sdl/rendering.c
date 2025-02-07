#include <SDL3/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>

#include "display.h"

#define SDL_CHECK_ERROR(COND)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(COND))                                                                                                   \
        {                                                                                                              \
            fputs(SDL_GetError(), stderr);                                                                             \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

void *pixel_buffer;

SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Window *window;

static void render_frame(SDL_Renderer *renderer, SDL_Texture *texture)
{
    SDL_UpdateTexture(texture, NULL, pixel_buffer, WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int init_rendering(void)
{
    pixel_buffer = get_frame_buffer();

    SDL_CHECK_ERROR(
        window = SDL_CreateWindow("GemuProject", 960, 864, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE));

    SDL_CHECK_ERROR(
        window = SDL_CreateWindow("GemuProject", 960, 864, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE));

    SDL_CHECK_ERROR(renderer = SDL_CreateRenderer(window, NULL));

    SDL_CHECK_ERROR(SDL_SetRenderVSync(renderer, 1));

    SDL_CHECK_ERROR(SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE));

    SDL_CHECK_ERROR(texture =
                        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144));
    // Disable texture filtering
    SDL_CHECK_ERROR(SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST));

    return EXIT_SUCCESS;
}
