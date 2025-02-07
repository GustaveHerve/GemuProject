#define _POSIX_C_SOURCE 2

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cpu.h"
#include "emulation.h"
#include "gb_core.h"
#include "rendering.h"

struct gb_core gb;

static struct
{
    char *rom_path;
    char *bootrom_path;
} settings = {0};

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
            settings.bootrom_path = optarg;
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

    settings.rom_path = argv[optind];
}

int main(int argc, char **argv)
{
    parse_arguments(argc, argv);

    // TODO: dissociate SDL handling from the rest of the program
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        fprintf(stderr, "Error initializing SDL\n");
        return EXIT_FAILURE;
    }

    init_rendering();

    cpu_init(&cpu, &rend);

    int success = start_emulator(settings.rom_path, settings.bootrom_path);

    free_renderer(rend);
    cpu_free(cpu);

    SDL_Quit();
    return success;
}
