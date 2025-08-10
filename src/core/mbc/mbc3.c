#include "mbc3.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "save.h"
#include "serialization.h"

// clang-format off
#define RTC_SECONDS         0x08
#define RTC_MINUTES         0x09
#define RTC_HOURS           0x0A
#define RTC_DAY_LOWER       0x0B
#define RTC_DAY_UPPER       0x0C

#define RTC_DAY_UPPER_MASK  0xC1
// clang-format on

static void _mbc_reset(struct mbc_base *mbc)
{
    struct mbc3 *mbc3 = (struct mbc3 *)mbc;

    mbc3->bank1 = 1;
    mbc3->bank2 = 0;
    mbc3->ram_rtc_registers_enabled = 0;

    // Set to 0xFFFF (purposefully impossible value) and not 0 to handle the first write to it
    mbc3->latch_last_write = -1;

    mbc3->rtc_clock.s = 0;
    mbc3->rtc_clock.m = 0;
    mbc3->rtc_clock.h = 0;
    mbc3->rtc_clock.dl = 0;
    mbc3->rtc_clock.dh = 0;
}

static void _mbc_free(struct mbc_base *mbc)
{
    (void)mbc;
}

static uint8_t _read_mbc_rom(struct mbc_base *mbc, uint16_t address)
{
    struct mbc3 *mbc3 = (struct mbc3 *)mbc;

    unsigned int res_addr = address & 0x3FFF;
    if (address >= 0x4000 && address <= 0x7FFF)
        res_addr = (mbc3->bank1 << 14) | res_addr;

    // Ensure that res_addr doesn't overflow the ROM size
    res_addr &= mbc->rom_total_size - 1;

    return mbc->rom[res_addr];
}

static void _write_mbc_rom(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc3 *mbc3 = (struct mbc3 *)mbc;

    // RAM Enable
    if (address <= 0x1FFF)
    {
        if ((val & 0x0F) == 0x0A)
            mbc3->ram_rtc_registers_enabled = 1;
        else
            mbc3->ram_rtc_registers_enabled = 0;
    }

    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        uint8_t bank = val & 0x7F;
        // Prevent bank 0x00 duplication
        if (bank == 0x00)
            mbc3->bank1 = 0x01;
        else
        {
            uint8_t mask = mbc->rom_bank_count - 1;
            mbc3->bank1 = bank & mask;
        }
    }

    // RAM bank switch OR RTC register select
    else if (address >= 0x4000 && address <= 0x5FFF)
    {
        if (val < RTC_SECONDS || val > RTC_DAY_UPPER)
            mbc3->bank2 = val & 0x03;
    }

    // Latch Clock Data
    else if (address >= 0x6000 && address <= 0x7FFF)
    {
        if (mbc3->latch_last_write == 0x00 && val == 0x01)
        {
            // Latch RTC registers
        }
        mbc3->latch_last_write = val;
    }
}

static uint8_t read_rtc_register(struct mbc3 *mbc)
{
    switch (mbc->bank2)
    {
    case RTC_SECONDS:
        return mbc->rtc_clock.s;
    case RTC_MINUTES:
        return mbc->rtc_clock.m;
    case RTC_HOURS:
        return mbc->rtc_clock.h;
    case RTC_DAY_LOWER:
        return mbc->rtc_clock.dl;
    case RTC_DAY_UPPER:
        return mbc->rtc_clock.dh;
    }
    return 0;
}

static uint8_t _read_mbc_ram(struct mbc_base *mbc, uint16_t address)
{
    struct mbc3 *mbc3 = (struct mbc3 *)mbc;

    if (!mbc3->ram_rtc_registers_enabled)
        return 0xFF;

    // RTC register mapping
    read_rtc_register(mbc3);

    if (mbc->ram_bank_count == 0)
        return 0xFF;

    unsigned int res_addr = address & 0x1FFF;
    // RAM bank mapping
    res_addr = (mbc3->bank2 << 13) | res_addr;

    // Ensure that res_addr doesn't overflow the RAM size
    res_addr &= mbc->ram_total_size - 1;

    return mbc->ram[res_addr];
}

static int write_rtc_register(struct mbc3 *mbc, uint8_t val)
{
    switch (mbc->bank2)
    {
    case RTC_SECONDS:
    {
        if (val < 60)
            mbc->rtc_clock.s = val;
        return 1;
    }
    case RTC_MINUTES:
    {
        if (val < 60)
            mbc->rtc_clock.m = val;
        return 1;
    }
    case RTC_HOURS:
    {
        if (val < 24)
            mbc->rtc_clock.h = val;
        return 1;
    }
    case RTC_DAY_LOWER:
    {
        mbc->rtc_clock.dl = val;
        return 1;
    }
    case RTC_DAY_UPPER:
    {
        val &= RTC_DAY_UPPER_MASK;
        mbc->rtc_clock.dh = val;
        return 1;
    }
    }
    return 0;
}

static void _write_mbc_ram(struct mbc_base *mbc, uint16_t address, uint8_t val)
{
    struct mbc3 *mbc3 = (struct mbc3 *)mbc;

    // Ignore writes if RAM / RTC registers are disabled
    if (!mbc3->ram_rtc_registers_enabled)
        return;

    // RTC register bank case
    if (write_rtc_register(mbc3, val))
        return;

    unsigned int res_addr = address & 0x1FFF;
    res_addr = (mbc3->bank2 << 13) | res_addr;

    if (mbc->ram_bank_count == 0)
        return;

    // Ensure that res_addr doesn't overflow the RAM size
    res_addr &= mbc->ram_total_size - 1;

    mbc->ram[res_addr] = val;

    // Save if MBC has a save battery
    if (mbc->save_file != NULL)
        save_ram_to_file(mbc);
}

static void _mbc_serialize(struct mbc_base *mbc, FILE *stream)
{
    // TODO
    struct mbc3 *mbc3 = (struct mbc3 *)mbc;

    fwrite(&mbc3->bank1, sizeof(uint8_t), 4, stream);

    fwrite_le_16(stream, mbc3->latch_last_write);
}

static void _mbc_load_from_stream(struct mbc_base *mbc, FILE *stream)
{
    // TODO
    struct mbc3 *mbc3 = (struct mbc3 *)mbc;

    fread(&mbc3->bank1, sizeof(uint8_t), 8, stream);

    fread_le_16(stream, &mbc3->latch_last_write);
}

int make_mbc3(struct mbc_base **output)
{
    if (!(*output = calloc(1, sizeof(struct mbc3))))
        return EXIT_FAILURE;

    (*output)->type = MBC3;
    MBC_SET_VTABLE(*output);
    _mbc_reset(*output);

    return EXIT_SUCCESS;
}
