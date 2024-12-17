/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright (C) 2024 Gary Sims
 *
 */

#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include <stdio.h>

// Define DEBUG to enable debug prints
// You can define DEBUG via compiler flags (e.g., -DDEBUG) instead of here
#ifndef DEBUG
    #ifdef NDEBUG
        #define DEBUG 0
    #else
        #define DEBUG 1
    #endif
#endif

#if DEBUG

    // Variadic macro for debug printing with contextual information
    #define DEBUG_PRINT(fmt, ...) \
        fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#else

    // When DEBUG is not defined, define DEBUG_PRINT as a no-op
    #define DEBUG_PRINT(fmt, ...) ((void)0)

#endif

#endif // DEBUG_PRINT_H
