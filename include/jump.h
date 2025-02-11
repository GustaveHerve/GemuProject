#ifndef CORE_JUMP_H
#define CORE_JUMP_H

#include <stdint.h>

struct gb_core;

int jr_e8(struct gb_core *gb);
int jr_cc_e8(struct gb_core *gb, int cc);

int ret(struct gb_core *gb);
int ret_cc(struct gb_core *gb, int cc);
int reti(struct gb_core *gb);

int jp_hl(struct gb_core *gb);
int jp_nn(struct gb_core *gb);
int jp_cc_nn(struct gb_core *gb, int cc);

int call_nn(struct gb_core *gb);
int call_cc_nn(struct gb_core *gb, int cc);

int rst(struct gb_core *gb, uint8_t vec);

#endif
