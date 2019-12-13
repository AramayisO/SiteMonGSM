/**
 * @file debug.h
 * @brief This module provides convenience functions and macros for debugging.
 * @author Aramayis Orkusyan
 * @date December 8, 2019
 * @compyright GNU General Public License v3.0
 */
#ifndef SITE_MON_GSM_DEBUG_H
#define SITE_MON_GSM_DEBUG_H

#include <stdio.h>

#ifndef NDEBUG
    #define DEBUG_LOG(stream, str, ...) { \
        fprintf(stream, str, ##__VA_ARGS__); \
    }
#else
    #define DEBUG_LOG(stream, str, ...) { \
    }
#endif // NDEBUG

#endif // SITE_MON_GSM_DEBUG_H
