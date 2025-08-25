#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <stdlib.h>

#include "common.h"
#include "dcimgui.h"
#include "dcimgui_impl_sdl3.h"
#include "dcimgui_impl_sdlrenderer3.h"
#include "display.h"
#include "logger.h"
#include "sdl_utils.h"
#include "ui.h"

void *pixel_buffer;

SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Window *window;

extern bool imgui_frame_ready;

int set_window_title(const char *title)
{
    SDL_CHECK_ERROR(SDL_SetWindowTitle(window, title));
    return EXIT_SUCCESS;
}

int set_vsync(int val)
{
    LOG_DEBUG(val ? "Vsync enabled" : "Vsync disabled");
    SDL_CHECK_ERROR(SDL_SetRenderVSync(renderer, val));
    return EXIT_SUCCESS;
}

static int draw_game_buffer(void)
{
    SDL_CHECK_ERROR(SDL_RenderClear(renderer));
    SDL_CHECK_ERROR(SDL_RenderTexture(renderer, texture, NULL, NULL));
    return EXIT_SUCCESS;
}

static int draw_imgui(void)
{
    if (imgui_frame_ready)
    {
        ImGuiIO *io = ImGui_GetIO();
        ImGui_Render();
        SDL_CHECK_ERROR(SDL_SetRenderScale(renderer, io->DisplayFramebufferScale.x, io->DisplayFramebufferScale.y));
        SDL_CHECK_ERROR(SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_DISABLED));
        cImGui_ImplSDLRenderer3_RenderDrawData(ImGui_GetDrawData(), renderer);
        SDL_CHECK_ERROR(SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE));
        imgui_frame_ready = false;
    }
    return EXIT_SUCCESS;
}

int render_frame_callback(void)
{
    if (draw_game_buffer())
    {
        LOG_ERROR("Error drawing the game buffer");
        return EXIT_FAILURE;
    }

    if (draw_imgui())
    {
        LOG_ERROR("Error drawing the ImGui");
        return EXIT_FAILURE;
    }

    SDL_CHECK_ERROR(SDL_RenderPresent(renderer));

    return EXIT_SUCCESS;
}

int frame_ready_callback(void)
{
    SDL_CHECK_ERROR(SDL_UpdateTexture(texture, NULL, pixel_buffer, SCREEN_WIDTH * sizeof(uint32_t)));
    return EXIT_SUCCESS;
}

static int init_imgui(void)
{
    CIMGUI_CHECKVERSION();

    ImGui_CreateContext(NULL);
    ImGuiIO *io = ImGui_GetIO();
    (void)io;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; /* Enable Keyboard Controls */
    io->ConfigNavEscapeClearFocusWindow = true;
    io->IniFilename = NULL;

    ImGui_StyleColorsDark(NULL);
    ImGuiStyle *style = ImGui_GetStyle();
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    ImGuiStyle_ScaleAllSizes(style, main_scale);
    style->FontScaleMain = main_scale;

    cImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    cImGui_ImplSDLRenderer3_Init(renderer);

    init_ui();

    return EXIT_SUCCESS;
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

    SDL_CHECK_ERROR(SDL_CreateWindowAndRenderer("GemuProject",
                                                960,
                                                864,
                                                SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE,
                                                &window,
                                                &renderer));
    SDL_CHECK_ERROR(SDL_SetRenderVSync(renderer, 0));
    SDL_CHECK_ERROR(SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE));
    SDL_CHECK_ERROR(texture =
                        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB32, SDL_TEXTUREACCESS_STREAMING, 160, 144));
    SDL_CHECK_ERROR(SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST)); /* Disable texture filtering */

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
