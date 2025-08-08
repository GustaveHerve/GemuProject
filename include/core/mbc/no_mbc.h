#ifndef CORE_NO_MBC_H
#define CORE_NO_MBC_H

#include "mbc_base.h"

struct no_mbc
{
    struct mbc_base base;
};

int make_no_mbc(struct mbc_base **output);

#endif
