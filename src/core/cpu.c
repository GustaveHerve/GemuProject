#include "cpu.h"

#include "control.h"
#include "emulation.h"
#include "jump.h"
#include "load.h"
#include "mbc_base.h"
#include "sync.h"
#include "utils.h"

#define MEMBUS_SIZE 65536

void cpu_set_registers_post_boot(struct cpu *cpu, int checksum)
{
    cpu->a = 0x01;
    set_z(cpu, 1);
    set_n(cpu, 0);
    if (checksum == 0x00)
    {
        set_h(cpu, 0);
        set_c(cpu, 0);
    }
    else
    {
        set_h(cpu, 1);
        set_c(cpu, 1);
    }
    cpu->b = 0x00;
    cpu->c = 0x13;
    cpu->d = 0x00;
    cpu->e = 0xD8;
    cpu->h = 0x01;
    cpu->l = 0x4D;
    cpu->pc = 0x0100;
    cpu->sp = 0xFFFE;

    cpu->ime = 0;
}
