#include "cpu.h"

#include <stdlib.h>
#include <time.h>

#include "apu.h"
#include "control.h"
#include "emulation.h"
#include "jump.h"
#include "load.h"
#include "mbc_base.h"
#include "ppu.h"
#include "sync.h"
#include "utils.h"

#define MEMBUS_SIZE 65536

void cpu_init(struct cpu *cpu, struct renderer *rend)
{
    cpu->regist = malloc(sizeof(struct cpu_register));
    cpu->membus = calloc(MEMBUS_SIZE, sizeof(uint8_t));
    cpu->ppu = malloc(sizeof(struct ppu));
    cpu->apu = malloc(sizeof(struct apu));

    cpu->mbc = NULL;

    ppu_init(cpu->ppu, cpu, rend);
    apu_init(cpu, cpu->apu);

    cpu->ime = 0;
    cpu->ie = &cpu->membus[0xFFFF];
    cpu->_if = &cpu->membus[0xFF0F];
    cpu->div = &cpu->membus[0xFF04];
    cpu->tima = &cpu->membus[0xFF05];
    cpu->tma = &cpu->membus[0xFF06];
    cpu->tac = &cpu->membus[0xFF07];
    cpu->halt = 0;
    cpu->stop = 0;

    cpu->previous_div = 0;
    cpu->internal_div = 0;

    cpu->serial_clock = 0;
    cpu->serial_acc = 0;

    // Joypad
    cpu->joyp_a = 0xF;
    cpu->joyp_d = 0xF;

    cpu->regist->a = 0x00;
    cpu->regist->f = 0x00;
    cpu->regist->b = 0x00;
    cpu->regist->c = 0x00;
    cpu->regist->h = 0x00;
    cpu->regist->l = 0x00;
    cpu->regist->pc = 0x00;
    cpu->regist->sp = 0x00;

    *cpu->ie = 0x00;
    *cpu->_if = 0xE1;
    *cpu->div = 0x00;
    *cpu->tima = 0x00;
    *cpu->tma = 0x00;
    *cpu->tac = 0x00;

    cpu->disabling_timer = 0;
    cpu->schedule_tima_overflow = 0;

    cpu->boot = &cpu->membus[0xFF50];

    cpu->sb = &cpu->membus[0xFF01];
    cpu->sc = &cpu->membus[0xFF02];

    cpu->tcycles_since_sync = 0;
    cpu->last_sync_timestamp = get_nanoseconds();
}

void cpu_set_registers_post_boot(struct cpu *cpu, int checksum)
{
    cpu->regist->a = 0x01;
    set_z(cpu->regist, 1);
    set_n(cpu->regist, 0);
    if (checksum == 0x00)
    {
        set_h(cpu->regist, 0);
        set_c(cpu->regist, 0);
    }
    else
    {
        set_h(cpu->regist, 1);
        set_c(cpu->regist, 1);
    }
    cpu->regist->b = 0x00;
    cpu->regist->c = 0x13;
    cpu->regist->d = 0x00;
    cpu->regist->e = 0xD8;
    cpu->regist->h = 0x01;
    cpu->regist->l = 0x4D;
    cpu->regist->pc = 0x0100;
    cpu->regist->sp = 0xFFFE;
}

void cpu_free(struct cpu *todelete)
{
    ppu_free(todelete->ppu);
    free(todelete->membus);
    free(todelete->regist);
    mbc_free(todelete->mbc);
    apu_free(todelete->apu);
    free(todelete);
}
