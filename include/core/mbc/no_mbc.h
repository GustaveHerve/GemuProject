#ifndef CORE_NO_MBC_H
#define CORE_NO_MBC_H

#include "mbc_base.h"

struct no_mbc
{
    struct mbc_base base;
};

struct mbc_base *make_no_mbc(void);

#endif
