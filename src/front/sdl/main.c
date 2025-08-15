#ifdef _LINUX
#define _POSIX_C_SOURCE 200809L
#endif

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <assert.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "audio.h"
#include "disassembler.h"
#include "emulation.h"
#include "events.h"
#include "gb_core.h"
#include "interrupts.h"
#include "logger.h"
#include "mbc_base.h"
#include "rendering.h"
#include "save.h"
#include "sdl_utils.h"
#include "serialization.h"
#include "sync.h"

#define SAVESTATE_EXTENSION ".savestate"

struct gb_core gb;

static struct
{
    char *rom_path;
    char *bootrom_path;
} args;

static void print_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: gemu [-b BOOT_ROM_PATH] ROM_PATH"
            "\nOptions:\n"
            "  -b BOOT_ROM_PATH   Specify the path to the boot ROM file.\n"
            "  -h                 Show this help message and exit.\n"
            "\nArguments:\n"
            "  ROM_PATH           Path to the ROM file to be used.\n");
}

static void parse_arguments(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "b:h")) != -1)
    {
        switch (opt)
        {
        case 'b':
            args.bootrom_path = optarg;
            break;
        case 'h':
            print_usage(stdout);
            exit(EXIT_SUCCESS);
        default:
            print_usage(stderr);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "ERROR: Expected a rom path\n");
        print_usage(stderr);
        exit(EXIT_FAILURE);
    }

    if (optind + 1 < argc)
    {
        fprintf(stderr, "ERROR: Unexpected argument: %s\n", argv[optind + 1]);
        print_usage(stderr);
        exit(EXIT_FAILURE);
    }

    args.rom_path = argv[optind];
}

static void init_gb_callbacks(struct gb_core *gb)
{
    assert(gb);
    gb->callbacks.queue_audio = queue_audio;
    gb->callbacks.get_queued_audio_sample_count = get_queued_sample_count;
    gb->callbacks.handle_events = handle_events;
    gb->callbacks.render_frame = render_frame_callback;
    gb->callbacks.frame_ready = frame_ready_callback;
}

#define SECOND_TO_NANOSECONDS 1000000000ULL
#define REFRESH_RATE_HZ 60

#if 0
static int main_loop(void)
{
    struct global_settings *settings = get_global_settings();
    const uint64_t frame_interval_ns = SECOND_TO_NANOSECONDS / REFRESH_RATE_HZ;
    uint64_t last_render_time = SDL_GetTicksNS();
    uint64_t now_ns;

    while (!settings->quit_signal)
    {
        if (settings->paused)
        {
            /* Sleep to avoid CPU hot loop resulting from busy waiting */
            now_ns = SDL_GetTicksNS();
            uint64_t elapsed = now_ns - last_render_time;

            if (elapsed < frame_interval_ns)
            {
                SDL_DelayPrecise(frame_interval_ns - elapsed);
                now_ns = SDL_GetTicksNS();
            }

            last_render_time = now_ns;
            gb.callbacks.handle_events(&gb);
            gb.callbacks.render_frame();
            continue;
        }

        if (settings->reset_signal)
        {
            reset_gb(&gb);
            settings->reset_signal = false;
        }

        else if (settings->save_state)
        {
            char save_path[PATH_MAX];
            snprintf(save_path, PATH_MAX, "%s" SAVESTATE_EXTENSION "%d", gb.mbc->rom_path, settings->save_state);
            gb_core_serialize(save_path, &gb);
            LOG_INFO("Created save state in slot %d", settings->save_state);
            settings->save_state = 0;
        }

        else if (settings->load_state)
        {
            char load_path[PATH_MAX];
            snprintf(load_path, PATH_MAX, "%s" SAVESTATE_EXTENSION "%d", gb.mbc->rom_path, settings->load_state);
            gb_core_load_from_file(load_path, &gb);
            LOG_INFO("Loaded save state in slot %d", settings->load_state);
            settings->load_state = 0;
        }

        if (gb.halt)
            tick_m(&gb);
        else if (next_op(&gb) == -1)
            return EXIT_FAILURE;

        synchronize(&gb); /* TODO: Find a less intensive place to call this when CPU is running */

        check_interrupt(&gb);

        /* Event handling and rendering routine */
        now_ns = SDL_GetTicksNS();
        if (now_ns - last_render_time >= frame_interval_ns)
        {
            /* Calling synchronize(&gb); here seems like a good idea but it is not enough to call it once per frame,
             * (fast to render screens like bootrom are too fast) */
            last_render_time += frame_interval_ns;
            gb.callbacks.handle_events(&gb);
            gb.callbacks.render_frame();
        }
    }
    return EXIT_SUCCESS;
}
#endif

static int main_loop(void)
{
    struct global_settings *settings = get_global_settings();
    double render_period_ns = (1e9 / REFRESH_RATE_HZ);
    uint64_t last_render_ts = SDL_GetTicksNS();
    uint64_t now_ts;

    uint64_t emulation_resume_ts = 0;

    while (!settings->quit_signal)
    {
        now_ts = SDL_GetTicksNS();

        /* Rendering / Event handling routine */
        if (now_ts - last_render_ts >= render_period_ns)
        {
            last_render_ts += render_period_ns;
            gb.callbacks.handle_events(&gb);
            // if (!settings->occluded)
            gb.callbacks.render_frame();
        }

        if (settings->paused)
        {
            uint64_t next_render_time = last_render_ts + render_period_ns;
            now_ts = SDL_GetTicksNS();
            if (now_ts < next_render_time)
                SDL_DelayPrecise(next_render_time - now_ts);
            continue;
        }

        if (settings->reset_signal)
        {
            reset_gb(&gb);
            settings->reset_signal = false;
        }
        else if (settings->save_state)
        {
            char save_path[PATH_MAX];
            snprintf(save_path, PATH_MAX, "%s" SAVESTATE_EXTENSION "%d", gb.mbc->rom_path, settings->save_state);
            gb_core_serialize(save_path, &gb);
            LOG_INFO("Created save state in slot %d", settings->save_state);
            settings->save_state = 0;
        }
        else if (settings->load_state)
        {
            char load_path[PATH_MAX];
            snprintf(load_path, PATH_MAX, "%s" SAVESTATE_EXTENSION "%d", gb.mbc->rom_path, settings->load_state);
            gb_core_load_from_file(load_path, &gb);
            LOG_INFO("Loaded save state in slot %d", settings->load_state);
            settings->load_state = 0;
        }

        /* GB emulation routine */
        if (now_ts >= emulation_resume_ts)
        {
            if (gb.halt)
                tick_m(&gb);
            else if (next_op(&gb) == -1)
                return EXIT_FAILURE;

            int64_t ns_to_wait = synchronize(&gb);
            if (ns_to_wait > 0)
                emulation_resume_ts = now_ts + (uint64_t)ns_to_wait;
            else
                emulation_resume_ts = now_ts;

            check_interrupt(&gb);
        }

        /* Idle waiting to avoid busy looping */
        now_ts = SDL_GetTicksNS();
        uint64_t next_render_ts = last_render_ts + render_period_ns;
        uint64_t sleep_until = (emulation_resume_ts < next_render_ts) ? emulation_resume_ts : next_render_ts;
        if (now_ts < sleep_until)
            SDL_DelayPrecise(sleep_until - now_ts);
    }

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    int err_code = EXIT_SUCCESS;
    parse_arguments(argc, argv);

    LOG_INFO("Welcome to GemuProject!");
    LOG_INFO("Initializing SDL...");

    SDL_CHECK_ERROR(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS));
    if (init_rendering())
    {
        err_code = EXIT_FAILURE;
        goto exit3;
    }
    if (init_audio())
    {
        err_code = EXIT_FAILURE;
        goto exit2;
    }

    LOG_INFO("Initializing emulator...");
    init_gb_callbacks(&gb);
    if (init_gb_core(&gb))
    {
        err_code = EXIT_FAILURE;
        goto exit1;
    }

    LOG_INFO("Loading rom: %s", args.rom_path);
    if (load_rom(&gb, args.rom_path, args.bootrom_path))
    {
        err_code = EXIT_FAILURE;
        goto exit1;
    }

    main_loop();

exit1:
    free_gb_core(&gb);
exit2:
    free_audio();
exit3:
    free_rendering();
    SDL_Quit();
    return err_code;
}
