/**
 * @file gsm.h
 *
 * @brief This module provides a high-level API for accessing and using the GSM modem.
 * @author Aramayis Orkusyan
 * @date December 8, 2019
 * @copyright GNU General Public License v3.0
 */

#ifndef SITE_MON_GSM_GSM_H
#define SITE_MON_GSM_GSM_H

#define GSM_TX_BUF_SIZE 256
#define GSM_RX_BUF_SIZE 256

#include <stdio.h>

/**
 * Initialize the GSM modem to allow for SMS messaging.
 *
 * @param device The serial port to which the GSM modem is connected. 
 * @return On success, returns 0. Otherwise, returns -1.
 */
int gsm_init(const char *device);

/**
 * Print the connected GSM modems product identification information.
 *
 * @param stream The output stream to print to.
 */
void gsm_print_identification(FILE *stream);

/**
 * Send a message to a destination address over the GSM network.
 *
 * @param destination The address to send the message to (e.g. a phone number)
 * @param message The null-terminated string to be sent to destination.
 * @return On success, returns 0. Otherwise, returns -1.
 */
int gsm_send_message(const char *destination, const char *message);

#endif

