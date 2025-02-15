#ifndef CORE_PREFIX_H
#define CORE_PREFIX_H

#include <stdint.h>

struct gb_core;

int rlc(struct gb_core *gb, uint8_t *dest);
int rlc_hl(struct gb_core *gb);
int rrc(struct gb_core *gb, uint8_t *dest);
int rrc_hl(struct gb_core *gb);

int rl(struct gb_core *gb, uint8_t *dest);
int rl_hl(struct gb_core *gb);
int rr(struct gb_core *gb, uint8_t *dest);
int rr_hl(struct gb_core *gb);

int sla(struct gb_core *gb, uint8_t *dest);
int sla_hl(struct gb_core *gb);
int sra(struct gb_core *gb, uint8_t *dest);
int sra_hl(struct gb_core *gb);

int swap(struct gb_core *gb, uint8_t *dest);
int swap_hl(struct gb_core *gb);

int srl(struct gb_core *gb, uint8_t *dest);
int srl_hl(struct gb_core *gb);

int bit(struct gb_core *gb, uint8_t *dest, int n);
int bit_hl(struct gb_core *gb, int n);

int res(uint8_t *dest, int n);
int res_hl(struct gb_core *gb, int n);

int set(uint8_t *dest, int n);
int set_hl(struct gb_core *gb, int n);

#endif
