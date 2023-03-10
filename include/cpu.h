#ifndef CPU_H
#define CPU_H

struct cpu_register
{
	uint16_t af;
	uint16_t bc;
	uint16_t de;
	uint16_t hl;
	uint16_t sp; //full 16 bit
	uint16_t pc; //full 16 bit
};

struct cpu
{
	struct cpu_register regist;
	uint8_t *membus; //16-bit address bus that stores ROM RAM I/O
};

int nop();
int ld_16bit(uint16_t *pc, int opcode);

#endif
