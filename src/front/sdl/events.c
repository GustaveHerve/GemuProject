#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_render.h>

#include "dcimgui.h"
#include "dcimgui_impl_sdl3.h"
#include "dcimgui_impl_sdlrenderer3.h"
#include "emulation.h"
#include "gb_core.h"
#include "logger.h"
#include "rendering.h"

#define DEFAULT_TITLE_TEXT "GemuProject - Press F1 to open menu"
#define PAUSE_TITLE_TEXT "GemuProject (Paused) - Press F1 to open menu"

extern SDL_Renderer *renderer;

bool imgui_frame_ready = false;
bool show_demo_window = false;

static struct
{
    bool left : 1;
    bool right : 1;
    bool up : 1;
    bool down : 1;
} dpad_state; /* Used to prevent impossible D-Pad input combinations (L+R / U+D) */

void handle_events(struct gb_core *gb)
{
    SDL_Event event;
    struct global_settings *settings = get_global_settings();
    ImGuiIO *io = ImGui_GetIO();
    while (SDL_PollEvent(&event))
    {
        if (show_demo_window)
            cImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
        {
            /* UI intercepts keyboard inputs when opened */
            if (show_demo_window && io->WantCaptureKeyboard)
                break;
            switch (event.key.key)
            {
            case SDLK_RIGHT:
                dpad_state.right = 1;
                gb->joyp_d &= ~(0x01);
                gb->joyp_d |= 0x02; /* Prevent Left */
                break;
            case SDLK_LEFT:
                dpad_state.left = 1;
                gb->joyp_d &= ~(0x02);
                gb->joyp_d |= 0x01; /* Prevent Right */
                break;
            case SDLK_UP:
                dpad_state.up = 1;
                gb->joyp_d &= ~(0x04);
                gb->joyp_d |= 0x08; /* Prevent Down */
                break;
            case SDLK_DOWN:
                dpad_state.down = 1;
                gb->joyp_d &= ~(0x08);
                gb->joyp_d |= 0x04; /* Prevent Up */
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
                    // set_vsync(1);
                }
                settings->paused = !settings->paused;
                if (settings->paused)
                    set_window_title(PAUSE_TITLE_TEXT);
                else
                    set_window_title(DEFAULT_TITLE_TEXT);
                break;
            case SDLK_T:
                settings->turbo = true;
                // set_vsync(0);
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
                dpad_state.right = 0;
                gb->joyp_d |= 0x01;
                if (dpad_state.left)
                    gb->joyp_d &= ~(0x02);
                break;
            case SDLK_LEFT:
                dpad_state.left = 0;
                gb->joyp_d |= 0x02;
                if (dpad_state.right)
                    gb->joyp_d &= ~(0x01);
                break;
            case SDLK_UP:
                dpad_state.up = 0;
                gb->joyp_d |= 0x04;
                if (dpad_state.down)
                    gb->joyp_d &= ~(0x08);
                break;
            case SDLK_DOWN:
                dpad_state.down = 0;
                gb->joyp_d |= 0x08;
                if (dpad_state.up)
                    gb->joyp_d &= ~(0x04);
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
                // set_vsync(1);
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
            case SDLK_F1:
                show_demo_window = !show_demo_window;
                if (show_demo_window)
                {
                    ImGui_SetWindowFocusStr("Dear ImGui Demo");
                }
                break;
            }
        }
        break;
        case SDL_EVENT_QUIT:
        {
            LOG_DEBUG("SDL_EVENT_QUIT event detected");
            struct global_settings *settings = get_global_settings();
            settings->quit_signal = true;
            break;
        }
        case SDL_EVENT_WINDOW_OCCLUDED:
        {
            LOG_DEBUG("SDL_EVENT_WINDOW_OCCLUDED event detected");
            break;
        }
        case SDL_EVENT_WINDOW_EXPOSED:
        {
            LOG_DEBUG("SDL_EVENT_WINDOW_EXPOSED event detected");
            break;
        }
        }
    }

    cImGui_ImplSDLRenderer3_NewFrame();
    cImGui_ImplSDL3_NewFrame();
    ImGui_NewFrame();

    if (show_demo_window)
        ImGui_ShowDemoWindow(&show_demo_window);

    imgui_frame_ready = true;
}
