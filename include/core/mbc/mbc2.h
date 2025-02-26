#ifndef CORE_MBC2_H
#define CORE_MBC2_H

#include "mbc_base.h"

struct mbc2
{
    struct mbc_base base;

    uint8_t RAMG;
    uint8_t ROMB;
};

struct mbc_base *make_mbc2(void);

#endif
