#ifndef CORE_LOAD_H
#define CORE_LOAD_H

#include <stdint.h>

struct gb_core;

int ld_rr_a(struct gb_core *gb, uint8_t *hi, uint8_t *lo);
int ld_r_r(struct gb_core *gb, uint8_t *dest, uint8_t *src);
int ld_r_u8(struct gb_core *gb, uint8_t *dest);
int ld_hl_u8(struct gb_core *gb);
int ld_a_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo);
int ld_hl_r(struct gb_core *gb, uint8_t *src);
int ld_r_hl(struct gb_core *gb, uint8_t *dest);
int ld_nn_a(struct gb_core *gb);
int ld_a_nn(struct gb_core *gb);

int ldh_n_a(struct gb_core *gb);
int ldh_a_n(struct gb_core *gb);

int ldh_a_c(struct gb_core *gb);
int ldh_c_a(struct gb_core *gb);

int ldi_hl_a(struct gb_core *gb);
int ldd_hl_a(struct gb_core *gb);

int ldi_a_hl(struct gb_core *gb);
int ldd_a_hl(struct gb_core *gb);

int ld_rr_nn(struct gb_core *gb, uint8_t *hi, uint8_t *lo);
int ld_sp_nn(struct gb_core *gb);
int ld_sp_hl(struct gb_core *gb);
int ld_nn_sp(struct gb_core *gb);

int ld_hl_spe8(struct gb_core *gb);

int pop_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo);
int pop_af(struct gb_core *gb);

int push_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo);

#endif
