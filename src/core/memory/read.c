#include "read.h"

#include "emulation.h"
#include "gb_core.h"
#include "mbc_base.h"

static uint8_t _rom(struct gb_core *gb, uint16_t address)
{
    if (!(gb->membus[BOOT] & 0x01) && address <= 0x00FF)
        return gb->membus[address];
    return read_mbc_rom(gb->mbc, address);
}

static uint8_t _vram(struct gb_core *gb, uint16_t address)
{
    if (gb->ppu.vram_locked)
        return 0xFF;
    return gb->membus[address];
}

static uint8_t _ex_ram(struct gb_core *gb, uint16_t address)
{
    return read_mbc_ram(gb->mbc, address);
}

static uint8_t _wram(struct gb_core *gb, uint16_t address)
{
    return gb->membus[address];
}

static uint8_t _echo_ram(struct gb_core *gb, uint16_t address)
{
    return gb->membus[address - 0x2000];
}

static uint8_t _oam(struct gb_core *gb, uint16_t address)
{
    if (address >= 0xFEA0 && address <= 0xFEFF)
    {
        // TODO: forbidden area
    }
    if (gb->ppu.oam_locked)
        return 0xFF;
    return gb->membus[address];
}

static uint8_t _io(struct gb_core *gb, uint16_t address)
{
    // TODO: io handling
    // JOYP
    if (address == 0xFF00)
    {
        // TODO: redo this
        // Neither directions nor actions buttons selected, low nibble = 0xF
        if ((gb->membus[address] & 0x30) == 0x30)
            return gb->membus[address] | 0xF;
    }
    return gb->membus[address];
}

static uint8_t _hram(struct gb_core *gb, uint16_t address)
{
    return gb->membus[address];
}

static uint8_t _ie(struct gb_core *gb, uint16_t address)
{
    (void)address;
    return gb->membus[IE];
}

static uint8_t _read_jmp_level_4(struct gb_core *gb, uint16_t address)
{
    static uint8_t (*read_jmp_table_level_4[16])(struct gb_core *, uint16_t address) = {
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
    return read_jmp_table_level_4[address & 0xF](gb, address);
}

static uint8_t _read_jmp_level_3(struct gb_core *gb, uint16_t address)
{
    static uint8_t (*read_jmp_table_level_3[16])(struct gb_core *, uint16_t address) = {
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
        _read_jmp_level_4,
    };
    return read_jmp_table_level_3[(address & 0x00F0) >> 4](gb, address);
}

static uint8_t _read_jmp_level_2(struct gb_core *gb, uint16_t address)
{
    static uint8_t (*read_jmp_table_level_2[16])(struct gb_core *, uint16_t address) = {
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
        _read_jmp_level_3,
    };
    return read_jmp_table_level_2[(address & 0x0F00) >> 8](gb, address);
}

static uint8_t _read_mem(struct gb_core *gb, uint16_t address)
{
    static uint8_t (*read_jmp_table[16])(struct gb_core *, uint16_t address) = {
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
        _read_jmp_level_2,
    };
    return read_jmp_table[(address & 0xF000) >> 12](gb, address);
}

uint8_t read_mem_no_oam_check(struct gb_core *gb, uint16_t address)
{
    return _read_mem(gb, address);
}

uint8_t read_mem(struct gb_core *gb, uint16_t address)
{
    // TODO: fix DMA handling
    // DMA, can only access HRAM
    // if (gb->ppu.dma == 1 && (address < 0xFF80 || address > 0xFFFE))
    //    return 0xFF;
    return _read_mem(gb, address);
}

uint8_t read_mem_tick(struct gb_core *gb, uint16_t address)
{
    uint8_t res = read_mem(gb, address);
    tick_m(gb);
    return res;
}
