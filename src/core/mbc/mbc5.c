#include "mbc5.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cpu.h"
#include "save.h"

static void _mbc_free(struct mbc_base *mbc)
{
    (void)mbc;
}

static uint8_t _read_mbc_rom(struct cpu *cpu, uint16_t address)
{
    struct mbc5 *mbc = (struct mbc5 *)cpu->mbc;

    unsigned int res_addr = address & 0x3FFF;
    if (address >= 0x4000 && address <= 0x7FFF)
        res_addr = (mbc->bank1 << 14) | res_addr;

    // Ensure that res_addr doesn't overflow the ROM size
    res_addr &= cpu->mbc->rom_total_size - 1;

    return cpu->mbc->rom[res_addr];
}

static void _write_mbc_rom(struct cpu *cpu, uint16_t address, uint8_t val)
{
    struct mbc5 *mbc = (struct mbc5 *)cpu->mbc;

    // RAM Enable
    if (address <= 0x1FFF)
    {
        if (val == 0x0A)
            mbc->ram_enabled = 1;
        else
            mbc->ram_enabled = 0;
    }

    // Lower 8 bit of ROM bank
    else if (address >= 0x2000 && address <= 0x2FFF)
    {
        uint8_t mask = cpu->mbc->rom_bank_count - 1;
        mbc->bank1 = val & mask;
    }

    // 9th bit of ROM bank
    else if (address >= 0x3000 && address <= 0x3FFF)
    {
        uint8_t bank = val & 0x01;
        mbc->bank1 |= bank << 7;
    }

    // RAM bank switch
    else if (address >= 0x4000 && address <= 0x5FFF)
        mbc->bank2 = val & 0x0F;
}

static uint8_t _read_mbc_ram(struct cpu *cpu, uint16_t address)
{
    struct mbc5 *mbc = (struct mbc5 *)cpu->mbc;

    if (!mbc->ram_enabled || cpu->mbc->ram_bank_count == 0)
        return 0xFF;

    unsigned int res_addr = address & 0x1FFF;
    res_addr = (mbc->bank2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the RAM size
    res_addr &= cpu->mbc->ram_total_size - 1;

    return cpu->mbc->ram[res_addr];
}

static void _write_mbc_ram(struct cpu *cpu, uint16_t address, uint8_t val)
{
    struct mbc5 *mbc = (struct mbc5 *)cpu->mbc;

    // Ignore writes if RAM is disabled or if there is no RAM
    if (!mbc->ram_enabled || cpu->mbc->ram_bank_count == 0)
        return;

    unsigned int res_addr = address & 0x1FFF;
    res_addr = (mbc->bank2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the ROM size
    res_addr &= cpu->mbc->ram_total_size - 1;

    cpu->mbc->ram[res_addr] = val;

    // Save if MBC has a save battery
    if (cpu->mbc->save_file != NULL)
        save_ram_to_file(cpu->mbc);
}

struct mbc_base *make_mbc5(void)
{
    struct mbc_base *mbc = calloc(1, sizeof(struct mbc5));
    struct mbc5 *mbc5 = (struct mbc5 *)mbc;

    mbc->type = MBC5;

    mbc->_mbc_free = &_mbc_free;

    mbc->_read_mbc_rom = &_read_mbc_rom;
    mbc->_write_mbc_rom = &_write_mbc_rom;

    mbc->_read_mbc_ram = &_read_mbc_ram;
    mbc->_write_mbc_ram = &_write_mbc_ram;

    mbc5->bank1 = 1;
    mbc5->bank2 = 0;
    mbc5->ram_enabled = 0;

    return mbc;
}
