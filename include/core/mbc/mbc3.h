#ifndef CORE_MBC3_H
#define CORE_MBC3_H

#include "mbc_base.h"

struct rtc
{
    uint8_t s;  // Seconds
    uint8_t m;  // Minutes
    uint8_t h;  // Hours
    uint8_t dl; // Lower byte of Day Counter
    uint8_t dh; // Upper bit of Day Counter, Carry and Halt flag
};

struct mbc3
{
    struct mbc_base base;
    uint8_t bank1;
    uint8_t bank2;

    uint8_t ram_rtc_registers_enabled;
    struct rtc rtc_clock;

    uint16_t latch_last_write;
};

struct mbc_base *make_mbc3(void);

#endif
