#ifndef CORE_CONTROL_H
#define CORE_CONTROL_H

struct gb_core;

int nop(void);
int stop(struct gb_core *gb);
int halt(struct gb_core *gb);
int ccf(struct gb_core *gb);
int scf(struct gb_core *gb);
int di(struct gb_core *gb);
int ei(struct gb_core *gb);

#endif
