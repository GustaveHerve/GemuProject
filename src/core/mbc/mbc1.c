#include "mbc1.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "save.h"

static void _mbc_reset(struct mbc_base *mbc)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    mbc1->BANK1 = 1;
    mbc1->BANK2 = 0;
    mbc1->RAMG = 0;
    mbc1->MODE = 0;
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
        if (mbc1->MODE)
            res_addr = (mbc1->BANK2 << (19 - mbc1->multicart)) | res_addr;
    }

    else if (address >= 0x4000 && address <= 0x7FFF)
    {
        uint8_t bank1 = mbc1->BANK1 & ~(mbc1->multicart << 4);
        res_addr = (mbc1->BANK2 << (19 - mbc1->multicart)) | (bank1 << 14) | res_addr;
    }

    // Ensure that res_addr doesn't overflow the ROM size
    res_addr &= mbc->rom_total_size - 1;

    return mbc->rom[res_addr];
}

static void _write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    // RAM Enable
    if (address <= 0x1FFF)
        mbc1->RAMG = (val & 0xF) == 0x0A;

    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        uint8_t bank = val & 0x1F;
        // Prevent bank 0x00 duplication (only if uses 5 bits)
        if (bank == 0x00)
            mbc1->BANK1 = 0x01;
        else
        {
            uint8_t mask = mbc->rom_bank_count - 1;
            mbc1->BANK1 = bank & mask;
        }
    }

    // RAM bank switch OR Upper bits of ROM bank switch
    else if (address >= 0x4000 && address <= 0x5FFF)
        mbc1->BANK2 = val & 0x03;

    // Banking mode select
    else if (address >= 0x6000 && address <= 0x7FFF)
        mbc1->MODE = val & 0x01;
}

static uint8_t _read_mbc_ram(struct mbc_base *mbc, uint16_t address)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    if (!mbc1->RAMG || mbc->ram_bank_count == 0)
        return 0xFF;

    unsigned int res_addr = address & 0x1FFF;
    if (mbc1->MODE)
        res_addr = (mbc1->BANK2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the RAM size
    res_addr &= mbc->ram_total_size - 1;

    return mbc->ram[res_addr];
}

static void _write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    // Ignore writes if RAM is disabled or if there is no external RAM
    if (!mbc1->RAMG || mbc->ram_bank_count == 0)
        return;

    unsigned int res_addr = address & 0x1FFF;
    if (mbc1->MODE)
        res_addr = (mbc1->BANK2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the RAM size
    res_addr &= mbc->ram_total_size - 1;

    mbc->ram[res_addr] = val;

    // Save if MBC has a save battery
    if (mbc->save_file != NULL)
        save_ram_to_file(mbc);
}

static void _mbc_serialize(struct mbc_base *mbc, FILE *stream)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    fwrite(&mbc1->BANK1, sizeof(uint8_t), 1, stream);
    fwrite(&mbc1->BANK2, sizeof(uint8_t), 1, stream);
    fwrite(&mbc1->RAMG, sizeof(uint8_t), 1, stream);
    fwrite(&mbc1->MODE, sizeof(uint8_t), 1, stream);
}

static void _mbc_load_from_stream(struct mbc_base *mbc, FILE *stream)
{
    struct mbc1 *mbc1 = (struct mbc1 *)mbc;

    fread(&mbc1->BANK1, sizeof(uint8_t), 1, stream);
    fread(&mbc1->BANK2, sizeof(uint8_t), 1, stream);
    fread(&mbc1->RAMG, sizeof(uint8_t), 1, stream);
    fread(&mbc1->MODE, sizeof(uint8_t), 1, stream);
}

int make_mbc1(struct mbc_base **output)
{
    if (!(*output = calloc(1, sizeof(struct mbc1))))
        return EXIT_FAILURE;

    (*output)->type = MBC1;
    MBC_SET_VTABLE(*output);
    _mbc_reset(*output);

    return EXIT_SUCCESS;
}
