#ifndef CORE_SERIALIZATION
#define CORE_SERIALIZATION

#include <stdint.h>
#include <stdio.h>

struct gb_core;

unsigned long fwrite_le_16(FILE *stream, uint16_t val);

unsigned long fread_le_16(FILE *stream, uint16_t *output);

#endif
