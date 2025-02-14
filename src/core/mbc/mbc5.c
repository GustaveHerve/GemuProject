#include "mbc5.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mbc_base.h"
#include "save.h"
#include "serialization.h"

static void _mbc_reset(struct mbc_base *mbc)
{
    struct mbc5 *mbc5 = (struct mbc5 *)mbc;

    mbc5->bank1 = 1;
    mbc5->bank2 = 0;
    mbc5->ram_enabled = 0;
}

static void _mbc_free(struct mbc_base *mbc)
{
    (void)mbc;
}

static uint8_t _read_mbc_rom(struct mbc_base *mbc, uint16_t address)
{
    struct mbc5 *mbc5 = (struct mbc5 *)mbc;

    unsigned int res_addr = address & 0x3FFF;
    if (address >= 0x4000 && address <= 0x7FFF)
        res_addr = (mbc5->bank1 << 14) | res_addr;

    // Ensure that res_addr doesn't overflow the ROM size
    res_addr &= mbc->rom_total_size - 1;

    return mbc->rom[res_addr];
}

static void _write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc5 *mbc5 = (struct mbc5 *)mbc;

    // RAM Enable
    if (address <= 0x1FFF)
    {
        if (val == 0x0A)
            mbc5->ram_enabled = 1;
        else
            mbc5->ram_enabled = 0;
    }

    // Lower 8 bit of ROM bank
    else if (address >= 0x2000 && address <= 0x2FFF)
    {
        uint8_t mask = mbc->rom_bank_count - 1;
        mbc5->bank1 = val & mask;
    }

    // 9th bit of ROM bank
    else if (address >= 0x3000 && address <= 0x3FFF)
    {
        uint8_t bank = val & 0x01;
        mbc5->bank1 |= bank << 7;
    }

    // RAM bank switch
    else if (address >= 0x4000 && address <= 0x5FFF)
        mbc5->bank2 = val & 0x0F;
}

static uint8_t _read_mbc_ram(struct mbc_base *mbc, uint16_t address)
{
    struct mbc5 *mbc5 = (struct mbc5 *)mbc;

    if (!mbc5->ram_enabled || mbc->ram_bank_count == 0)
        return 0xFF;

    unsigned int res_addr = address & 0x1FFF;
    res_addr = (mbc5->bank2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the RAM size
    res_addr &= mbc->ram_total_size - 1;

    return mbc->ram[res_addr];
}

static void _write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc5 *mbc5 = (struct mbc5 *)mbc;

    // Ignore writes if RAM is disabled or if there is no RAM
    if (!mbc5->ram_enabled || mbc->ram_bank_count == 0)
        return;

    unsigned int res_addr = address & 0x1FFF;
    res_addr = (mbc5->bank2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the ROM size
    res_addr &= mbc->ram_total_size - 1;

    mbc->ram[res_addr] = val;

    // Save if MBC has a save battery
    if (mbc->save_file != NULL)
        save_ram_to_file(mbc);
}

static void _mbc_serialize(struct mbc_base *mbc, FILE *stream)
{
    struct mbc5 *mbc5 = (struct mbc5 *)mbc;

    fwrite_le_16(stream, mbc5->bank1);

    fwrite(&mbc5->bank2, sizeof(uint8_t), 2, stream);
}

static void _mbc_load_from_stream(struct mbc_base *mbc, FILE *stream)
{
    struct mbc5 *mbc5 = (struct mbc5 *)mbc;

    fread_le_16(stream, &mbc5->bank1);

    fread(&mbc5->bank2, sizeof(uint8_t), 2, stream);
}

struct mbc_base *make_mbc5(void)
{
    struct mbc_base *mbc = calloc(1, sizeof(struct mbc5));

    mbc->type = MBC5;

    MBC_SET_VTABLE;

    _mbc_reset(mbc);

    return mbc;
}
