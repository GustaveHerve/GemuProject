#include <SDL3/SDL_render.h>
#include <stdlib.h>

#include "common.h"
#include "display.h"
#include "sdl_utils.h"

void *pixel_buffer;

SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Window *window;

int set_window_title(const char *title)
{
    SDL_CHECK_ERROR(SDL_SetWindowTitle(window, title));
    return 0;
}

int set_vsync(int val)
{
    SDL_CHECK_ERROR(SDL_SetRenderVSync(renderer, val));
    return 0;
}

int render_frame_callback(void)
{
    SDL_CHECK_ERROR(SDL_UpdateTexture(texture, NULL, pixel_buffer, SCREEN_WIDTH * sizeof(uint32_t)));
    SDL_CHECK_ERROR(SDL_RenderClear(renderer));
    SDL_CHECK_ERROR(SDL_RenderTexture(renderer, texture, NULL, NULL));
    SDL_CHECK_ERROR(SDL_RenderPresent(renderer));
    return 0;
}

int init_rendering(void)
{
    pixel_buffer = get_frame_buffer();

    SDL_CHECK_ERROR(
        window = SDL_CreateWindow("GemuProject", 960, 864, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE));

    SDL_CHECK_ERROR(renderer = SDL_CreateRenderer(window, NULL));

    SDL_CHECK_ERROR(SDL_SetRenderVSync(renderer, 1));

    SDL_CHECK_ERROR(SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE));

    SDL_CHECK_ERROR(texture =
                        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB32, SDL_TEXTUREACCESS_STREAMING, 160, 144));
    /* Disable texture filtering */
    SDL_CHECK_ERROR(SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST));

    return EXIT_SUCCESS;
}

void free_rendering(void)
{
    if (texture)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
}
