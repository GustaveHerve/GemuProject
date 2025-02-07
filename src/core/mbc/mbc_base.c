#include "mbc_base.h"

#include <err.h>
#include <stdlib.h>

#include "cpu.h"
#include "mbc1.h"
#include "mbc3.h"
#include "mbc5.h"
#include "no_mbc.h"
#include "save.h"

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

static struct mbc_base *make_mbc(uint8_t type_byte)
{
    struct mbc_base *res = NULL;
    switch (type_byte)
    {
    case 0x00:
        res = make_no_mbc();
        break;
    case 0x01:
        res = make_mbc1();
        break;
    case 0x02:
        res = make_mbc1();
        break;
    case 0x03:
        res = make_mbc1();
        break;
    case 0x11:
        res = make_mbc3();
        break;
    case 0x12:
        res = make_mbc3();
        break;
    case 0x13:
        res = make_mbc3();
        break;
    case 0x1A:
        res = make_mbc5();
        break;
    case 0x1B:
        res = make_mbc5();
        break;
    case 0x1C:
        res = make_mbc5();
        break;
    case 0x1D:
        res = make_mbc5();
        break;
    case 0x1E:
        res = make_mbc5();
        break;
    }

    // Unsupported MBC type
    if (!res)
        return NULL;

    res->rom_path = NULL;
    res->save_file = NULL;

    res->rom = NULL;
    res->ram = NULL;

    res->rom_size_header = 0;
    res->ram_size_header = 0;

    res->rom_bank_count = 0;
    res->ram_bank_count = 0;

    res->rom_total_size = 0;
    res->ram_total_size = 0;

    return res;
}

void set_mbc(struct mbc_base **output, uint8_t *rom, char *rom_path)
{
    uint8_t type = rom[0x0147];
    struct mbc_base *mbc = make_mbc(type);

    // Unsupported MBC type
    if (!mbc)
        errx(-3, "ERROR: Provided rom file uses an unsupported MBC type");

    mbc->rom_path = rom_path;

    mbc->rom = rom;

    mbc->rom_size_header = rom[0x0148];
    mbc->ram_size_header = rom[0x0149];

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
    }

    mbc->rom_total_size = mbc->rom_bank_count * 16384;
    mbc->ram_total_size = mbc->ram_bank_count * 8192;

    // Allocate the external RAM
    free(mbc->ram);
    mbc->ram = calloc(8192 * mbc->ram_bank_count, sizeof(uint8_t));

    // Create / Load save file if battery
    if (type == 0x03 || type == 0x06 || type == 0x09 || type == 0x0D || type == 0x0F || type == 0x10 || type == 0x13 ||
        type == 0x1B || type == 0x1E || type == 0x22 || type == 0xFF)
        mbc->save_file = open_save_file(mbc);

    // Free previous MBC if one is already loaded
    if (*output)
        (*output)->_mbc_free(*output);

    *output = mbc;
}

uint8_t read_mbc_rom(struct cpu *cpu, uint16_t address)
{
    return cpu->mbc->_read_mbc_rom(cpu, address);
}

void write_mbc_rom(struct cpu *cpu, uint16_t address, uint8_t val)
{
    cpu->mbc->_write_mbc_rom(cpu, address, val);
}

uint8_t read_mbc_ram(struct cpu *cpu, uint16_t address)
{
    return cpu->mbc->_read_mbc_ram(cpu, address);
}

void write_mbc_ram(struct cpu *cpu, uint16_t address, uint8_t val)
{
    cpu->mbc->_write_mbc_ram(cpu, address, val);
}
