#include "mbc_base.h"

#include <assert.h>
#include <err.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include "mbc1.h"
#include "mbc2.h"
#include "mbc3.h"
#include "mbc5.h"
#include "no_mbc.h"
#include "save.h"

void mbc_reset(struct mbc_base *mbc)
{
    if (!mbc)
        return;
    mbc->_mbc_reset(mbc);
}

void mbc_free(struct mbc_base *mbc)
{
    if (!mbc)
        return;

    mbc->_mbc_free(mbc);

    if (mbc->save_file)
        fclose(mbc->save_file);
    free(mbc->rom);
    free(mbc->ram);
    free(mbc);
}

static int make_mbc(uint8_t type_byte, struct mbc_base **output)
{
    int err = EXIT_SUCCESS;
    switch (type_byte)
    {
    case 0x00:
        err = make_no_mbc(output);
        break;
    case 0x01:
    case 0x02:
    case 0x03:
        err = make_mbc1(output);
        break;
    case 0x05:
    case 0x06:
        err = make_mbc2(output);
        break;
    case 0x11:
    case 0x12:
    case 0x13:
        err = make_mbc3(output);
        break;
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
        err = make_mbc5(output);
        break;
    default:
        /* Unsupported MBC type */
        fprintf(stderr, "ERROR: ROM is of unsuported MBC type\n");
        return EXIT_FAILURE;
    }

    if (err == EXIT_FAILURE)
    {
        fprintf(stderr, "ERROR: MBC allocation failed\n");
        return EXIT_FAILURE;
    }

    (*output)->rom_path = NULL;
    (*output)->save_file = NULL;

    (*output)->rom = NULL;
    (*output)->ram = NULL;

    (*output)->rom_size_header = 0;
    (*output)->ram_size_header = 0;

    (*output)->rom_bank_count = 0;
    (*output)->ram_bank_count = 0;

    (*output)->rom_total_size = 0;
    (*output)->ram_total_size = 0;

    return EXIT_SUCCESS;
}

int set_mbc(struct mbc_base **output, uint8_t *rom, char *rom_path, size_t file_size)
{
    assert(output);
    assert(rom);
    assert(rom_path);

    uint8_t type = rom[0x0147];
    struct mbc_base *mbc = NULL;
    if (make_mbc(type, &mbc) == EXIT_FAILURE)
        goto error_exit;

    mbc->rom_path = rom_path;
    mbc->rom_basename = basename(rom_path);
    mbc->rom = rom;
    mbc->rom_size_header = rom[0x0148];
    mbc->ram_size_header = rom[0x0149];

    if (mbc->rom_size_header > 0x08) /* Unknown/Unofficial ROM size */
        // TODO: what to do in case of weird ROM size ?
        // mbc->rom_bank_count = (file_size + (1 << 14) - 1) / (1 << 14);
        mbc->rom_bank_count = 512;
    else
        mbc->rom_bank_count = 1 << (mbc->rom_size_header + 1);

    switch (mbc->ram_size_header)
    {
    case 0x00:
        mbc->ram_bank_count = 0;
        break;
    case 0x02:
        mbc->ram_bank_count = 1;
        break;
    case 0x03:
        mbc->ram_bank_count = 4;
        break;
    case 0x04:
        mbc->ram_bank_count = 16;
        break;
    case 0x05:
        mbc->ram_bank_count = 8;
        break;
    default:
        /* Unknown RAM size, assume there is max possible RAM just to be safe */
        mbc->ram_bank_count = 16;
        break;
    }

    mbc->rom_total_size = mbc->rom_bank_count * 16384;
    mbc->ram_total_size = mbc->ram_bank_count * 8192;

    // Allocate the external RAM
    free(mbc->ram);
    if (mbc->type == MBC2)
        mbc->ram = calloc(512, sizeof(uint8_t));
    else
        mbc->ram = calloc(8192 * mbc->ram_bank_count, sizeof(uint8_t));

    if (!mbc->ram)
        goto error_exit;

    // Create / Load save file if battery
    if (type == 0x03 || type == 0x06 || type == 0x09 || type == 0x0D || type == 0x0F || type == 0x10 || type == 0x13 ||
        type == 0x1B || type == 0x1E || type == 0x22 || type == 0xFF)
    {
        if (open_save_file(mbc))
            goto error_exit;
    }

    // Free previous MBC if one is already loaded
    if (*output)
        (*output)->_mbc_free(*output);

    *output = mbc;
    return EXIT_SUCCESS;

error_exit:
    free(mbc);
    return EXIT_FAILURE;
}

uint8_t read_mbc_rom(struct mbc_base *mbc, uint16_t address)
{
    return mbc->_read_mbc_rom(mbc, address);
}

void write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    mbc->_write_mbc_rom(mbc, address, val);
}

uint8_t read_mbc_ram(struct mbc_base *mbc, uint16_t address)
{
    return mbc->_read_mbc_ram(mbc, address);
}

void write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    mbc->_write_mbc_ram(mbc, address, val);
}

void mbc_serialize(struct mbc_base *mbc, FILE *stream)
{
    fwrite(mbc->ram, sizeof(uint8_t), mbc->ram_total_size, stream);
    mbc->_mbc_serialize(mbc, stream);
}

void mbc_load_from_stream(struct mbc_base *mbc, FILE *stream)
{
    fread(mbc->ram, sizeof(uint8_t), mbc->ram_total_size, stream);
    mbc->_mbc_load_from_stream(mbc, stream);
}
