#ifndef CORE_MBC5_H
#define CORE_MBC5_H

#include "mbc_base.h"

struct mbc5
{
    struct mbc_base base;
    uint8_t ROMB0;
    uint8_t ROMB1;

    uint8_t RAMB;

    uint8_t RAMG;
};

int make_mbc5(struct mbc_base **output);

#endif
