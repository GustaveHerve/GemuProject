#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>

#include "dcimgui_impl_sdl3.h"
#include "dcimgui_impl_sdlrenderer3.h"
#include "emulation.h"
#include "gb_core.h"
#include "rendering.h"

extern SDL_Renderer *renderer;
bool show_demo_window = true;

void handle_events(struct gb_core *gb)
{
    SDL_Event event;
    struct global_settings *settings = get_global_settings();
    while (SDL_PollEvent(&event))
    {
        cImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
        {
            switch (event.key.key)
            {
            case SDLK_RIGHT:
                gb->joyp_d &= ~(0x01);
                break;
            case SDLK_LEFT:
                gb->joyp_d &= ~(0x02);
                break;
            case SDLK_UP:
                gb->joyp_d &= ~(0x04);
                break;
            case SDLK_DOWN:
                gb->joyp_d &= ~(0x08);
                break;

            case SDLK_X:
                gb->joyp_a &= ~(0x01);
                break;
            case SDLK_Z:
                gb->joyp_a &= ~(0x02);
                break;
            case SDLK_SPACE:
                gb->joyp_a &= ~(0x04);
                break;
            case SDLK_RETURN:
                gb->joyp_a &= ~(0x08);
                break;
            case SDLK_P:
                if (settings->turbo)
                {
                    settings->turbo = false;
                    set_vsync(1);
                }
                settings->paused = !settings->paused;
                if (settings->paused)
                    set_window_title("GemuProject - Paused");
                else
                    set_window_title("GemuProject");
                break;
            case SDLK_T:
                settings->turbo = true;
                set_vsync(0);
                break;
            case SDLK_R:
                if (!settings->paused)
                    settings->reset_signal = true;
                break;
            }

            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            switch (event.key.key)
            {
            case SDLK_RIGHT:
                gb->joyp_d |= 0x01;
                break;
            case SDLK_LEFT:
                gb->joyp_d |= 0x02;
                break;
            case SDLK_UP:
                gb->joyp_d |= 0x04;
                break;
            case SDLK_DOWN:
                gb->joyp_d |= 0x08;
                break;

            case SDLK_X:
                gb->joyp_a |= 0x01;
                break;
            case SDLK_Z:
                gb->joyp_a |= 0x02;
                break;
            case SDLK_SPACE:
                gb->joyp_a |= 0x04;
                break;
            case SDLK_RETURN:
                gb->joyp_a |= 0x08;
                break;
            case SDLK_T:
            {
                struct global_settings *settings = get_global_settings();
                settings->turbo = false;
                set_vsync(1);
                break;
            }
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
            case SDLK_8:
            case SDLK_9:
            {
                struct global_settings *settings = get_global_settings();
                unsigned char key = event.key.key - SDLK_1 + 1;
#ifdef _MACOS
                if (event.key.mod & SDL_KMOD_LGUI)
#else
                if (event.key.mod & SDL_KMOD_CTRL)
#endif
                    settings->load_state = key;
                else
                    settings->save_state = key;
                break;
            }
            }
        }
        break;
        case SDL_EVENT_QUIT:
        {
            struct global_settings *settings = get_global_settings();
            settings->quit_signal = true;
            return;
        }
        }
    }

    SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_DISABLED);
    cImGui_ImplSDLRenderer3_NewFrame();
    cImGui_ImplSDL3_NewFrame();
    ImGui_NewFrame();

    if (show_demo_window)
        ImGui_ShowDemoWindow(&show_demo_window);
}
