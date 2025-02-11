#include "audio.h"
#define _POSIX_C_SOURCE 2

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "disassembler.h"
#include "emulation.h"
#include "events.h"
#include "gb_core.h"
#include "interrupts.h"
#include "rendering.h"
#include "sdl_utils.h"
#include "sync.h"

struct gb_core gb;

static struct
{
    char *rom_path;
    char *bootrom_path;
} args = {0};

static void print_usage(FILE *stream)
{
    fprintf(stream, "Usage: gemu [-b BOOT_ROM_PATH] ROM_PATH");
    fprintf(stream, "\nOptions:\n");
    fprintf(stream, "  -b BOOT_ROM_PATH   Specify the path to the boot ROM file.\n");
    fprintf(stream, "  -h                 Show this help message and exit.\n");
    fprintf(stream, "\nArguments:\n");
    fprintf(stream, "  ROM_PATH           Path to the ROM file to be used.\n");
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
            exit(0);
        default:
            print_usage(stderr);
            exit(-1);
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "Expected a rom path\n");
        print_usage(stderr);
        exit(-1);
    }

    if (optind + 1 < argc)
    {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind + 1]);
        print_usage(stderr);
        exit(-1);
    }

    args.rom_path = argv[optind];
}

static void init_gb_callbacks(struct gb_core *gb)
{
    gb->callbacks.queue_audio = queue_audio;
    gb->callbacks.get_queued_audio_sample_count = get_queued_sample_count;
    gb->callbacks.handle_events = handle_events;
    gb->callbacks.render_frame = render_frame_callback;
}

void main_loop(void)
{
    struct global_settings *settings = get_global_settings();
    while (!settings->quit_signal)
    {
        if (settings->paused)
        {
            // Nothing to do meanwhile, wait for an event and handle it
            SDL_WaitEvent(NULL);
            handle_events(&gb);
            continue;
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
}

int main(int argc, char **argv)
{
    parse_arguments(argc, argv);

    SDL_CHECK_ERROR(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    if (init_rendering())
        return EXIT_FAILURE;

    init_gb_callbacks(&gb);

    init_gb_core(&gb);

    if (!load_rom(&gb, args.rom_path, args.bootrom_path))
        main_loop();

    free_gb_core(&gb);

    free_rendering();

    SDL_Quit();
    return 0;
}
