#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include <SDL3/SDL_error.h>
#include <stdlib.h>

#include "logger.h"

#define SDL_CHECK_ERROR(EXPR)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(EXPR))                                                                                                   \
        {                                                                                                              \
            LOG_ERROR("SDL Error: %s", SDL_GetError());                                                                \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

#define SDL_CHECK_ERROR_GOTO(EXPR, LABEL)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(EXPR))                                                                                                   \
        {                                                                                                              \
            LOG_ERROR("SDL Error: %s", SDL_GetError());                                                                \
            goto LABEL;                                                                                                \
        }                                                                                                              \
    } while (0)

#endif
