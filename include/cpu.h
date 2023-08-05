#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include "ppu.h"

struct cpu_register
{
	uint8_t a;
	uint8_t f;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint8_t e;
	uint8_t h;
	uint8_t l;

	uint16_t sp; //full 16 bit
	uint16_t pc; //full 16 bit
};

struct cpu
{
    struct ppu *ppu;
	struct cpu_register *regist;
	uint8_t *membus; //16-bit address bus that stores ROM RAM I/O
                     //
    int ime;
    uint8_t *ie;
    uint8_t *_if;

    uint8_t *div;
    uint8_t *tima;
    uint8_t *tma;
    uint8_t *tac;

    int halt;
    int stop;
};

void cpu_init(struct cpu *new_cpu);
void cpu_init_regist(struct cpu *cpu);
void cpu_start(struct cpu *cpu);
void cpu_free(struct cpu *todelete);

void next_op(struct cpu *cpu);

//Interrupts
void set_if(struct cpu *cpu, int bit);
void clear_if(struct cpu *cpu, int bit);

#endif
