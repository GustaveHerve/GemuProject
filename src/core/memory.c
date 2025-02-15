#include "memory.h"

#include <assert.h>

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

static uint8_t _read_mem_v2(struct gb_core *gb, uint16_t address)
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

static uint8_t _read_mem(struct gb_core *gb, uint16_t address)
{
    // BOOTROM mapping
    if (!(gb->membus[BOOT] & 0x01) && address <= 0x00FF)
        return gb->membus[address];

    // ROM
    else if (address <= 0x7FFF)
        return read_mbc_rom(gb->mbc, address);

    // VRAM
    else if (address >= 0x8000 && address <= 0x9FFF)
    {
        if (gb->ppu.vram_locked)
            return 0xFF;
    }

    // External RAM read
    else if (address >= 0xA000 && address <= 0xBFFF)
        return read_mbc_ram(gb->mbc, address);

    // Echo RAM
    else if (address >= 0xE000 && address <= 0xFDFF)
        return gb->membus[address - 0x2000];
    // OAM
    else if (address >= 0xFE00 && address <= 0xFEFF)
    {
        if (gb->ppu.oam_locked)
            return 0xFF;
    }

    // JOYP
    else if (address == 0xFF00)
    {
        // TODO: redo this
        // Neither directions nor actions buttons selected, low nibble = 0xF
        if ((gb->membus[address] & 0x30) == 0x30)
            return gb->membus[address] | 0xF;
    }

    return gb->membus[address];
}

uint8_t read_mem_no_oam_check(struct gb_core *gb, uint16_t address)
{
    return _read_mem_v2(gb, address);
}

uint8_t read_mem(struct gb_core *gb, uint16_t address)
{
    // TODO: fix DMA handling
    // DMA, can only access HRAM
    // if (gb->ppu.dma == 1 && (address < 0xFF80 || address > 0xFFFE))
    //    return 0xFF;
    return _read_mem_v2(gb, address);
}

uint8_t read_mem_tick(struct gb_core *gb, uint16_t address)
{
    uint8_t res = read_mem(gb, address);
    tick_m(gb);
    return res;
}

// static void test(struct gb_core *gb)
// {
//     (void)gb;
//     return;
// }
//
// void (*write_jmp_table[4])(struct gb_core *) = {test, test, test, test};

void write_mem(struct gb_core *gb, uint16_t address, uint8_t val)
{
    uint8_t write = 1;
    if (address <= 0x7FFF)
    {
        write = 0;
        write_mbc_rom(gb->mbc, address, val);
    }

    // VRAM
    else if (address >= 0x8000 && address <= 0x9FFF)
    {
        if (gb->ppu.vram_locked)
            write = 0;
    }

    // External RAM
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        write = 0;
        write_mbc_ram(gb->mbc, address, val);
    }

    // Echo RAM
    else if (address >= 0xE000 && address <= 0xFDFF)
        address -= 0x2000;

    // OAM
    else if (address >= 0xFE00 && address <= 0xFEFF)
    {
        if (gb->ppu.oam_locked)
            write = 0;
    }

    // JOYP
    else if (address == 0xFF00)
    {
        // TODO: redo this
        write = 0;
        val &= 0x30; // don't write in bit 3-0 and keep only bit 5-4
        uint8_t low_nibble = 0x00;
        if (((val >> 4) & 0x01) == 0x00)
            low_nibble = gb->joyp_d;
        else if (((val >> 5) & 0x01) == 0x00)
            low_nibble = gb->joyp_a;
        else
            low_nibble = 0xF;
        uint8_t new = low_nibble & 0x0F;
        new |= val;
        new |= (gb->membus[address] & 0xC0); // keep the 7-6 bit
        gb->membus[address] = new;
    }

    // SC
    else if (address == 0xFF02)
    {
        gb->membus[SC] = 0x7C | (val & 0x81);
        write = 0;
    }

    // DIV
    else if (address == 0xFF04)
    {
        gb->internal_div = 0;
        gb->membus[DIV] = 0;
        write = 0;
    }

    // TAC
    else if (address == 0xFF07)
    {
        gb->membus[TAC] = 0xF8 | (val & 0x7);
        write = 0;
    }

    // IF
    else if (address == 0xFF0F)
    {
        write = 0;
        uint8_t temp = (gb->membus[address] & 0xE0);
        temp |= (val & 0x1F);
        gb->membus[address] = temp;
    }

    // APU registers
    else if (address == NR14 || address == NR24 || address == NR34 || address == NR44)
    {
        write = 0;
        gb->membus[address] = val & ~(NRx4_UNUSED_PART);
        uint8_t ch_number = ((address - NR14) / (NR24 - NR14)) + 1;
        /* Trigger event */
        if (val & NRx4_TRIGGER_MASK)
        {
            static void (*trigger_handlers[])(struct gb_core *) = {
                &handle_trigger_event_ch1,
                &handle_trigger_event_ch2,
                &handle_trigger_event_ch3,
                &handle_trigger_event_ch4,
            };

            trigger_handlers[ch_number - 1](gb);
        }

        if (val & NRx4_LENGTH_ENABLE)
            enable_timer(gb, ch_number);
    }

    // STAT
    else if (address == 0xFF40)
    {
        // LCD off
        if (!(val >> 6))
            ppu_reset(gb);
    }

    // DMA
    else if (address == 0xFF46)
    {
        write = 0;
        gb->ppu.dma = 2;
        gb->ppu.dma_acc = 0;
        gb->ppu.dma_source = val;
    }

    // BOOT
    else if (address == 0xFF50)
    {
        // Prevent enabling bootrom again
        if (gb->membus[0xFF50] & 0x01)
            write = 0;
    }

    // IE
    else if (address == 0xFFFF)
    {
        write = 0;
        uint8_t temp = (gb->membus[address] & 0xE0);
        temp |= (val & 0x1F);
        gb->membus[address] = temp;
    }

    if (write)
        gb->membus[address] = val;

    tick_m(gb);
}
