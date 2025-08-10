#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "mbc_base.h"

#define SAVEFILE_EXTENSION ".sav"

int open_save_file(struct mbc_base *mbc)
{
    assert(mbc);

    int err_code = EXIT_SUCCESS;
    char *save_path = NULL;

    size_t len = strlen(mbc->rom_path) + (sizeof(SAVEFILE_EXTENSION) - 1) + 1;
    if (!(save_path = malloc(len)))
    {
        err_code = EXIT_FAILURE;
        goto exit;
    }

    if (snprintf(save_path, len, "%s" SAVEFILE_EXTENSION, mbc->rom_path) < 0)
    {
        err_code = EXIT_FAILURE;
        goto exit;
    }

    /* Try to open the save file */
    FILE *res = NULL;
    res = fopen(save_path, "rb+");
    if (!res) /* If no save file exists, create it */
    {
        LOG_WARN("No existing save file found, creating a new one");
        res = fopen(save_path, "wb+");
    }
    else
    {
        LOG_INFO("Found a save file, loading it: %s", save_path);
    }

    if (!res)
    {
        LOG_ERROR("Could not create a save file");
        err_code = EXIT_FAILURE;
        goto exit;
    }

    rewind(res);
    fseek(res, 0, SEEK_END);
    long fsize = ftell(res);
    rewind(res);
    if (fsize <= mbc->ram_total_size)
        fread(mbc->ram, 1, fsize, res);
    // TODO: handle save file too big / invalid
    rewind(res);

    mbc->save_file = res;

exit:
    free(save_path);
    return err_code;
}

int save_ram_to_file(struct mbc_base *mbc)
{
    assert(mbc);

    rewind(mbc->save_file);
    int res = fwrite(mbc->ram, 1, mbc->ram_total_size, mbc->save_file);
    rewind(mbc->save_file);
    return res;
}
