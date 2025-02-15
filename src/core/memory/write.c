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
    if (!gb->ppu.oam_locked)
        gb->memory.oam[OAM_OFFSET(address)] = val;
}

static void _io(struct gb_core *gb, uint16_t address, uint8_t val)
{
    if (address == JOYP)
    {
        // TODO: redo this
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
        new |= (gb->memory.io[IO_OFFSET(JOYP)] & 0xC0); // keep the 7-6 bit
        gb->memory.io[IO_OFFSET(JOYP)] = new;
        return;
    }

    if (address == SC)
    {
        gb->memory.io[IO_OFFSET(SC)] = 0x7C | (val & 0x81);
        return;
    }

    if (address == DIV)
    {
        gb->internal_div = 0;
        gb->memory.io[IO_OFFSET(DIV)] = 0;
        return;
    }

    if (address == TAC)
    {
        gb->memory.io[IO_OFFSET(TAC)] = 0xF8 | (val & 0x7);
        return;
    }

    if (address == IF)
    {
        uint8_t temp = (gb->memory.io[IO_OFFSET(IF)] & 0xE0);
        temp |= (val & 0x1F);
        gb->memory.io[IO_OFFSET(IF)] = temp;
        return;
    }

    if (address == NR14 || address == NR24 || address == NR34 || address == NR44)
    {
        gb->memory.io[IO_OFFSET(address)] = val & ~(NRx4_UNUSED_PART);
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
        return;
    }

    if (address == LCDC)
    {
        // LCD off
        if (!(val >> 7))
            ppu_reset(gb);
        gb->memory.io[IO_OFFSET(LCDC)] = val;
        return;
    }

    if (address == DMA)
    {
        gb->ppu.dma = 2;
        gb->ppu.dma_acc = 0;
        gb->ppu.dma_source = val;
        return;
    }

    if (address == BOOT)
    {
        // Prevent enabling bootrom again
        if (!(gb->memory.io[IO_OFFSET(BOOT)] & 0x01))
            gb->memory.io[IO_OFFSET(BOOT)] = val;
        return;
    }

    gb->memory.io[IO_OFFSET(address)] = val;
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
