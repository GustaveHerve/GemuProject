#include "emulation.h"
#define _POSIX_C_SOURCE 200809L

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <float.h>
#include <stdbool.h>

#include "dcimgui.h"
#include "display.h"
#include "logger.h"
#include "rendering.h"

extern SDL_Window *window;

bool vsync_enable = false;

static ImVec4 palette[5];

void init_ui(void)
{
    for (size_t i = 0; i < 5; ++i)
    {
        struct color c = get_color_index(i);
        ImU32 u32 = IM_COL32(c.r, c.g, c.b, 0);
        palette[i] = ImGui_ColorConvertU32ToFloat4(u32);
    }
}

static void SDLCALL open_rom_callback(void *userdata, const char *const *filelist, int filter)
{
    (void)userdata;
    (void)filter;
    if (!filelist)
    {
        LOG_ERROR("Error opening rom: %s", SDL_GetError());
        return;
    }
    if (!*filelist)
    {
        LOG_WARN("No rom file selected");
        return;
    }
    LOG_INFO("Loading rom: %s", *filelist);
    if (!(get_global_settings()->open_rom = strdup(*filelist)))
    {
        LOG_ERROR("Error duplicating rom path string");
    }
}

static void open_rom_dialog(void)
{
    static const SDL_DialogFileFilter filters[] = {{"GameBoy roms", "gb;gbc"}, {"All files", "*"}};
    SDL_ShowOpenFileDialog(open_rom_callback, NULL, window, filters, SDL_arraysize(filters), NULL, false);
}

static void update_palette_index(unsigned int index)
{
    ImU32 u32 = ImGui_ColorConvertFloat4ToU32(palette[index]);
    struct color new_c = {
        .r = (u32 >> IM_COL32_R_SHIFT) & 0xFF,
        .g = (u32 >> IM_COL32_G_SHIFT) & 0xFF,
        .b = (u32 >> IM_COL32_B_SHIFT) & 0xFF,
    };
    set_color_index(new_c, index);
}

static void reset_default_palette(void)
{
    reset_palette();
    init_ui();
}

void show_ui(void)
{
    ImGui_SetNextWindowSizeConstraints((ImVec2){250, 200}, (ImVec2){FLT_MAX, FLT_MAX}, NULL, NULL);
    ImGui_SetNextWindowSize((ImVec2){300, 300}, ImGuiCond_Appearing);
    ImGui_Begin("GemuProject configuration", NULL, ImGuiWindowFlags_None);

    if (ImGui_Button("Open ROM"))
        open_rom_dialog();

    if (ImGui_CollapsingHeader("Video settings", ImGuiTreeNodeFlags_None))
    {
        if (ImGui_Checkbox("VSync", &vsync_enable))
            set_vsync(vsync_enable);
    }

    if (ImGui_CollapsingHeader("Color palette", ImGuiTreeNodeFlags_None))
    {
        if (ImGui_ColorEdit3("index 0",
                             (float *)&palette[0],
                             ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha))
            update_palette_index(0);
        if (ImGui_ColorEdit3("index 1",
                             (float *)&palette[1],
                             ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha))
            update_palette_index(1);
        if (ImGui_ColorEdit3("index 2",
                             (float *)&palette[2],
                             ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha))
            update_palette_index(2);
        if (ImGui_ColorEdit3("index 3",
                             (float *)&palette[3],
                             ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha))
            update_palette_index(3);
        if (ImGui_ColorEdit3("screen off",
                             (float *)&palette[4],
                             ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha))
            update_palette_index(4);

        if (ImGui_Button("Reset default palette"))
            reset_default_palette();
    }

    ImGui_End();
}
