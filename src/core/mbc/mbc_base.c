#include "mbc_base.h"

#include <assert.h>
#include <err.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
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

    if (err)
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

static bool is_multicart(struct mbc_base *mbc)
{
    if (mbc->rom_bank_count != 64) /* All known MBC1M are 1 MiB */
        return false;
    LOG_DEBUG("Potential MBC1 Multicart, checking for duplicate logo header data...");

    /* Check for duplicate header logo data */
    static const uint8_t logo[] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83,
                                   0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
                                   0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
                                   0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};
    unsigned int match_counter = 0;
    for (uint8_t bank2 = 0; bank2 < 4; ++bank2)
    {
        unsigned int start_addr = (bank2 << 18) | 0x104;
        if (mbc->rom_total_size < start_addr + sizeof(logo))
            break;
        match_counter += (!memcmp(logo, mbc->rom + start_addr, sizeof(logo)));
    }

    return match_counter >= 3;
}

int set_mbc(struct mbc_base **output, uint8_t *rom, char *rom_path)
{
    assert(output);
    assert(rom);
    assert(rom_path);

    uint8_t type = rom[0x0147];
    struct mbc_base *mbc = NULL;
    if (make_mbc(type, &mbc))
        goto error_exit;

    mbc->rom_path = rom_path;
    mbc->rom_basename = basename(rom_path);
    mbc->rom = rom;
    mbc->rom_size_header = rom[0x0148];
    mbc->ram_size_header = rom[0x0149];

    if (mbc->rom_size_header > 0x08) /* Unknown/Unofficial ROM size */
        mbc->rom_bank_count =
            512; /* Allocate max possible value just in case, it's MBC's reponsability to handle wrapping by masking */
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

    if (mbc->type == MBC1 && is_multicart(mbc))
    {
        LOG_DEBUG("Multicart detected");
        ((struct mbc1 *)mbc)->multicart = true;
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
