#ifndef ROTSHIFT_H
#define ROTSHIFT_H

struct cpu;

int rlca(struct cpu *cpu);
int rla(struct cpu *cpu);
int rrca(struct cpu *cpu);
int rra(struct cpu *cpu);

#endif
