#ifndef CORE_LOGIC_H
#define CORE_LOGIC_H

#include <stdint.h>

struct cpu;
struct gb_core;

int inc_r(struct gb_core *gb, uint8_t *dest);
int inc_hl(struct gb_core *gb);

int inc_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo);
int inc_sp(struct gb_core *gb);

int dec_r(struct gb_core *gb, uint8_t *dest);
int dec_hl(struct gb_core *gb);

int dec_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo);
int dec_sp(struct gb_core *gb);

int add_a_r(struct gb_core *gb, uint8_t *src);
int add_a_hl(struct gb_core *gb);
int add_a_n(struct gb_core *gb);

int adc_a_r(struct gb_core *gb, uint8_t *src);
int adc_a_hl(struct gb_core *gb);
int adc_a_n(struct gb_core *gb);

int add_hl_rr(struct gb_core *gb, uint8_t *hi, uint8_t *lo);
int add_hl_sp(struct gb_core *gb);

int add_sp_e8(struct gb_core *gb);

int sub_a_r(struct gb_core *gb, uint8_t *src);
int sub_a_hl(struct gb_core *gb);
int sub_a_n(struct gb_core *gb);

int sbc_a_r(struct gb_core *gb, uint8_t *src);
int sbc_a_hl(struct gb_core *gb);
int sbc_a_n(struct gb_core *gb);

int and_a_r(struct gb_core *gb, uint8_t *src);
int and_a_hl(struct gb_core *gb);
int and_a_n(struct gb_core *gb);

int xor_a_r(struct gb_core *gb, uint8_t *src);
int xor_a_hl(struct gb_core *gb);
int xor_a_n(struct gb_core *gb);

int or_a_r(struct gb_core *gb, uint8_t *src);
int or_a_hl(struct gb_core *gb);
int or_a_n(struct gb_core *gb);

int cp_a_r(struct gb_core *gb, uint8_t *src);
int cp_a_hl(struct gb_core *gb);
int cp_a_n(struct gb_core *gb);

int cpl(struct gb_core *gb);
int daa(struct gb_core *gb);

#endif
