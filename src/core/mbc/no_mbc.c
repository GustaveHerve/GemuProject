#include "no_mbc.h"

#include <stdlib.h>

#include "cpu.h"
#include "mbc_base.h"

static void _mbc_free(struct mbc_base *mbc)
{
    (void)mbc;
    /* Nothing to do, all is handled in mbc_base */
    return;
}

static uint8_t _read_mbc_rom(struct cpu *cpu, uint16_t address)
{
    struct no_mbc *mbc = (struct no_mbc *)cpu->mbc;
    return mbc->base.rom[address];
}

static void _write_mbc_rom(struct cpu *cpu, uint16_t address, uint8_t val)
{
    (void)cpu;
    (void)address;
    (void)val;
    /* There is no register to write to without an MBC, don't do anything */
    return;
}

static uint8_t _read_mbc_ram(struct cpu *cpu, uint16_t address)
{
    (void)cpu;
    (void)address;
    /* Invalid read as there is no RAM chip mapped, return 0xFF */
    return 0xFF;
}

static void _write_mbc_ram(struct cpu *cpu, uint16_t address, uint8_t val)
{
    (void)cpu;
    (void)address;
    (void)val;
    /* Invalid write as there is no RAM chip mapped, don't do anything */
    return;
}

struct mbc_base *make_no_mbc(void)
{
    struct mbc_base *mbc = malloc(sizeof(struct no_mbc));

    mbc->type = NO_MBC;

    mbc->_mbc_free = &_mbc_free;

    mbc->_read_mbc_rom = &_read_mbc_rom;
    mbc->_write_mbc_rom = &_write_mbc_rom;

    mbc->_read_mbc_ram = &_read_mbc_ram;
    mbc->_write_mbc_ram = &_write_mbc_ram;

    return mbc;
}
