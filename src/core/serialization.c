#define _DEFAULT_SOURCE
#include <endian.h>
#include <stdint.h>
#include <stdio.h>

unsigned long fwrite_le_16(FILE *stream, uint16_t val)
{
    uint8_t buffer[2] = {val & 0xFF, (val >> 8) & 0xFF};
    return fwrite(buffer, sizeof(uint8_t), 2, stream);
}

unsigned long fread_le_16(FILE *stream, uint16_t *output)
{
    int res = fread(output, sizeof(uint8_t), 2, stream);
    *output = le16toh(*output);
    return res;
}
