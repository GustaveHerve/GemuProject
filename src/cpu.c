#include <stdlib.h>
#include "cpu.h"
#include "utils.h"

#define MEMBUS_SIZE 65536 //In bytes

void cpu_init(struct cpu *cpu)
{
	cpu->regist = malloc(sizeof(struct cpu_register));
	cpu->membus = malloc(sizeof(uint8_t) * MEMBUS_SIZE);
    cpu->ime_enable = 0;
    cpu->halt = 0;
    cpu->stop = 0;
}

//Set registers' default values
void cpu_init_regist(struct cpu *cpu)
{
    cpu->regist->a = 0x01;
    cpu->regist->f = 0xB0;
    cpu->regist->b = 0x00;
    cpu->regist->c = 0x13;
    cpu->regist->d = 0x00;
    cpu->regist->e = 0xD8;
    cpu->regist->h = 0x01;
    cpu->regist->l = 0x4D;
    cpu->regist->sp = 0xFFFE;
    cpu->regist->pc = 0x0100;
}

void cpu_start(struct cpu *cpu)
{

}

void cpu_free(struct cpu *todelete)
{
	free(todelete->membus);
	free(todelete);
}
