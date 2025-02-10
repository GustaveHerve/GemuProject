#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include <SDL3/SDL_error.h>
#include <stdio.h>

#define SDL_CHECK_ERROR(EXPR)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(EXPR))                                                                                                   \
        {                                                                                                              \
            fputs(SDL_GetError(), stderr);                                                                             \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

#endif
