#ifndef CORE_MBC1_H
#define CORE_MBC1_H

#include <stdbool.h>

#include "mbc_base.h"

struct mbc1
{
    struct mbc_base base;

    uint8_t BANK1;
    uint8_t BANK2;

    uint8_t RAMG; // AKA RAMG

    uint8_t MODE; // Banking Mode

    bool multicart;
};

int make_mbc1(struct mbc_base **output);

#endif
