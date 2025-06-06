#include "no_mbc.h"

#include <stdlib.h>

#include "mbc_base.h"

static void _mbc_reset(struct mbc_base *mbc)
{
    (void)mbc;
    /* Nothing to do */
}

static void _mbc_free(struct mbc_base *mbc)
{
    (void)mbc;
    /* Nothing to do, all is handled in mbc_base */
    return;
}

static uint8_t _read_mbc_rom(struct mbc_base *mbc, uint16_t address)
{
    return mbc->rom[address];
}

static void _write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    (void)mbc;
    (void)address;
    (void)val;
    /* There is no register to write to without an MBC, don't do anything */
    return;
}

static uint8_t _read_mbc_ram(struct mbc_base *mbc, uint16_t address)
{
    (void)mbc;
    (void)address;
    /* Invalid read as there is no RAM chip mapped, return 0xFF */
    return 0xFF;
}

static void _write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    (void)mbc;
    (void)address;
    (void)val;
    /* Invalid write as there is no RAM chip mapped, don't do anything */
    return;
}

static void _mbc_serialize(struct mbc_base *mbc, FILE *stream)
{
    (void)mbc;
    (void)stream;
    /* Nothing to serialize */
}

static void _mbc_load_from_stream(struct mbc_base *mbc, FILE *stream)
{
    (void)mbc;
    (void)stream;
    /* Nothing to load */
}

struct mbc_base *make_no_mbc(void)
{
    struct mbc_base *mbc = malloc(sizeof(struct no_mbc));

    mbc->type = NO_MBC;

    MBC_SET_VTABLE;

    _mbc_reset(mbc);

    return mbc;
}
