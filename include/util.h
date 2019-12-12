/**
 * @file util.h
 * @brief This module provides commonly used utility functions.
 * @author Aramayis Orkusyan
 * @date December 8, 2019
 * @compyright GNU General Public License v3.0
 */
#ifndef SITE_MON_GSM_UTIL_H
#define SITE_MON_GSM_UTIL_H

#include <unistd.h>

/**
 * Suspend execution for specified number of seconds.
 */
#define SLEEP_SECONDS(n) (usleep(n * 1000000))

/**
 * Suspend execution for specified number of milliseconds.
 */
#define SLEEP_MSECONDS(n) (usleep(n * 1000))

/**
 * Suspend execution for specified number of microseconds.
 */
#define SLEEP_USECONDS(n) (usleep(n))

/**
 * Checks if a string contains any alphanumeric characters.
 *
 * @param str A null-terminated string.
 * @return If str contains an alphanumeric character, returns 1.
 *         Otherwise returns 0.
 */
int has_alphanumeric(const char *str);

#endif
