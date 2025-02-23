#include "read.h"

#include <assert.h>

#include "common.h"
#include "emulation.h"
#include "gb_core.h"
#include "mbc_base.h"

static uint8_t _rom(struct gb_core *gb, uint16_t address)
{
    if (!(gb->memory.io[IO_OFFSET(BOOT)] & 0x01) && gb->memory.boot_rom && address < gb->memory.boot_rom_size)
        return gb->memory.boot_rom[address];
    return read_mbc_rom(gb->mbc, address);
}

static uint8_t _vram(struct gb_core *gb, uint16_t address)
{
    if (gb->ppu.vram_locked)
        return 0xFF;
    return gb->memory.vram[VRAM_OFFSET(address)];
}

static uint8_t _ex_ram(struct gb_core *gb, uint16_t address)
{
    return read_mbc_ram(gb->mbc, address);
}

static uint8_t _wram(struct gb_core *gb, uint16_t address)
{
    return gb->memory.wram[WRAM_OFFSET(address)];
}

static uint8_t _echo_ram(struct gb_core *gb, uint16_t address)
{
    return gb->memory.wram[WRAM_OFFSET(address)];
}

static uint8_t _oam(struct gb_core *gb, uint16_t address)
{
    if (address >= 0xFEA0 && address <= 0xFEFF)
    {
        // TODO: OAM corruption
        return 0x00;
    }
    if (gb->ppu.oam_locked)
        return 0xFF;
    return gb->memory.oam[OAM_OFFSET(address)];
}

static uint8_t _io(struct gb_core *gb, uint16_t address)
{
    switch (address)
    {
    case JOYP:
    {
        uint8_t select_bits = (io_read(gb->memory.io, JOYP) >> 4) & 0x03;
        uint8_t buttons_bits[4] = {
            gb->joyp_a & gb->joyp_d, // TODO: check this case
            gb->joyp_a,
            gb->joyp_d,
            0xF,
        };
        return (gb->memory.io[IO_OFFSET(JOYP)] & 0xF0) | buttons_bits[select_bits];
    }
    case DIV:
        return gb->internal_div >> 8;

    case WAVE_RAM:
    case WAVE_RAM + 1:
    case WAVE_RAM + 2:
    case WAVE_RAM + 3:
    case WAVE_RAM + 4:
    case WAVE_RAM + 5:
    case WAVE_RAM + 6:
    case WAVE_RAM + 7:
    case WAVE_RAM + 8:
    case WAVE_RAM + 9:
    case WAVE_RAM + 10:
    case WAVE_RAM + 11:
    case WAVE_RAM + 12:
    case WAVE_RAM + 13:
    case WAVE_RAM + 14:
    case WAVE_RAM + 15:
        /* Wave RAM accessible to CPU only on same cycle as CH3 read */
        if (is_channel_on(gb, 3) && (gb->apu.ch3.frequency_timer >= 4 || gb->apu.ch3.phantom_sample))
            return 0xFF;
        /* Whole wave RAM is mapped to the same byte read by CH3 */
        uint8_t pos = (gb->apu.ch3.wave_pos) % 32;
        return gb->memory.io[IO_OFFSET(WAVE_RAM + pos / 2)];
    }
    return io_read(gb->memory.io, address);
}

static uint8_t _hram(struct gb_core *gb, uint16_t address)
{
    return gb->memory.hram[HRAM_OFFSET(address)];
}

static uint8_t _ie(struct gb_core *gb, uint16_t address)
{
    (void)address;
    return gb->memory.ie;
}

static uint8_t _read_jmp_level_4(struct gb_core *gb, uint16_t address)
{
    static uint8_t (*read_jmp_table_level_4[16])(struct gb_core *, uint16_t) = {
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
    static uint8_t (*read_jmp_table_level_3[16])(struct gb_core *, uint16_t) = {
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
    static uint8_t (*read_jmp_table_level_2[16])(struct gb_core *, uint16_t) = {
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
    static uint8_t (*read_jmp_table[16])(struct gb_core *, uint16_t) = {
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
    // Only HRAM is accessible during a DMA transfer
    if (gb->ppu.dma == 1 && (address < 0xFF80 || address > 0xFFFE))
        return 0xFF;
    return _read_mem(gb, address);
}

uint8_t read_mem_tick(struct gb_core *gb, uint16_t address)
{
    uint8_t res = read_mem(gb, address);
    tick_m(gb);
    return res;
}
