#include <stdint.h>
#include <stdio.h>

unsigned long fwrite_le_16(FILE *stream, uint16_t val)
{
    uint8_t buffer[2] = {val & 0xFF, (val >> 8) & 0xFF};
    return fwrite(buffer, sizeof(uint8_t), 2, stream);
}

unsigned long fread_le_16(FILE *stream, uint16_t *output)
{
    uint8_t lo;
    uint8_t hi;
    int res = fread(&lo, sizeof(uint8_t), 1, stream);
    res += fread(&hi, sizeof(uint8_t), 1, stream);
    *output = hi << 8 | lo;
    return res;
}
