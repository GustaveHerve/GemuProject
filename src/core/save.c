#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbc/mbc_base.h"

FILE *open_save_file(struct mbc_base *mbc)
{
    size_t len = strlen(mbc->rom_path) + 5;
    char *save_path = malloc(len);
    snprintf(save_path, len, "%s.sav", mbc->rom_path);

    FILE *res = NULL;
    res = fopen(save_path, "rb+");
    if (!res)
        res = fopen(save_path, "wb+");
    free(save_path);

    rewind(res);
    fseek(res, 0, SEEK_END);
    long fsize = ftell(res);
    rewind(res);
    if (fsize <= mbc->ram_total_size)
        fread(mbc->ram, 1, fsize, res);
    rewind(res);
    return res;
}

int save_ram_to_file(struct mbc_base *mbc)
{
    rewind(mbc->save_file);
    int res = fwrite(mbc->ram, 1, mbc->ram_total_size, mbc->save_file);
    rewind(mbc->save_file);
    return res;
}
