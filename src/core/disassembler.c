#include "disassembler.h"

#include <err.h>

#include "control.h"
#include "emulation.h"
#include "gb_core.h"
#include "jump.h"
#include "load.h"
#include "logic.h"
#include "prefix.h"
#include "read.h"
#include "rotshift.h"
#include "utils.h"

static int prefix_op(struct gb_core *gb);

int next_op(struct gb_core *gb)
{
    int mcycles = 0;
    uint8_t opcode = read_mem_tick(gb, gb->cpu.pc);
    gb->cpu.pc += !gb->halt_bug;
    gb->halt_bug = 0;
    switch (opcode)
    {
    case 0x00:
        mcycles = nop();
        break;
    case 0x01:
        mcycles = ld_rr_nn(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0x02:
        mcycles = ld_rr_a(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0x03:
        mcycles = inc_rr(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0x04:
        mcycles = inc_r(gb, &gb->cpu.b);
        break;
    case 0x05:
        mcycles = dec_r(gb, &gb->cpu.b);
        break;
    case 0x06:
        mcycles = ld_r_u8(gb, &gb->cpu.b);
        break;
    case 0x07:
        mcycles = rlca(gb);
        break;
    case 0x08:
        mcycles = ld_nn_sp(gb);
        break;
    case 0x09:
        mcycles = add_hl_rr(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0x0A:
        mcycles = ld_a_rr(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0x0B:
        mcycles = dec_rr(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0x0C:
        mcycles = inc_r(gb, &gb->cpu.c);
        break;
    case 0x0D:
        mcycles = dec_r(gb, &gb->cpu.c);
        break;
    case 0x0E:
        mcycles = ld_r_u8(gb, &gb->cpu.c);
        break;
    case 0x0F:
        mcycles = rrca(gb);
        break;
    case 0x10:
        mcycles = stop(gb);
        break;
    case 0x11:
        mcycles = ld_rr_nn(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0x12:
        mcycles = ld_rr_a(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0x13:
        mcycles = inc_rr(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0x14:
        mcycles = inc_r(gb, &gb->cpu.d);
        break;
    case 0x15:
        mcycles = dec_r(gb, &gb->cpu.d);
        break;
    case 0x16:
        mcycles = ld_r_u8(gb, &gb->cpu.d);
        break;
    case 0x17:
        mcycles = rla(gb);
        break;
    case 0x18:
        mcycles = jr_e8(gb);
        break;
    case 0x19:
        mcycles = add_hl_rr(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0x1A:
        mcycles = ld_a_rr(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0x1B:
        mcycles = dec_rr(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0x1C:
        mcycles = inc_r(gb, &gb->cpu.e);
        break;
    case 0x1D:
        mcycles = dec_r(gb, &gb->cpu.e);
        break;
    case 0x1E:
        mcycles = ld_r_u8(gb, &gb->cpu.e);
        break;
    case 0x1F:
        mcycles = rra(gb);
        break;
    case 0x20:
        mcycles = jr_cc_e8(gb, get_z(&gb->cpu) == 0);
        break;
    case 0x21:
        mcycles = ld_rr_nn(gb, &gb->cpu.h, &gb->cpu.l);
        break;
    case 0x22:
        mcycles = ldi_hl_a(gb);
        break;
    case 0x23:
        mcycles = inc_rr(gb, &gb->cpu.h, &gb->cpu.l);
        break;
    case 0x24:
        mcycles = inc_r(gb, &gb->cpu.h);
        break;
    case 0x25:
        mcycles = dec_r(gb, &gb->cpu.h);
        break;
    case 0x26:
        mcycles = ld_r_u8(gb, &gb->cpu.h);
        break;
    case 0x27:
        mcycles = daa(gb);
        break;
    case 0x28:
        mcycles = jr_cc_e8(gb, get_z(&gb->cpu) == 1);
        break;
    case 0x29:
        mcycles = add_hl_rr(gb, &gb->cpu.h, &gb->cpu.l);
        break;
    case 0x2A:
        mcycles = ldi_a_hl(gb);
        break;
    case 0x2B:
        mcycles = dec_rr(gb, &gb->cpu.h, &gb->cpu.l);
        break;
    case 0x2C:
        mcycles = inc_r(gb, &gb->cpu.l);
        break;
    case 0x2D:
        mcycles = dec_r(gb, &gb->cpu.l);
        break;
    case 0x2E:
        mcycles = ld_r_u8(gb, &gb->cpu.l);
        break;
    case 0x2F:
        mcycles = cpl(gb);
        break;
    case 0x30:
        mcycles = jr_cc_e8(gb, get_c(&gb->cpu) == 0);
        break;
    case 0x31:
        mcycles = ld_sp_nn(gb);
        break;
    case 0x32:
        mcycles = ldd_hl_a(gb);
        break;
    case 0x33:
        mcycles = inc_sp(gb);
        break;
    case 0x34:
        mcycles = inc_hl(gb);
        break;
    case 0x35:
        mcycles = dec_hl(gb);
        break;
    case 0x36:
        mcycles = ld_hl_u8(gb);
        break;
    case 0x37:
        mcycles = scf(gb);
        break;
    case 0x38:
        mcycles = jr_cc_e8(gb, get_c(&gb->cpu) == 1);
        break;
    case 0x39:
        mcycles = add_hl_sp(gb);
        break;
    case 0x3A:
        mcycles = ldd_a_hl(gb);
        break;
    case 0x3B:
        mcycles = dec_sp(gb);
        break;
    case 0x3C:
        mcycles = inc_r(gb, &gb->cpu.a);
        break;
    case 0x3D:
        mcycles = dec_r(gb, &gb->cpu.a);
        break;
    case 0x3E:
        mcycles = ld_r_u8(gb, &gb->cpu.a);
        break;
    case 0x3F:
        mcycles = ccf(gb);
        break;
    case 0x40:
        mcycles = ld_r_r(gb, &gb->cpu.b, &gb->cpu.b);
        break;
    case 0x41:
        mcycles = ld_r_r(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0x42:
        mcycles = ld_r_r(gb, &gb->cpu.b, &gb->cpu.d);
        break;
    case 0x43:
        mcycles = ld_r_r(gb, &gb->cpu.b, &gb->cpu.e);
        break;
    case 0x44:
        mcycles = ld_r_r(gb, &gb->cpu.b, &gb->cpu.h);
        break;
    case 0x45:
        mcycles = ld_r_r(gb, &gb->cpu.b, &gb->cpu.l);
        break;
    case 0x46:
        mcycles = ld_r_hl(gb, &gb->cpu.b);
        break;
    case 0x47:
        mcycles = ld_r_r(gb, &gb->cpu.b, &gb->cpu.a);
        break;
    case 0x48:
        mcycles = ld_r_r(gb, &gb->cpu.c, &gb->cpu.b);
        break;
    case 0x49:
        mcycles = ld_r_r(gb, &gb->cpu.c, &gb->cpu.c);
        break;
    case 0x4A:
        mcycles = ld_r_r(gb, &gb->cpu.c, &gb->cpu.d);
        break;
    case 0x4B:
        mcycles = ld_r_r(gb, &gb->cpu.c, &gb->cpu.e);
        break;
    case 0x4C:
        mcycles = ld_r_r(gb, &gb->cpu.c, &gb->cpu.h);
        break;
    case 0x4D:
        mcycles = ld_r_r(gb, &gb->cpu.c, &gb->cpu.l);
        break;
    case 0x4E:
        mcycles = ld_r_hl(gb, &gb->cpu.c);
        break;
    case 0x4F:
        mcycles = ld_r_r(gb, &gb->cpu.c, &gb->cpu.a);
        break;
    case 0x50:
        mcycles = ld_r_r(gb, &gb->cpu.d, &gb->cpu.b);
        break;
    case 0x51:
        mcycles = ld_r_r(gb, &gb->cpu.d, &gb->cpu.c);
        break;
    case 0x52:
        mcycles = ld_r_r(gb, &gb->cpu.d, &gb->cpu.d);
        break;
    case 0x53:
        mcycles = ld_r_r(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0x54:
        mcycles = ld_r_r(gb, &gb->cpu.d, &gb->cpu.h);
        break;
    case 0x55:
        mcycles = ld_r_r(gb, &gb->cpu.d, &gb->cpu.l);
        break;
    case 0x56:
        mcycles = ld_r_hl(gb, &gb->cpu.d);
        break;
    case 0x57:
        mcycles = ld_r_r(gb, &gb->cpu.d, &gb->cpu.a);
        break;
    case 0x58:
        mcycles = ld_r_r(gb, &gb->cpu.e, &gb->cpu.b);
        break;
    case 0x59:
        mcycles = ld_r_r(gb, &gb->cpu.e, &gb->cpu.c);
        break;
    case 0x5A:
        mcycles = ld_r_r(gb, &gb->cpu.e, &gb->cpu.d);
        break;
    case 0x5B:
        mcycles = ld_r_r(gb, &gb->cpu.e, &gb->cpu.e);
        break;
    case 0x5C:
        mcycles = ld_r_r(gb, &gb->cpu.e, &gb->cpu.h);
        break;
    case 0x5D:
        mcycles = ld_r_r(gb, &gb->cpu.e, &gb->cpu.l);
        break;
    case 0x5E:
        mcycles = ld_r_hl(gb, &gb->cpu.e);
        break;
    case 0x5F:
        mcycles = ld_r_r(gb, &gb->cpu.e, &gb->cpu.a);
        break;
    case 0x60:
        mcycles = ld_r_r(gb, &gb->cpu.h, &gb->cpu.b);
        break;
    case 0x61:
        mcycles = ld_r_r(gb, &gb->cpu.h, &gb->cpu.c);
        break;
    case 0x62:
        mcycles = ld_r_r(gb, &gb->cpu.h, &gb->cpu.d);
        break;
    case 0x63:
        mcycles = ld_r_r(gb, &gb->cpu.h, &gb->cpu.e);
        break;
    case 0x64:
        mcycles = ld_r_r(gb, &gb->cpu.h, &gb->cpu.h);
        break;
    case 0x65:
        mcycles = ld_r_r(gb, &gb->cpu.h, &gb->cpu.l);
        break;
    case 0x66:
        mcycles = ld_r_hl(gb, &gb->cpu.h);
        break;
    case 0x67:
        mcycles = ld_r_r(gb, &gb->cpu.h, &gb->cpu.a);
        break;
    case 0x68:
        mcycles = ld_r_r(gb, &gb->cpu.l, &gb->cpu.b);
        break;
    case 0x69:
        mcycles = ld_r_r(gb, &gb->cpu.l, &gb->cpu.c);
        break;
    case 0x6A:
        mcycles = ld_r_r(gb, &gb->cpu.l, &gb->cpu.d);
        break;
    case 0x6B:
        mcycles = ld_r_r(gb, &gb->cpu.l, &gb->cpu.e);
        break;
    case 0x6C:
        mcycles = ld_r_r(gb, &gb->cpu.l, &gb->cpu.h);
        break;
    case 0x6D:
        mcycles = ld_r_r(gb, &gb->cpu.l, &gb->cpu.l);
        break;
    case 0x6E:
        mcycles = ld_r_hl(gb, &gb->cpu.l);
        break;
    case 0x6F:
        mcycles = ld_r_r(gb, &gb->cpu.l, &gb->cpu.a);
        break;
    case 0x70:
        mcycles = ld_hl_r(gb, &gb->cpu.b);
        break;
    case 0x71:
        mcycles = ld_hl_r(gb, &gb->cpu.c);
        break;
    case 0x72:
        mcycles = ld_hl_r(gb, &gb->cpu.d);
        break;
    case 0x73:
        mcycles = ld_hl_r(gb, &gb->cpu.e);
        break;
    case 0x74:
        mcycles = ld_hl_r(gb, &gb->cpu.h);
        break;
    case 0x75:
        mcycles = ld_hl_r(gb, &gb->cpu.l);
        break;
    case 0x76:
        mcycles = halt(gb);
        break;
    case 0x77:
        mcycles = ld_hl_r(gb, &gb->cpu.a);
        break;
    case 0x78:
        mcycles = ld_r_r(gb, &gb->cpu.a, &gb->cpu.b);
        break;
    case 0x79:
        mcycles = ld_r_r(gb, &gb->cpu.a, &gb->cpu.c);
        break;
    case 0x7A:
        mcycles = ld_r_r(gb, &gb->cpu.a, &gb->cpu.d);
        break;
    case 0x7B:
        mcycles = ld_r_r(gb, &gb->cpu.a, &gb->cpu.e);
        break;
    case 0x7C:
        mcycles = ld_r_r(gb, &gb->cpu.a, &gb->cpu.h);
        break;
    case 0x7D:
        mcycles = ld_r_r(gb, &gb->cpu.a, &gb->cpu.l);
        break;
    case 0x7E:
        mcycles = ld_r_hl(gb, &gb->cpu.a);
        break;
    case 0x7F:
        mcycles = ld_r_r(gb, &gb->cpu.a, &gb->cpu.a);
        break;
    case 0x80:
        mcycles = add_a_r(gb, &gb->cpu.b);
        break;
    case 0x81:
        mcycles = add_a_r(gb, &gb->cpu.c);
        break;
    case 0x82:
        mcycles = add_a_r(gb, &gb->cpu.d);
        break;
    case 0x83:
        mcycles = add_a_r(gb, &gb->cpu.e);
        break;
    case 0x84:
        mcycles = add_a_r(gb, &gb->cpu.h);
        break;
    case 0x85:
        mcycles = add_a_r(gb, &gb->cpu.l);
        break;
    case 0x86:
        mcycles = add_a_hl(gb);
        break;
    case 0x87:
        mcycles = add_a_r(gb, &gb->cpu.a);
        break;
    case 0x88:
        mcycles = adc_a_r(gb, &gb->cpu.b);
        break;
    case 0x89:
        mcycles = adc_a_r(gb, &gb->cpu.c);
        break;
    case 0x8A:
        mcycles = adc_a_r(gb, &gb->cpu.d);
        break;
    case 0x8B:
        mcycles = adc_a_r(gb, &gb->cpu.e);
        break;
    case 0x8C:
        mcycles = adc_a_r(gb, &gb->cpu.h);
        break;
    case 0x8D:
        mcycles = adc_a_r(gb, &gb->cpu.l);
        break;
    case 0x8E:
        mcycles = adc_a_hl(gb);
        break;
    case 0x8F:
        mcycles = adc_a_r(gb, &gb->cpu.a);
        break;
    case 0x90:
        mcycles = sub_a_r(gb, &gb->cpu.b);
        break;
    case 0x91:
        mcycles = sub_a_r(gb, &gb->cpu.c);
        break;
    case 0x92:
        mcycles = sub_a_r(gb, &gb->cpu.d);
        break;
    case 0x93:
        mcycles = sub_a_r(gb, &gb->cpu.e);
        break;
    case 0x94:
        mcycles = sub_a_r(gb, &gb->cpu.h);
        break;
    case 0x95:
        mcycles = sub_a_r(gb, &gb->cpu.l);
        break;
    case 0x96:
        mcycles = sub_a_hl(gb);
        break;
    case 0x97:
        mcycles = sub_a_r(gb, &gb->cpu.a);
        break;
    case 0x98:
        mcycles = sbc_a_r(gb, &gb->cpu.b);
        break;
    case 0x99:
        mcycles = sbc_a_r(gb, &gb->cpu.c);
        break;
    case 0x9A:
        mcycles = sbc_a_r(gb, &gb->cpu.d);
        break;
    case 0x9B:
        mcycles = sbc_a_r(gb, &gb->cpu.e);
        break;
    case 0x9C:
        mcycles = sbc_a_r(gb, &gb->cpu.h);
        break;
    case 0x9D:
        mcycles = sbc_a_r(gb, &gb->cpu.l);
        break;
    case 0x9E:
        mcycles = sbc_a_hl(gb);
        break;
    case 0x9F:
        mcycles = sbc_a_r(gb, &gb->cpu.a);
        break;
    case 0xA0:
        mcycles = and_a_r(gb, &gb->cpu.b);
        break;
    case 0xA1:
        mcycles = and_a_r(gb, &gb->cpu.c);
        break;
    case 0xA2:
        mcycles = and_a_r(gb, &gb->cpu.d);
        break;
    case 0xA3:
        mcycles = and_a_r(gb, &gb->cpu.e);
        break;
    case 0xA4:
        mcycles = and_a_r(gb, &gb->cpu.h);
        break;
    case 0xA5:
        mcycles = and_a_r(gb, &gb->cpu.l);
        break;
    case 0xA6:
        mcycles = and_a_hl(gb);
        break;
    case 0xA7:
        mcycles = and_a_r(gb, &gb->cpu.a);
        break;
    case 0xA8:
        mcycles = xor_a_r(gb, &gb->cpu.b);
        break;
    case 0xA9:
        mcycles = xor_a_r(gb, &gb->cpu.c);
        break;
    case 0xAA:
        mcycles = xor_a_r(gb, &gb->cpu.d);
        break;
    case 0xAB:
        mcycles = xor_a_r(gb, &gb->cpu.e);
        break;
    case 0xAC:
        mcycles = xor_a_r(gb, &gb->cpu.h);
        break;
    case 0xAD:
        mcycles = xor_a_r(gb, &gb->cpu.l);
        break;
    case 0xAE:
        mcycles = xor_a_hl(gb);
        break;
    case 0xAF:
        mcycles = xor_a_r(gb, &gb->cpu.a);
        break;
    case 0xB0:
        mcycles = or_a_r(gb, &gb->cpu.b);
        break;
    case 0xB1:
        mcycles = or_a_r(gb, &gb->cpu.c);
        break;
    case 0xB2:
        mcycles = or_a_r(gb, &gb->cpu.d);
        break;
    case 0xB3:
        mcycles = or_a_r(gb, &gb->cpu.e);
        break;
    case 0xB4:
        mcycles = or_a_r(gb, &gb->cpu.h);
        break;
    case 0xB5:
        mcycles = or_a_r(gb, &gb->cpu.l);
        break;
    case 0xB6:
        mcycles = or_a_hl(gb);
        break;
    case 0xB7:
        mcycles = or_a_r(gb, &gb->cpu.a);
        break;
    case 0xB8:
        mcycles = cp_a_r(gb, &gb->cpu.b);
        break;
    case 0xB9:
        mcycles = cp_a_r(gb, &gb->cpu.c);
        break;
    case 0xBA:
        mcycles = cp_a_r(gb, &gb->cpu.d);
        break;
    case 0xBB:
        mcycles = cp_a_r(gb, &gb->cpu.e);
        break;
    case 0xBC:
        mcycles = cp_a_r(gb, &gb->cpu.h);
        break;
    case 0xBD:
        mcycles = cp_a_r(gb, &gb->cpu.l);
        break;
    case 0xBE:
        mcycles = cp_a_hl(gb);
        break;
    case 0xBF:
        mcycles = cp_a_r(gb, &gb->cpu.a);
        break;
    case 0xC0:
        mcycles = ret_cc(gb, get_z(&gb->cpu) == 0);
        break;
    case 0xC1:
        mcycles = pop_rr(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0xC2:
        mcycles = jp_cc_nn(gb, get_z(&gb->cpu) == 0);
        break;
    case 0xC3:
        mcycles = jp_nn(gb);
        break;
    case 0xC4:
        mcycles = call_cc_nn(gb, get_z(&gb->cpu) == 0);
        break;
    case 0xC5:
        mcycles = push_rr(gb, &gb->cpu.b, &gb->cpu.c);
        break;
    case 0xC6:
        mcycles = add_a_n(gb);
        break;
    case 0xC7:
        mcycles = rst(gb, 0x00);
        break;
    case 0xC8:
        mcycles = ret_cc(gb, get_z(&gb->cpu) == 1);
        break;
    case 0xC9:
        mcycles = ret(gb);
        break;
    case 0xCA:
        mcycles = jp_cc_nn(gb, get_z(&gb->cpu) == 1);
        break;
    case 0xCB:
        mcycles = prefix_op(gb);
        break;
    case 0xCC:
        mcycles = call_cc_nn(gb, get_z(&gb->cpu) == 1);
        break;
    case 0xCD:
        mcycles = call_nn(gb);
        break;
    case 0xCE:
        mcycles = adc_a_n(gb);
        break;
    case 0xCF:
        mcycles = rst(gb, 0x08);
        break;
    case 0xD0:
        mcycles = ret_cc(gb, get_c(&gb->cpu) == 0);
        break;
    case 0xD1:
        mcycles = pop_rr(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0xD2:
        mcycles = jp_cc_nn(gb, get_c(&gb->cpu) == 0);
        break;
    case 0xD4:
        mcycles = call_cc_nn(gb, get_c(&gb->cpu) == 0);
        break;
    case 0xD5:
        mcycles = push_rr(gb, &gb->cpu.d, &gb->cpu.e);
        break;
    case 0xD6:
        mcycles = sub_a_n(gb);
        break;
    case 0xD7:
        mcycles = rst(gb, 0x10);
        break;
    case 0xD8:
        mcycles = ret_cc(gb, get_c(&gb->cpu) == 1);
        break;
    case 0xD9:
        mcycles = reti(gb);
        break;
    case 0xDA:
        mcycles = jp_cc_nn(gb, get_c(&gb->cpu) == 1);
        break;
    case 0xDC:
        mcycles = call_cc_nn(gb, get_c(&gb->cpu) == 1);
        break;
    case 0xDE:
        mcycles = sbc_a_n(gb);
        break;
    case 0xDF:
        mcycles = rst(gb, 0x18);
        break;
    case 0xE0:
        mcycles = ldh_n_a(gb);
        break;
    case 0xE1:
        mcycles = pop_rr(gb, &gb->cpu.h, &gb->cpu.l);
        break;
    case 0xE2:
        mcycles = ldh_c_a(gb);
        break;
    case 0xE5:
        mcycles = push_rr(gb, &gb->cpu.h, &gb->cpu.l);
        break;
    case 0xE6:
        mcycles = and_a_n(gb);
        break;
    case 0xE7:
        mcycles = rst(gb, 0x20);
        break;
    case 0xE8:
        mcycles = add_sp_e8(gb);
        break;
    case 0xE9:
        mcycles = jp_hl(gb);
        break;
    case 0xEA:
        mcycles = ld_nn_a(gb);
        break;
    case 0xEE:
        mcycles = xor_a_n(gb);
        break;
    case 0xEF:
        mcycles = rst(gb, 0x28);
        break;
    case 0xF0:
        mcycles = ldh_a_n(gb);
        break;
    case 0xF1:
        mcycles = pop_af(gb);
        break;
    case 0xF2:
        mcycles = ldh_a_c(gb);
        break;
    case 0xF3:
        mcycles = di(gb);
        break;
    case 0xF5:
        mcycles = push_rr(gb, &gb->cpu.a, &gb->cpu.f);
        break;
    case 0xF6:
        mcycles = or_a_n(gb);
        break;
    case 0xF7:
        mcycles = rst(gb, 0x30);
        break;
    case 0xF8:
        mcycles = ld_hl_spe8(gb);
        break;
    case 0xF9:
        mcycles = ld_sp_hl(gb);
        break;
    case 0xFA:
        mcycles = ld_a_nn(gb);
        break;
    case 0xFB:
        mcycles = ei(gb);
        break;
    case 0xFE:
        mcycles = cp_a_n(gb);
        break;
    case 0xFF:
        mcycles = rst(gb, 0x38);
        break;
    default:
        errx(-2, "ERROR: undefined opcode at PC=0x%X, closing emulator...", gb->cpu.pc - 1);
        break;
    }
    return mcycles;
}

static int prefix_op(struct gb_core *gb)
{
    // Fetch the prefix opcode
    uint8_t opcode = read_mem_tick(gb, gb->cpu.pc++);
    int mcycles = 0;
    switch (opcode)
    {
    case 0x00:
        mcycles = rlc(gb, &gb->cpu.b);
        break;
    case 0x01:
        mcycles = rlc(gb, &gb->cpu.c);
        break;
    case 0x02:
        mcycles = rlc(gb, &gb->cpu.d);
        break;
    case 0x03:
        mcycles = rlc(gb, &gb->cpu.e);
        break;
    case 0x04:
        mcycles = rlc(gb, &gb->cpu.h);
        break;
    case 0x05:
        mcycles = rlc(gb, &gb->cpu.l);
        break;
    case 0x06:
        mcycles = rlc_hl(gb);
        break;
    case 0x07:
        mcycles = rlc(gb, &gb->cpu.a);
        break;
    case 0x08:
        mcycles = rrc(gb, &gb->cpu.b);
        break;
    case 0x09:
        mcycles = rrc(gb, &gb->cpu.c);
        break;
    case 0x0A:
        mcycles = rrc(gb, &gb->cpu.d);
        break;
    case 0x0B:
        mcycles = rrc(gb, &gb->cpu.e);
        break;
    case 0x0C:
        mcycles = rrc(gb, &gb->cpu.h);
        break;
    case 0x0D:
        mcycles = rrc(gb, &gb->cpu.l);
        break;
    case 0x0E:
        mcycles = rrc_hl(gb);
        break;
    case 0x0F:
        mcycles = rrc(gb, &gb->cpu.a);
        break;
    case 0x10:
        mcycles = rl(gb, &gb->cpu.b);
        break;
    case 0x11:
        mcycles = rl(gb, &gb->cpu.c);
        break;
    case 0x12:
        mcycles = rl(gb, &gb->cpu.d);
        break;
    case 0x13:
        mcycles = rl(gb, &gb->cpu.e);
        break;
    case 0x14:
        mcycles = rl(gb, &gb->cpu.h);
        break;
    case 0x15:
        mcycles = rl(gb, &gb->cpu.l);
        break;
    case 0x16:
        mcycles = rl_hl(gb);
        break;
    case 0x17:
        mcycles = rl(gb, &gb->cpu.a);
        break;
    case 0x18:
        mcycles = rr(gb, &gb->cpu.b);
        break;
    case 0x19:
        mcycles = rr(gb, &gb->cpu.c);
        break;
    case 0x1A:
        mcycles = rr(gb, &gb->cpu.d);
        break;
    case 0x1B:
        mcycles = rr(gb, &gb->cpu.e);
        break;
    case 0x1C:
        mcycles = rr(gb, &gb->cpu.h);
        break;
    case 0x1D:
        mcycles = rr(gb, &gb->cpu.l);
        break;
    case 0x1E:
        mcycles = rr_hl(gb);
        break;
    case 0x1F:
        mcycles = rr(gb, &gb->cpu.a);
        break;
    case 0x20:
        mcycles = sla(gb, &gb->cpu.b);
        break;
    case 0x21:
        mcycles = sla(gb, &gb->cpu.c);
        break;
    case 0x22:
        mcycles = sla(gb, &gb->cpu.d);
        break;
    case 0x23:
        mcycles = sla(gb, &gb->cpu.e);
        break;
    case 0x24:
        mcycles = sla(gb, &gb->cpu.h);
        break;
    case 0x25:
        mcycles = sla(gb, &gb->cpu.l);
        break;
    case 0x26:
        mcycles = sla_hl(gb);
        break;
    case 0x27:
        mcycles = sla(gb, &gb->cpu.a);
        break;
    case 0x28:
        mcycles = sra(gb, &gb->cpu.b);
        break;
    case 0x29:
        mcycles = sra(gb, &gb->cpu.c);
        break;
    case 0x2A:
        mcycles = sra(gb, &gb->cpu.d);
        break;
    case 0x2B:
        mcycles = sra(gb, &gb->cpu.e);
        break;
    case 0x2C:
        mcycles = sra(gb, &gb->cpu.h);
        break;
    case 0x2D:
        mcycles = sra(gb, &gb->cpu.l);
        break;
    case 0x2E:
        mcycles = sra_hl(gb);
        break;
    case 0x2F:
        mcycles = sra(gb, &gb->cpu.a);
        break;
    case 0x30:
        mcycles = swap(gb, &gb->cpu.b);
        break;
    case 0x31:
        mcycles = swap(gb, &gb->cpu.c);
        break;
    case 0x32:
        mcycles = swap(gb, &gb->cpu.d);
        break;
    case 0x33:
        mcycles = swap(gb, &gb->cpu.e);
        break;
    case 0x34:
        mcycles = swap(gb, &gb->cpu.h);
        break;
    case 0x35:
        mcycles = swap(gb, &gb->cpu.l);
        break;
    case 0x36:
        mcycles = swap_hl(gb);
        break;
    case 0x37:
        mcycles = swap(gb, &gb->cpu.a);
        break;
    case 0x38:
        mcycles = srl(gb, &gb->cpu.b);
        break;
    case 0x39:
        mcycles = srl(gb, &gb->cpu.c);
        break;
    case 0x3A:
        mcycles = srl(gb, &gb->cpu.d);
        break;
    case 0x3B:
        mcycles = srl(gb, &gb->cpu.e);
        break;
    case 0x3C:
        mcycles = srl(gb, &gb->cpu.h);
        break;
    case 0x3D:
        mcycles = srl(gb, &gb->cpu.l);
        break;
    case 0x3E:
        mcycles = srl_hl(gb);
        break;
    case 0x3F:
        mcycles = srl(gb, &gb->cpu.a);
        break;
    case 0x40:
        mcycles = bit(gb, &gb->cpu.b, 0);
        break;
    case 0x41:
        mcycles = bit(gb, &gb->cpu.c, 0);
        break;
    case 0x42:
        mcycles = bit(gb, &gb->cpu.d, 0);
        break;
    case 0x43:
        mcycles = bit(gb, &gb->cpu.e, 0);
        break;
    case 0x44:
        mcycles = bit(gb, &gb->cpu.h, 0);
        break;
    case 0x45:
        mcycles = bit(gb, &gb->cpu.l, 0);
        break;
    case 0x46:
        mcycles = bit_hl(gb, 0);
        break;
    case 0x47:
        mcycles = bit(gb, &gb->cpu.a, 0);
        break;
    case 0x48:
        mcycles = bit(gb, &gb->cpu.b, 1);
        break;
    case 0x49:
        mcycles = bit(gb, &gb->cpu.c, 1);
        break;
    case 0x4A:
        mcycles = bit(gb, &gb->cpu.d, 1);
        break;
    case 0x4B:
        mcycles = bit(gb, &gb->cpu.e, 1);
        break;
    case 0x4C:
        mcycles = bit(gb, &gb->cpu.h, 1);
        break;
    case 0x4D:
        mcycles = bit(gb, &gb->cpu.l, 1);
        break;
    case 0x4E:
        mcycles = bit_hl(gb, 1);
        break;
    case 0x4F:
        mcycles = bit(gb, &gb->cpu.a, 1);
        break;
    case 0x50:
        mcycles = bit(gb, &gb->cpu.b, 2);
        break;
    case 0x51:
        mcycles = bit(gb, &gb->cpu.c, 2);
        break;
    case 0x52:
        mcycles = bit(gb, &gb->cpu.d, 2);
        break;
    case 0x53:
        mcycles = bit(gb, &gb->cpu.e, 2);
        break;
    case 0x54:
        mcycles = bit(gb, &gb->cpu.h, 2);
        break;
    case 0x55:
        mcycles = bit(gb, &gb->cpu.l, 2);
        break;
    case 0x56:
        mcycles = bit_hl(gb, 2);
        break;
    case 0x57:
        mcycles = bit(gb, &gb->cpu.a, 2);
        break;
    case 0x58:
        mcycles = bit(gb, &gb->cpu.b, 3);
        break;
    case 0x59:
        mcycles = bit(gb, &gb->cpu.c, 3);
        break;
    case 0x5A:
        mcycles = bit(gb, &gb->cpu.d, 3);
        break;
    case 0x5B:
        mcycles = bit(gb, &gb->cpu.e, 3);
        break;
    case 0x5C:
        mcycles = bit(gb, &gb->cpu.h, 3);
        break;
    case 0x5D:
        mcycles = bit(gb, &gb->cpu.l, 3);
        break;
    case 0x5E:
        mcycles = bit_hl(gb, 3);
        break;
    case 0x5F:
        mcycles = bit(gb, &gb->cpu.a, 3);
        break;
    case 0x60:
        mcycles = bit(gb, &gb->cpu.b, 4);
        break;
    case 0x61:
        mcycles = bit(gb, &gb->cpu.c, 4);
        break;
    case 0x62:
        mcycles = bit(gb, &gb->cpu.d, 4);
        break;
    case 0x63:
        mcycles = bit(gb, &gb->cpu.e, 4);
        break;
    case 0x64:
        mcycles = bit(gb, &gb->cpu.h, 4);
        break;
    case 0x65:
        mcycles = bit(gb, &gb->cpu.l, 4);
        break;
    case 0x66:
        mcycles = bit_hl(gb, 4);
        break;
    case 0x67:
        mcycles = bit(gb, &gb->cpu.a, 4);
        break;
    case 0x68:
        mcycles = bit(gb, &gb->cpu.b, 5);
        break;
    case 0x69:
        mcycles = bit(gb, &gb->cpu.c, 5);
        break;
    case 0x6A:
        mcycles = bit(gb, &gb->cpu.d, 5);
        break;
    case 0x6B:
        mcycles = bit(gb, &gb->cpu.e, 5);
        break;
    case 0x6C:
        mcycles = bit(gb, &gb->cpu.h, 5);
        break;
    case 0x6D:
        mcycles = bit(gb, &gb->cpu.l, 5);
        break;
    case 0x6E:
        mcycles = bit_hl(gb, 5);
        break;
    case 0x6F:
        mcycles = bit(gb, &gb->cpu.a, 5);
        break;
    case 0x70:
        mcycles = bit(gb, &gb->cpu.b, 6);
        break;
    case 0x71:
        mcycles = bit(gb, &gb->cpu.c, 6);
        break;
    case 0x72:
        mcycles = bit(gb, &gb->cpu.d, 6);
        break;
    case 0x73:
        mcycles = bit(gb, &gb->cpu.e, 6);
        break;
    case 0x74:
        mcycles = bit(gb, &gb->cpu.h, 6);
        break;
    case 0x75:
        mcycles = bit(gb, &gb->cpu.l, 6);
        break;
    case 0x76:
        mcycles = bit_hl(gb, 6);
        break;
    case 0x77:
        mcycles = bit(gb, &gb->cpu.a, 6);
        break;
    case 0x78:
        mcycles = bit(gb, &gb->cpu.b, 7);
        break;
    case 0x79:
        mcycles = bit(gb, &gb->cpu.c, 7);
        break;
    case 0x7A:
        mcycles = bit(gb, &gb->cpu.d, 7);
        break;
    case 0x7B:
        mcycles = bit(gb, &gb->cpu.e, 7);
        break;
    case 0x7C:
        mcycles = bit(gb, &gb->cpu.h, 7);
        break;
    case 0x7D:
        mcycles = bit(gb, &gb->cpu.l, 7);
        break;
    case 0x7E:
        mcycles = bit_hl(gb, 7);
        break;
    case 0x7F:
        mcycles = bit(gb, &gb->cpu.a, 7);
        break;
    case 0x80:
        mcycles = res(&gb->cpu.b, 0);
        break;
    case 0x81:
        mcycles = res(&gb->cpu.c, 0);
        break;
    case 0x82:
        mcycles = res(&gb->cpu.d, 0);
        break;
    case 0x83:
        mcycles = res(&gb->cpu.e, 0);
        break;
    case 0x84:
        mcycles = res(&gb->cpu.h, 0);
        break;
    case 0x85:
        mcycles = res(&gb->cpu.l, 0);
        break;
    case 0x86:
        mcycles = res_hl(gb, 0);
        break;
    case 0x87:
        mcycles = res(&gb->cpu.a, 0);
        break;
    case 0x88:
        mcycles = res(&gb->cpu.b, 1);
        break;
    case 0x89:
        mcycles = res(&gb->cpu.c, 1);
        break;
    case 0x8A:
        mcycles = res(&gb->cpu.d, 1);
        break;
    case 0x8B:
        mcycles = res(&gb->cpu.e, 1);
        break;
    case 0x8C:
        mcycles = res(&gb->cpu.h, 1);
        break;
    case 0x8D:
        mcycles = res(&gb->cpu.l, 1);
        break;
    case 0x8E:
        mcycles = res_hl(gb, 1);
        break;
    case 0x8F:
        mcycles = res(&gb->cpu.a, 1);
        break;
    case 0x90:
        mcycles = res(&gb->cpu.b, 2);
        break;
    case 0x91:
        mcycles = res(&gb->cpu.c, 2);
        break;
    case 0x92:
        mcycles = res(&gb->cpu.d, 2);
        break;
    case 0x93:
        mcycles = res(&gb->cpu.e, 2);
        break;
    case 0x94:
        mcycles = res(&gb->cpu.h, 2);
        break;
    case 0x95:
        mcycles = res(&gb->cpu.l, 2);
        break;
    case 0x96:
        mcycles = res_hl(gb, 2);
        break;
    case 0x97:
        mcycles = res(&gb->cpu.a, 2);
        break;
    case 0x98:
        mcycles = res(&gb->cpu.b, 3);
        break;
    case 0x99:
        mcycles = res(&gb->cpu.c, 3);
        break;
    case 0x9A:
        mcycles = res(&gb->cpu.d, 3);
        break;
    case 0x9B:
        mcycles = res(&gb->cpu.e, 3);
        break;
    case 0x9C:
        mcycles = res(&gb->cpu.h, 3);
        break;
    case 0x9D:
        mcycles = res(&gb->cpu.l, 3);
        break;
    case 0x9E:
        mcycles = res_hl(gb, 3);
        break;
    case 0x9F:
        mcycles = res(&gb->cpu.a, 3);
        break;
    case 0xA0:
        mcycles = res(&gb->cpu.b, 4);
        break;
    case 0xA1:
        mcycles = res(&gb->cpu.c, 4);
        break;
    case 0xA2:
        mcycles = res(&gb->cpu.d, 4);
        break;
    case 0xA3:
        mcycles = res(&gb->cpu.e, 4);
        break;
    case 0xA4:
        mcycles = res(&gb->cpu.h, 4);
        break;
    case 0xA5:
        mcycles = res(&gb->cpu.l, 4);
        break;
    case 0xA6:
        mcycles = res_hl(gb, 4);
        break;
    case 0xA7:
        mcycles = res(&gb->cpu.a, 4);
        break;
    case 0xA8:
        mcycles = res(&gb->cpu.b, 5);
        break;
    case 0xA9:
        mcycles = res(&gb->cpu.c, 5);
        break;
    case 0xAA:
        mcycles = res(&gb->cpu.d, 5);
        break;
    case 0xAB:
        mcycles = res(&gb->cpu.e, 5);
        break;
    case 0xAC:
        mcycles = res(&gb->cpu.h, 5);
        break;
    case 0xAD:
        mcycles = res(&gb->cpu.l, 5);
        break;
    case 0xAE:
        mcycles = res_hl(gb, 5);
        break;
    case 0xAF:
        mcycles = res(&gb->cpu.a, 5);
        break;
    case 0xB0:
        mcycles = res(&gb->cpu.b, 6);
        break;
    case 0xB1:
        mcycles = res(&gb->cpu.c, 6);
        break;
    case 0xB2:
        mcycles = res(&gb->cpu.d, 6);
        break;
    case 0xB3:
        mcycles = res(&gb->cpu.e, 6);
        break;
    case 0xB4:
        mcycles = res(&gb->cpu.h, 6);
        break;
    case 0xB5:
        mcycles = res(&gb->cpu.l, 6);
        break;
    case 0xB6:
        mcycles = res_hl(gb, 6);
        break;
    case 0xB7:
        mcycles = res(&gb->cpu.a, 6);
        break;
    case 0xB8:
        mcycles = res(&gb->cpu.b, 7);
        break;
    case 0xB9:
        mcycles = res(&gb->cpu.c, 7);
        break;
    case 0xBA:
        mcycles = res(&gb->cpu.d, 7);
        break;
    case 0xBB:
        mcycles = res(&gb->cpu.e, 7);
        break;
    case 0xBC:
        mcycles = res(&gb->cpu.h, 7);
        break;
    case 0xBD:
        mcycles = res(&gb->cpu.l, 7);
        break;
    case 0xBE:
        mcycles = res_hl(gb, 7);
        break;
    case 0xBF:
        mcycles = res(&gb->cpu.a, 7);
        break;
    case 0xC0:
        mcycles = set(&gb->cpu.b, 0);
        break;
    case 0xC1:
        mcycles = set(&gb->cpu.c, 0);
        break;
    case 0xC2:
        mcycles = set(&gb->cpu.d, 0);
        break;
    case 0xC3:
        mcycles = set(&gb->cpu.e, 0);
        break;
    case 0xC4:
        mcycles = set(&gb->cpu.h, 0);
        break;
    case 0xC5:
        mcycles = set(&gb->cpu.l, 0);
        break;
    case 0xC6:
        mcycles = set_hl(gb, 0);
        break;
    case 0xC7:
        mcycles = set(&gb->cpu.a, 0);
        break;
    case 0xC8:
        mcycles = set(&gb->cpu.b, 1);
        break;
    case 0xC9:
        mcycles = set(&gb->cpu.c, 1);
        break;
    case 0xCA:
        mcycles = set(&gb->cpu.d, 1);
        break;
    case 0xCB:
        mcycles = set(&gb->cpu.e, 1);
        break;
    case 0xCC:
        mcycles = set(&gb->cpu.h, 1);
        break;
    case 0xCD:
        mcycles = set(&gb->cpu.l, 1);
        break;
    case 0xCE:
        mcycles = set_hl(gb, 1);
        break;
    case 0xCF:
        mcycles = set(&gb->cpu.a, 1);
        break;
    case 0xD0:
        mcycles = set(&gb->cpu.b, 2);
        break;
    case 0xD1:
        mcycles = set(&gb->cpu.c, 2);
        break;
    case 0xD2:
        mcycles = set(&gb->cpu.d, 2);
        break;
    case 0xD3:
        mcycles = set(&gb->cpu.e, 2);
        break;
    case 0xD4:
        mcycles = set(&gb->cpu.h, 2);
        break;
    case 0xD5:
        mcycles = set(&gb->cpu.l, 2);
        break;
    case 0xD6:
        mcycles = set_hl(gb, 2);
        break;
    case 0xD7:
        mcycles = set(&gb->cpu.a, 2);
        break;
    case 0xD8:
        mcycles = set(&gb->cpu.b, 3);
        break;
    case 0xD9:
        mcycles = set(&gb->cpu.c, 3);
        break;
    case 0xDA:
        mcycles = set(&gb->cpu.d, 3);
        break;
    case 0xDB:
        mcycles = set(&gb->cpu.e, 3);
        break;
    case 0xDC:
        mcycles = set(&gb->cpu.h, 3);
        break;
    case 0xDD:
        mcycles = set(&gb->cpu.l, 3);
        break;
    case 0xDE:
        mcycles = set_hl(gb, 3);
        break;
    case 0xDF:
        mcycles = set(&gb->cpu.a, 3);
        break;
    case 0xE0:
        mcycles = set(&gb->cpu.b, 4);
        break;
    case 0xE1:
        mcycles = set(&gb->cpu.c, 4);
        break;
    case 0xE2:
        mcycles = set(&gb->cpu.d, 4);
        break;
    case 0xE3:
        mcycles = set(&gb->cpu.e, 4);
        break;
    case 0xE4:
        mcycles = set(&gb->cpu.h, 4);
        break;
    case 0xE5:
        mcycles = set(&gb->cpu.l, 4);
        break;
    case 0xE6:
        mcycles = set_hl(gb, 4);
        break;
    case 0xE7:
        mcycles = set(&gb->cpu.a, 4);
        break;
    case 0xE8:
        mcycles = set(&gb->cpu.b, 5);
        break;
    case 0xE9:
        mcycles = set(&gb->cpu.c, 5);
        break;
    case 0xEA:
        mcycles = set(&gb->cpu.d, 5);
        break;
    case 0xEB:
        mcycles = set(&gb->cpu.e, 5);
        break;
    case 0xEC:
        mcycles = set(&gb->cpu.h, 5);
        break;
    case 0xED:
        mcycles = set(&gb->cpu.l, 5);
        break;
    case 0xEE:
        mcycles = set_hl(gb, 5);
        break;
    case 0xEF:
        mcycles = set(&gb->cpu.a, 5);
        break;
    case 0xF0:
        mcycles = set(&gb->cpu.b, 6);
        break;
    case 0xF1:
        mcycles = set(&gb->cpu.c, 6);
        break;
    case 0xF2:
        mcycles = set(&gb->cpu.d, 6);
        break;
    case 0xF3:
        mcycles = set(&gb->cpu.e, 6);
        break;
    case 0xF4:
        mcycles = set(&gb->cpu.h, 6);
        break;
    case 0xF5:
        mcycles = set(&gb->cpu.l, 6);
        break;
    case 0xF6:
        mcycles = set_hl(gb, 6);
        break;
    case 0xF7:
        mcycles = set(&gb->cpu.a, 6);
        break;
    case 0xF8:
        mcycles = set(&gb->cpu.b, 7);
        break;
    case 0xF9:
        mcycles = set(&gb->cpu.c, 7);
        break;
    case 0xFA:
        mcycles = set(&gb->cpu.d, 7);
        break;
    case 0xFB:
        mcycles = set(&gb->cpu.e, 7);
        break;
    case 0xFC:
        mcycles = set(&gb->cpu.h, 7);
        break;
    case 0xFD:
        mcycles = set(&gb->cpu.l, 7);
        break;
    case 0xFE:
        mcycles = set_hl(gb, 7);
        break;
    case 0xFF:
        mcycles = set(&gb->cpu.a, 7);
        break;
    }
    return mcycles;
}
