#include "mbc1.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "save.h"

static void _mbc_reset(struct mbc_base *mbc)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    mbc1->bank1 = 1;
    mbc1->bank2 = 0;
    mbc1->ram_enabled = 0;
    mbc1->mbc1_mode = 1;
}

static void _mbc_free(struct mbc_base *mbc)
{
    (void)mbc;
}

static uint8_t _read_mbc_rom(struct mbc_base *mbc, uint16_t address)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    unsigned int res_addr = address & 0x3FFF;
    if (address <= 0x3FFF)
    {
        if (mbc1->mbc1_mode)
            res_addr = (mbc1->bank2 << 19) | res_addr;
    }

    else if (address >= 0x4000 && address <= 0x7FFF)
        res_addr = (mbc1->bank2 << 19) | (mbc1->bank1 << 14) | res_addr;

    // Ensure that res_addr doesn't overflow the ROM size
    res_addr &= mbc->rom_total_size - 1;

    return mbc->rom[res_addr];
}

static void _write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    // RAM Enable
    if (address <= 0x1FFF)
    {
        if ((val & 0x0F) == 0x0A)
            mbc1->ram_enabled = 1;
        else
            mbc1->ram_enabled = 0;
    }

    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        uint8_t bank = val & 0x1F;
        // Prevent bank 0x00 duplication (only if uses 5 bits)
        if (bank == 0x00)
            mbc1->bank1 = 0x01;
        else
        {
            uint8_t mask = mbc->rom_bank_count - 1;
            mbc1->bank1 = bank & mask;
        }
    }

    // RAM bank switch OR Upper bits of ROM bank switch
    else if (address >= 0x4000 && address <= 0x5FFF)
        mbc1->bank2 = val & 0x03;

    // Banking mode select
    else if (address >= 0x6000 && address <= 0x7FFF)
        mbc1->mbc1_mode = val & 0x01;
}

static uint8_t _read_mbc_ram(struct mbc_base *mbc, uint16_t address)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    if (!mbc1->ram_enabled || mbc->ram_bank_count == 0)
        return 0xFF;

    unsigned int res_addr = address & 0x1FFF;
    if (mbc1->mbc1_mode)
        res_addr = (mbc1->bank2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the RAM size
    res_addr &= mbc->ram_total_size - 1;

    return mbc->ram[res_addr];
}

static void _write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    // Ignore writes if RAM is disabled or if there is no external RAM
    if (!mbc1->ram_enabled || mbc->ram_bank_count == 0)
        return;

    unsigned int res_addr = address & 0x1FFF;
    if (mbc1->mbc1_mode)
        res_addr = (mbc1->bank2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the RAM size
    res_addr &= mbc->ram_total_size - 1;

    mbc->ram[res_addr] = val;

    // Save if MBC has a save battery
    if (mbc->save_file != NULL)
        save_ram_to_file(mbc);
}

struct mbc_base *make_mbc1(void)
{
    struct mbc_base *mbc = calloc(1, sizeof(struct mbc1));

    mbc->type = MBC1;

    mbc->_mbc_reset = _mbc_reset;
    mbc->_mbc_free = _mbc_free;

    mbc->_read_mbc_rom = _read_mbc_rom;
    mbc->_write_mbc_rom = _write_mbc_rom;

    mbc->_read_mbc_ram = _read_mbc_ram;
    mbc->_write_mbc_ram = _write_mbc_ram;

    _mbc_reset(mbc);

    return mbc;
}
