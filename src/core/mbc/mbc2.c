#include "mbc2.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mbc_base.h"
#include "save.h"

static void _mbc_reset(struct mbc_base *mbc)
{
    struct mbc2 *mbc2 = (struct mbc2 *)mbc;

    mbc2->RAMG = 0;
    mbc2->ROMB = 1;
}

static void _mbc_free(struct mbc_base *mbc)
{
    (void)mbc;
}

static uint8_t _read_mbc_rom(struct mbc_base *mbc, uint16_t address)
{
    struct mbc2 *mbc2 = (struct mbc2 *)mbc;

    unsigned int res_addr = address & 0x3FFF;

    if (address >= 0x4000 && address <= 0x7FFF)
        res_addr = (mbc2->ROMB << 14) | res_addr;

    // Ensure that res_addr doesn't overflow the ROM size
    res_addr &= mbc->rom_total_size - 1;

    return mbc->rom[res_addr];
}

static void _write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc2 *mbc2 = (struct mbc2 *)mbc;

    if (address <= 0x3FFF)
    {
        uint16_t a8_bit = address & 0x100;
        if (!a8_bit)
        {
            /* RAMG */
            if ((val & 0xF) == 0xA)
                mbc2->RAMG = 1;
            else
                mbc2->RAMG = 0;
        }
        else
        {
            /* ROMB */
            uint8_t bank = val & 0xF;
            // Prevent bank 0x00 duplication (only if uses 5 bits)
            if (bank == 0)
                mbc2->ROMB = 1;
            else
            {
                uint8_t mask = mbc->rom_bank_count - 1;
                mbc2->ROMB = bank & mask;
            }
        }
    }
}

static uint8_t _read_mbc_ram(struct mbc_base *mbc, uint16_t address)
{
    struct mbc2 *mbc2 = (struct mbc2 *)mbc;

    if (!mbc2->RAMG)
        return 0xFF;

    unsigned int res_addr = address & 0x1FF;

    return mbc->ram[res_addr] | 0xF0;
}

static void _write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc2 *mbc2 = (struct mbc2 *)mbc;

    // Ignore writes if RAM is disabled
    if (!mbc2->RAMG)
        return;

    unsigned int res_addr = address & 0x1FF;

    mbc->ram[res_addr] = val & 0x0F;

    // Save if MBC has a save battery
    if (mbc->save_file != NULL)
        save_ram_to_file(mbc);
}

static void _mbc_serialize(struct mbc_base *mbc, FILE *stream)
{
    struct mbc2 *mbc2 = (struct mbc2 *)mbc;

    fwrite(&mbc2->RAMG, sizeof(uint8_t), 1, stream);
    fwrite(&mbc2->ROMB, sizeof(uint8_t), 1, stream);
}

static void _mbc_load_from_stream(struct mbc_base *mbc, FILE *stream)
{
    struct mbc2 *mbc2 = (struct mbc2 *)mbc;

    fread(&mbc2->RAMG, sizeof(uint8_t), 1, stream);
    fread(&mbc2->ROMB, sizeof(uint8_t), 1, stream);
}

int make_mbc2(struct mbc_base **output)
{
    if (!(*output = calloc(1, sizeof(struct mbc2))))
        return EXIT_FAILURE;

    (*output)->type = MBC2;
    MBC_SET_VTABLE(*output);
    _mbc_reset(*output);

    return EXIT_SUCCESS;
}
