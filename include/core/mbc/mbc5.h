#ifndef CORE_MBC5_H
#define CORE_MBC5_H

#include "mbc_base.h"

struct mbc5
{
    struct mbc_base base;
    uint16_t bank1;
    uint8_t bank2;

    uint8_t ram_enabled;
};

struct mbc_base *make_mbc5(void);

#endif
