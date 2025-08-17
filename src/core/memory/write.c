#include "write.h"

#include "emulation.h"
#include "gb_core.h"
#include "interrupts.h"
#include "mbc_base.h"
#include "read.h"
#include "ring_buffer.h"

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
    if (!gb->ppu.oam_locked && gb->ppu.dma != 1)
        gb->memory.oam[OAM_OFFSET(address)] = val;
}

static void _io(struct gb_core *gb, uint16_t address, uint8_t val)
{
    switch (address)
    {
    case JOYP:
    {
        uint8_t prev_joyp = read_mem(gb, JOYP);
        io_write(gb->memory.io, address, val);
        check_joyp_int(gb, prev_joyp);
        return;
    }
    case DIV:
        gb->internal_div = 0;
        return;

    case TIMA:
        /* Ignore TIMA write on cycle after TIMA overflow */
        if (gb->schedule_tima_overflow)
            return;
        break;

    case NR10:
    case NR11:
    case NR12:
    case NR13:
    case NR14:
    case NR21:
    case NR22:
    case NR23:
    case NR24:
    case NR30:
    case NR31:
    case NR32:
    case NR33:
    case NR34:
    case NR41:
    case NR42:
    case NR43:
    case NR44:
    case NR50:
    case NR51:
    case NR52:
        apu_write_reg(gb, address, val);
        return;

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
        /* Attempting write to wave RAM while channel 3 is active */
        if (is_channel_on(gb, 3))
        {
            /* CPU can only access the Wave RAM when CH3 is also accessing it */
            if ((gb->apu.ch3.frequency_timer >= 4 || gb->apu.ch3.phantom_sample))
                return;
            /* CPU can only access the same byte that CH3 is acessing (CH3 has priority over CPU) */
            uint8_t pos = (gb->apu.ch3.wave_pos) % 32;
            address = WAVE_RAM + pos / 2;
        }
        break;

    case LCDC:
        /* LCD off */
        if (!(val >> 7))
            ppu_reset(gb);
        break;

    case DMA:
    {
        struct dma_request new_req = {
            .source = val > 0xDF ? val & 0xDF : val,
            .status = DMA_REQUESTED,
        };
        RING_BUFFER_ENQUEUE(dma_request, &gb->ppu.dma_requests, &new_req);
        break;
    }

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
    gb->memory.ie = val;
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
