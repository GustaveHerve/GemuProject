#include "write.h"

#include "emulation.h"
#include "gb_core.h"
#include "mbc_base.h"

static void _rom(struct gb_core *gb, uint16_t address, uint8_t val)
{
    write_mbc_rom(gb->mbc, address, val);
}

static void _vram(struct gb_core *gb, uint16_t address, uint8_t val)
{
    if (!gb->ppu.vram_locked)
        gb->memory.vram[VRAM_OFFSET(address)] = val;
}

static void _ex_ram(struct gb_core *gb, uint16_t address, uint8_t val)
{
    write_mbc_ram(gb->mbc, address, val);
}

static void _wram(struct gb_core *gb, uint16_t address, uint8_t val)
{
    gb->memory.wram[WRAM_OFFSET(address)] = val;
}

static void _echo_ram(struct gb_core *gb, uint16_t address, uint8_t val)
{
    gb->memory.wram[WRAM_OFFSET(address)] = val;
}

static void _oam(struct gb_core *gb, uint16_t address, uint8_t val)
{
    if (address >= 0xFEA0 && address <= 0xFEFF)
        return;
    if (!gb->ppu.oam_locked)
        gb->memory.oam[OAM_OFFSET(address)] = val;
}

static void _io(struct gb_core *gb, uint16_t address, uint8_t val)
{
    switch (address)
    {
    case DIV:
        gb->internal_div = 0;
        return;
    case NR10:
    case NR11:
    case NR12:
    case NR13:
    case NR21:
    case NR22:
    case NR23:
    case NR30:
    case NR31:
    case NR32:
    case NR33:
    case NR41:
    case NR42:
    case NR43:
    case NR50:
    case NR51:
        if (!is_apu_on(gb))
            return;
        break;
    case NR14:
    case NR24:
    case NR34:
    case NR44:
        if (!is_apu_on(gb))
            return;
        gb->memory.io[IO_OFFSET(address)] = val & ~(NRx4_UNUSED_PART);
        uint8_t ch_number = ((address - NR14) / (NR24 - NR14)) + 1;
        /* Trigger event */
        if (val & NRx4_TRIGGER_MASK)
        {
            static void (*trigger_handlers[])(struct gb_core *) = {
                handle_trigger_event_ch1,
                handle_trigger_event_ch2,
                handle_trigger_event_ch3,
                handle_trigger_event_ch4,
            };

            trigger_handlers[ch_number - 1](gb);
        }

        if (val & NRx4_LENGTH_ENABLE)
            enable_timer(gb, ch_number);
        return;
    case NR52:
        // APU off
        if (!(val >> 7))
        {
            apu_turn_off(gb);
            return;
        }
        break;
    case LCDC:
        // LCD off
        if (!(val >> 7))
            ppu_reset(gb);
        break;
    case DMA:
        gb->ppu.dma = 2;
        gb->ppu.dma_acc = 0;
        gb->ppu.dma_source = val;
        break;
    case BOOT:
        if (gb->memory.io[IO_OFFSET(BOOT)] & 0x01)
            return;
    }

    io_write(gb->memory.io, address, val);
}

static void _hram(struct gb_core *gb, uint16_t address, uint8_t val)
{
    gb->memory.hram[HRAM_OFFSET(address)] = val;
}

static void _ie(struct gb_core *gb, uint16_t address, uint8_t val)
{
    (void)address;
    uint8_t temp = (gb->memory.ie & 0xE0);
    temp |= (val & 0x1F);
    gb->memory.ie = temp;
}

static void _write_jmp_level_4(struct gb_core *gb, uint16_t address, uint8_t val)
{
    static void (*write_jmp_table_level_4[16])(struct gb_core *, uint16_t address, uint8_t val) = {
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _ie,
    };
    write_jmp_table_level_4[address & 0xF](gb, address, val);
}

static void _write_jmp_level_3(struct gb_core *gb, uint16_t address, uint8_t val)
{
    static void (*write_jmp_table_level_3[16])(struct gb_core *, uint16_t address, uint8_t val) = {
        _io,
        _io,
        _io,
        _io,
        _io,
        _io,
        _io,
        _io,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _hram,
        _write_jmp_level_4,
    };
    write_jmp_table_level_3[(address & 0x00F0) >> 4](gb, address, val);
}

static void _write_jmp_level_2(struct gb_core *gb, uint16_t address, uint8_t val)
{
    static void (*write_jmp_table_level_2[16])(struct gb_core *, uint16_t address, uint8_t val) = {
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _echo_ram,
        _oam,
        _write_jmp_level_3,
    };
    write_jmp_table_level_2[(address & 0x0F00) >> 8](gb, address, val);
}

static void _write_mem(struct gb_core *gb, uint16_t address, uint8_t val)
{
    static void (*write_jmp_table[16])(struct gb_core *, uint16_t address, uint8_t val) = {
        _rom,
        _rom,
        _rom,
        _rom,
        _rom,
        _rom,
        _rom,
        _rom,
        _vram,
        _vram,
        _ex_ram,
        _ex_ram,
        _wram,
        _wram,
        _echo_ram,
        _write_jmp_level_2,
    };
    write_jmp_table[(address & 0xF000) >> 12](gb, address, val);
}

void write_mem(struct gb_core *gb, uint16_t address, uint8_t val)
{
    _write_mem(gb, address, val);
    tick_m(gb);
}
