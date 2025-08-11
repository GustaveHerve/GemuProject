#include <SDL3/SDL_render.h>
#include <stdlib.h>

#include "common.h"
#include "dcimgui.h"
#include "dcimgui_impl_sdl3.h"
#include "dcimgui_impl_sdlrenderer3.h"
#include "display.h"
#include "logger.h"
#include "sdl_utils.h"

void *pixel_buffer;

SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Window *window;

bool first_time_render = true;

extern bool show_demo_window;

int set_window_title(const char *title)
{
    SDL_CHECK_ERROR(SDL_SetWindowTitle(window, title));
    return EXIT_SUCCESS;
}

int set_vsync(int val)
{
    SDL_CHECK_ERROR(SDL_SetRenderVSync(renderer, val));
    return EXIT_SUCCESS;
}

int render_frame_callback(void)
{
    SDL_CHECK_ERROR(SDL_UpdateTexture(texture, NULL, pixel_buffer, SCREEN_WIDTH * sizeof(uint32_t)));
    SDL_CHECK_ERROR(SDL_RenderClear(renderer));
    SDL_CHECK_ERROR(SDL_RenderTexture(renderer, texture, NULL, NULL));

    if (show_demo_window && !first_time_render)
    {
        SDL_CHECK_ERROR(SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_DISABLED));

        ImGuiIO *io = ImGui_GetIO();
        ImGui_Render();
        SDL_CHECK_ERROR(SDL_SetRenderScale(renderer, io->DisplayFramebufferScale.x, io->DisplayFramebufferScale.y));
        cImGui_ImplSDLRenderer3_RenderDrawData(ImGui_GetDrawData(), renderer);

        SDL_CHECK_ERROR(SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE));
    }
    SDL_CHECK_ERROR(SDL_RenderPresent(renderer));

    first_time_render = false;
    return EXIT_SUCCESS;
}

static void init_imgui(void)
{
    CIMGUI_CHECKVERSION();

    ImGui_CreateContext(NULL);
    ImGuiIO *io = ImGui_GetIO();
    (void)io;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Control
    // io->DisplayFramebufferScale = (ImVec2){960, 864};

    ImGui_StyleColorsDark(NULL);

    ImGuiStyle *style = ImGui_GetStyle();
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    ImGuiStyle_ScaleAllSizes(style, main_scale);
    style->FontScaleMain = main_scale;

    cImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    cImGui_ImplSDLRenderer3_Init(renderer);
}

static void deinit_imgui(void)
{
    cImGui_ImplSDLRenderer3_Shutdown();
    cImGui_ImplSDL3_Shutdown();
    ImGui_DestroyContext(NULL);
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

    init_imgui();

    return EXIT_SUCCESS;
}

void free_rendering(void)
{
    deinit_imgui();
    if (texture)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
}
