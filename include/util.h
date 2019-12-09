/**
 * @file util.h
 * @brief This module provides commonly used utility functions.
 * @author Aramayis Orkusyan
 * @date December 8, 2019
 * @compyright GNU General Public License v3.0
 */
#ifndef SITE_MON_GSM_UTIL_H
#define SITE_MON_GSM_UTIL_H

/**
 * Checks if a string contains any alphanumeric characters.
 *
 * @param str A null-terminated string.
 * @return If str contains an alphanumeric character, returns 1.
 *         Otherwise returns 0.
 */
int has_alphanumeric(const char *str);

#endif
