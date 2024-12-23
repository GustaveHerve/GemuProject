#include "emulation.h"

#include <SDL_stdinc.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL_events.h"
#include "apu.h"
#include "cpu.h"
#include "disassembler.h"
#include "interrupts.h"
#include "mbc_base.h"
#include "ppu.h"
#include "serial.h"
#include "sync.h"
#include "timers.h"

static void set_memory_post_boot(struct cpu *cpu)
{
    cpu->membus[0xFF00] = 0xCF;
    cpu->membus[0xFF01] = 0x00;
    cpu->membus[0xFF02] = 0x7E;
    cpu->membus[0xFF04] = 0xAB;
    cpu->internal_div = 0xAB00;
    cpu->membus[0xFF05] = 0x00;
    cpu->membus[0xFF06] = 0x00;
    cpu->membus[0xFF07] = 0xF8;
    cpu->membus[0xFF0F] = 0xE1;
    cpu->membus[0xFF10] = 0x80;
    cpu->membus[0xFF11] = 0xBF;
    cpu->membus[0xFF12] = 0xF3;
    cpu->membus[0xFF13] = 0xFF;
    cpu->membus[0xFF14] = 0xBF;
    cpu->membus[0xFF16] = 0x3F;
    cpu->membus[0xFF17] = 0x00;
    cpu->membus[0xFF18] = 0xFF;
    cpu->membus[0xFF19] = 0xBF;
    cpu->membus[0xFF1A] = 0x7F;
    cpu->membus[0xFF1B] = 0xFF;
    cpu->membus[0xFF1C] = 0x9F;
    cpu->membus[0xFF1D] = 0xFF;
    cpu->membus[0xFF1E] = 0xBF;
    cpu->membus[0xFF20] = 0xFF;
    cpu->membus[0xFF21] = 0x00;
    cpu->membus[0xFF22] = 0x00;
    cpu->membus[0xFF23] = 0xBF;
    cpu->membus[0xFF24] = 0x77;
    cpu->membus[0xFF25] = 0xF3;
    cpu->membus[0xFF26] = 0xF1;
    cpu->membus[0xFF40] = 0x91;
    cpu->membus[0xFF41] = 0x85;
    cpu->membus[0xFF42] = 0x00;
    cpu->membus[0xFF43] = 0x00;
    cpu->membus[0xFF44] = 0x00;
    cpu->membus[0xFF45] = 0x00;
    cpu->membus[0xFF46] = 0xFF;
    cpu->membus[0xFF47] = 0xFC;
    cpu->membus[0xFF48] = 0xFF;
    cpu->membus[0xFF49] = 0xFF;
    cpu->membus[0xFF4A] = 0x00;
    cpu->membus[0xFF4B] = 0x00;
    cpu->membus[0xFF4D] = 0xFF;
    cpu->membus[0xFF4F] = 0xFF;
    cpu->membus[0xFF50] = 0xFF;
    cpu->membus[0xFF51] = 0xFF;
    cpu->membus[0xFF52] = 0xFF;
    cpu->membus[0xFF53] = 0xFF;
    cpu->membus[0xFF54] = 0xFF;
    cpu->membus[0xFF55] = 0xFF;
    cpu->membus[0xFF56] = 0xFF;
    cpu->membus[0xFF68] = 0xFF;
    cpu->membus[0xFF69] = 0xFF;
    cpu->membus[0xFF6A] = 0xFF;
    cpu->membus[0xFF6B] = 0xFF;
    cpu->membus[0xFF70] = 0xFF;
    cpu->membus[0xFFFF] = 0x00;

    cpu->ppu->current_mode = 1;
    cpu->ppu->line_dot_count = 400;
    cpu->ppu->mode1_153th = 1;
}

struct global_settings settings = {0};

struct global_settings *get_global_settings(void)
{
    return &settings;
}

void handle_events(struct cpu *cpu)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_RIGHT:
                cpu->joyp_d &= ~(0x01);
                break;
            case SDLK_LEFT:
                cpu->joyp_d &= ~(0x02);
                break;
            case SDLK_UP:
                cpu->joyp_d &= ~(0x04);
                break;
            case SDLK_DOWN:
                cpu->joyp_d &= ~(0x08);
                break;

            case SDLK_x:
                cpu->joyp_a &= ~(0x01);
                break;
            case SDLK_z:
                cpu->joyp_a &= ~(0x02);
                break;
            case SDLK_SPACE:
                cpu->joyp_a &= ~(0x04);
                break;
            case SDLK_RETURN:
                cpu->joyp_a &= ~(0x08);
                break;
            case SDLK_p:
            {
                settings.paused = !settings.paused;
                if (settings.paused)
                    SDL_SetWindowTitle(cpu->ppu->renderer->window, "GemuProject - Paused");
                else
                    SDL_SetWindowTitle(cpu->ppu->renderer->window, "GemuProject");
                break;
            }
            case SDLK_t:
                settings.turbo = SDL_TRUE;
                break;
            }
            break;
        }
        case SDL_KEYUP:
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_RIGHT:
                cpu->joyp_d |= 0x01;
                break;
            case SDLK_LEFT:
                cpu->joyp_d |= 0x02;
                break;
            case SDLK_UP:
                cpu->joyp_d |= 0x04;
                break;
            case SDLK_DOWN:
                cpu->joyp_d |= 0x08;
                break;

            case SDLK_x:
                cpu->joyp_a |= 0x01;
                break;
            case SDLK_z:
                cpu->joyp_a |= 0x02;
                break;
            case SDLK_SPACE:
                cpu->joyp_a |= 0x04;
                break;
            case SDLK_RETURN:
                cpu->joyp_a |= 0x08;
                break;
            case SDLK_t:
                settings.turbo = SDL_FALSE;
                break;
            }
        }
        break;
        case SDL_QUIT:
            cpu->running = 0;
            return;
        }
    }
}

void main_loop(struct cpu *cpu, char *rom_path, char *boot_rom_path)
{
    cpu->running = 1;

    if (boot_rom_path != NULL)
    {
        // Enable bootrom
        cpu->membus[0xFF50] = 0xFE;

        // Open BOOTROM
        FILE *fptr = fopen(boot_rom_path, "rb");
        if (!fptr)
        {
            fprintf(stderr, "Invalid boot rom path: %s\n", boot_rom_path);
            return;
        }
        fseek(fptr, 0, SEEK_END);
        long fsize = ftell(fptr);
        rewind(fptr);

        fread(cpu->membus, 1, fsize, fptr);
        fclose(fptr);
    }

    // Open ROM, get its size and and copy its content in MBC struct
    FILE *fptr = fopen(rom_path, "rb");
    if (!fptr)
    {
        fprintf(stderr, "Invalid rom path: %s\n", rom_path);
        return;
    }
    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);

    uint8_t *rom = malloc(sizeof(uint8_t) * fsize);
    fread(rom, 1, fsize, fptr);
    fclose(fptr);

    uint8_t checksum = rom[0x14d];

    // Init MBC / cartridge info and fill rom in buffer
    set_mbc(&cpu->mbc, rom, rom_path);

    lcd_off(cpu);

    if (boot_rom_path == NULL)
    {
        cpu->regist->pc = 0x0100;
        cpu_set_registers_post_boot(cpu, checksum);
        set_memory_post_boot(cpu);
    }

    while (cpu->running)
    {
        if (settings.paused)
        {
            SDL_WaitEvent(NULL);
            handle_events(cpu);
            continue;
        }

        if (!cpu->halt)
            next_op(cpu);
        else
        {
            tick_m(cpu); // Previous instruction tick + next OPCode fetch
            synchronize(cpu);
        }

        check_interrupt(cpu);
    }
}

void tick_m(struct cpu *cpu)
{
    cpu->tcycles_since_sync += 4;

    if (cpu->ime == 2)
        cpu->ime = 1;

    apu_tick_m(cpu->apu);

    update_timers(cpu);
    update_serial(cpu);

    if (get_lcdc(cpu->ppu, LCDC_LCD_PPU_ENABLE))
        ppu_tick_m(cpu->ppu);
}
