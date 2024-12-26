#define _POSIX_C_SOURCE 2

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "SDL.h"
#include "cpu.h"
#include "emulation.h"
#include "ppu.h"

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
    if (SDL_Init(SDL_INIT_EVERYTHING))
        return EXIT_FAILURE;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    window = SDL_CreateWindow("GemuProject",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              960,
                              864,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, 160, 144);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    struct renderer *rend = malloc(sizeof(struct renderer));
    rend->format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB888);
    rend->renderer = renderer;
    rend->window = window;
    rend->texture = texture;

    struct cpu *cpu = malloc(sizeof(struct cpu));
    cpu_init(cpu, rend);

    int success = main_loop(cpu, settings.rom_path, settings.bootrom_path);

    free_renderer(cpu->ppu->renderer);
    cpu_free(cpu);

    SDL_Quit();
    return success;
}
