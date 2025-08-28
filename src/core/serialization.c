#include <stdint.h>
#include <stdio.h>

unsigned long fwrite_le_16(FILE *stream, uint16_t val)
{
    uint8_t buffer[2] = {
        val & 0xFF,
        (val >> 8) & 0xFF,
    };
    return fwrite(buffer, sizeof(uint8_t), 2, stream);
}

unsigned long fwrite_le_32(FILE *stream, uint32_t val)
{
    uint8_t buffer[4] = {
        val & 0xFF,
        (val >> 8) & 0xFF,
        (val >> 16) & 0xFF,
        (val >> 24) & 0xFF,
    };
    return fwrite(buffer, sizeof(uint8_t), 4, stream);
}

unsigned long fwrite_le_64(FILE *stream, uint64_t val)
{
    uint8_t buffer[8] = {
        val & 0xFF,
        (val >> 8) & 0xFF,
        (val >> 16) & 0xFF,
        (val >> 24) & 0xFF,
        (val >> 32) & 0xFF,
        (val >> 40) & 0xFF,
        (val >> 48) & 0xFF,
        (val >> 56) & 0xFF,
    };
    return fwrite(buffer, sizeof(uint8_t), 8, stream);
}

unsigned long fread_le_16(FILE *stream, uint16_t *output)
{
    uint8_t buffer[2];
    unsigned long res = fread(&buffer, sizeof(uint8_t), 2, stream);
    *output = (uint16_t)buffer[1] << 8 | (uint16_t)buffer[0];
    return res;
}

unsigned long fread_le_32(FILE *stream, uint32_t *output)
{
    uint8_t buffer[4];
    unsigned long res = fread(&buffer, sizeof(uint8_t), 4, stream);
    *output = (uint32_t)buffer[3] << 24 | (uint32_t)buffer[2] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[0];
    return res;
}

unsigned long fread_le_64(FILE *stream, uint64_t *output)
{
    uint8_t buffer[8];
    unsigned long res = fread(&buffer, sizeof(uint8_t), 8, stream);
    *output = (uint64_t)buffer[7] << 56 | (uint64_t)buffer[6] << 48 | (uint64_t)buffer[5] << 40 |
              (uint64_t)buffer[4] << 32 | (uint64_t)buffer[3] << 24 | (uint64_t)buffer[2] << 16 |
              (uint64_t)buffer[1] << 8 | (uint64_t)buffer[0];
    return res;
}
