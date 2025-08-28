#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <stdint.h>

#define ARR_SIZE(X) (sizeof((X)) / sizeof((X)[0]))

// 8 bit and 16 bit manipulations

static inline uint16_t unpack16(uint8_t hi, uint8_t lo)
{
    return (uint16_t)hi << 8 | lo;
}

static inline void rotl(uint8_t *src)
{
    *src = ((uint8_t)*src << 1) | (*src >> 7);
}

static inline void rotr(uint8_t *src)
{
    *src = (*src >> 1) | ((uint8_t)*src << 7);
}

static inline void rotl16(uint16_t *src)
{
    *src = ((uint16_t)*src << 1) | (*src >> 15);
}

static inline void rotr16(uint16_t *src)
{
    *src = (*src >> 1) | ((uint16_t)*src << 15);
}

#endif
