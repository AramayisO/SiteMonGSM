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

typedef enum gsm_functionality_mode
{
    // The RF part of the module is powered off and the USIM card is not accessible,
    // but serial and USB ports are still accessible. The power consumption in
    // this mode is lower than normal mode and flight mode. 
    GSM_MINIMUM_FUNCTIONALITY_MODE = 0, 
    // Normal mode. All parts of the module powered on.
    GSM_FULL_FUNCTIONALITY_MODE = 1,
    // The RF part of the module is powered off but serial and USB ports are 
    // still accessible. Power consumption in this mode is lower than normal mode.
    GSM_FLIGHT_MODE = 4,  
    // Catch all that can be used to indicate errors.
    GSM_FUNCTIONALITY_MODE_ERROR
} gsm_functionality_mode_t;

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

/**
 * Sets the functionality mode of the modem.
 *
 * @param mode The mode to set the modem to.
 * @return On success, returns 0. Otherwise, returns -1.
 * @note This function can be used to set the modem to low power mode.
 */
int gsm_set_functionality_mode(gsm_functionality_mode_t mode);

/**
 * Returns the functionality mode of the modem.
 *
 * @return On success, returns the mode of the modem. Otherwise, returns
 *         @ref GSM_FUNCTIONALITY_MODE_ERROR.
 */
gsm_functionality_mode_t gsm_get_functionality_mode(void);

#endif

