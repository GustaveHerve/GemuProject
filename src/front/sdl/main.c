#ifdef _LINUX
#define _POSIX_C_SOURCE 200809L
#endif

#include <SDL3/SDL_main.h>
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
}

static int main_loop(void)
{
    struct global_settings *settings = get_global_settings();
    while (!settings->quit_signal)
    {
        if (settings->paused)
        {
            // Nothing to do meanwhile, wait for an event and handle it
            SDL_CHECK_ERROR(SDL_WaitEvent(NULL));
            handle_events(&gb);
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
            serialize_gb_to_file(save_path, &gb);
            settings->save_state = 0;
        }

        else if (settings->load_state)
        {
            char load_path[PATH_MAX];
            snprintf(load_path, PATH_MAX, "%s" SAVESTATE_EXTENSION "%d", gb.mbc->rom_path, settings->load_state);
            load_gb_from_file(load_path, &gb);
            settings->load_state = 0;
        }

        if (!gb.halt)
            next_op(&gb);
        else
        {
            tick_m(&gb);
            synchronize(&gb);
        }

        check_interrupt(&gb);
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
