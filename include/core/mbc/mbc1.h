#ifndef CORE_MBC1_H
#define CORE_MBC1_H

#include "mbc_base.h"

struct mbc1
{
    struct mbc_base base;
    uint8_t bank1;
    uint8_t bank2;

    uint8_t ram_enabled; // AKA RAMG

    uint8_t mbc1_mode; // Banking Mode
};

struct mbc_base *make_mbc1(void);

#endif
