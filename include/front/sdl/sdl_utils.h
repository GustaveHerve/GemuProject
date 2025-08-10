#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include <SDL3/SDL_error.h>
#include <stdio.h>
#include <stdlib.h>

#define SDL_CHECK_ERROR(EXPR)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(EXPR))                                                                                                   \
        {                                                                                                              \
            perror(SDL_GetError());                                                                                    \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

#define SDL_CHECK_ERROR_GOTO(EXPR, LABEL)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(EXPR))                                                                                                   \
        {                                                                                                              \
            perror(SDL_GetError());                                                                                    \
            goto LABEL;                                                                                                \
        }                                                                                                              \
    } while (0)

#endif
