#ifndef CORE_ROTSHIFT_H
#define CORE_ROTSHIFT_H

struct gb_core;

int rlca(struct gb_core *gb);
int rla(struct gb_core *gb);
int rrca(struct gb_core *gb);
int rra(struct gb_core *gb);

#endif
