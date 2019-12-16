#include "gsm.h"
#include "serial.h"
#include "util.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Control characters
#define CTRL_Z   0x1A

// Supported AT commands
#define AT       "AT"
#define ATI      "ATI"
#define AT_CMGF  "AT+CMGF"
#define AT_CMGS  "AT+CMGS"
#define AT_CSCS  "AT+CSCS"
#define AT_CFUN  "AT+CFUN"
#define AT_OK    "OK"
#define AT_ERROR "ERROR"

// Supported message formats
#define GSM_MESSAGE_FORMAT_PDU_MODE  0
#define GSM_MESSAGE_FORMAT_TEXT_MODE 1

// Supported character sets 
#define GSM_CHARSET_IRA  "IRA"
#define GSM_CHARSET_GSM  "GSM"
#define GSM_CHARSET_UCS2 "UCS2"

// The buffer capacity should be on less than the size to allow for null
// terminating character.
#define GSM_RX_BUF_CAPACITY (GSM_RX_BUF_SIZE - 1)

static int  fd;
static char tx_buf[GSM_TX_BUF_SIZE];
static char rx_buf[GSM_RX_BUF_SIZE];

/****************************************************************************** 
 *
 * Function     gsm_check_liveness()
 *
 * Description: Checks if GSM modem is connected and reachable via serial.
 *
 * Notes:       This function simply tests to see if the GSM modem responds 
 *              with "OK" to the "AT" command.
 *
 * Returns:     If GSM modem is connected and reachable, returns 1. Otherwise,
 *              returns 0.
 ******************************************************************************/
static int gsm_check_liveness()
{
    // Send AT command to GSM modem.
    sprintf(tx_buf, "%s\r", AT);

    if (serial_write(fd, tx_buf, strlen(tx_buf)) == -1)
    {
        serial_ioflush(fd);
        return -1;
    }

    // Give enough time for GSM modem to have a chance to respond.
    SLEEP_MSECONDS(500);

    // Check if GSM modem sent OK response.
    ssize_t nbytes;
    if ((nbytes = serial_read(fd, rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(fd);
        return -1;
    }
    rx_buf[nbytes] = '\0';
    return (strstr(rx_buf, AT_OK) ? 1 : 0);
}

/****************************************************************************** 
 *
 * Function:    gsm_set_message_format()
 *
 * Description: Sets the input and output format of short messages.
 *
 * Returns:     If successful, returns 0. Otherwise, returns -1.
 *
 ******************************************************************************/
int gsm_set_message_format(unsigned int fmt)
{

    // Send AT_CMGF command.
    sprintf(tx_buf, "%s=%u\r", AT_CMGF, fmt);

    if (serial_write(fd, tx_buf, strlen(tx_buf)) == -1)
    {
        serial_ioflush(fd);
        return -1;
    }

    // Give modem enough time to process command and write response to serial.
    SLEEP_MSECONDS(500);

    // Check if modem sent OK response.
    ssize_t nbytes;
    if ((nbytes = serial_read(fd, rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(fd);
        return -1;
    }
    rx_buf[nbytes] = '\0';
    return (strstr(rx_buf, AT_OK) ? 0 : -1);
}

/****************************************************************************** 
 *
 * Function:    gsm_set_character_set()
 *
 * Description: Set the character set to bye used by the TE. 
 *
 * Returns:     If successful, returns 0. Otherwise, returns -1.
 *
 ******************************************************************************/
static int gsm_set_character_set(const char *charset)
{

    // Send AT+CSCS command.
    sprintf(tx_buf, "%s=\"%s\"\r", AT_CSCS, charset);

    if (serial_write(fd, tx_buf, strlen(tx_buf)) == -1)
    {
        serial_ioflush(fd);
        return -1;
    }

    // Give modem enough time to process command and write response to serial.
    SLEEP_MSECONDS(500);

    // Check if modem sent OK response.
    ssize_t nbytes;
    if ((nbytes = serial_read(fd, rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(fd);
        return -1;
    }
    rx_buf[nbytes] = '\0';
    return (strstr(rx_buf, AT_OK) ? 0 : -1);
}

/****************************************************************************** 
 *
 * Function:    gsm_init()
 *
 * Description: 
 *
 * Returns:     If successful, returns 0. Otherwise, returns -1.
 *
 ******************************************************************************/
int gsm_init(const char *serial_port)
{
    // Attempt to open serial connection to GSM modem.
    if((fd = serial_open(serial_port, B115200)) == -1)
    {
        DEBUG_LOG(stdout, "%s: Filed to open serial port\n", __FILE__);
        return -1;
    }

    // Check that modem is connected and responding to AT commands.
    if (!gsm_check_liveness())
    {
        DEBUG_LOG(stdout, "gsm_init: failed liveness check\n");
        serial_close(fd);
        return -1;
    }
    DEBUG_LOG(stdout, "gsm_init: passed liveness check\n");

    // Set the message format to text mode.
    if (gsm_set_message_format(GSM_MESSAGE_FORMAT_TEXT_MODE) == -1)
    {
        DEBUG_LOG(stdout, "gsm_init: failed to set message format to text mode\n");
        serial_close(fd);
        return -1;
    }
    DEBUG_LOG(stdout, "gsm_init: set message format to text mode\n");

    // Set the character set to GSM.
    if (gsm_set_character_set(GSM_CHARSET_GSM) == -1)
    {
        DEBUG_LOG(stdout, "gsm_init: failed to set character set to GSM\n");
        serial_close(fd);
        return -1;
    }
    DEBUG_LOG(stdout, "gsm_init: set character set to GSM\n");
    serial_ioflush(fd);
    return 0;
}

/****************************************************************************** 
 *
 * Function:    gsm_send_message()
 *
 * Description: Send a message to a destination address over the GSM network.  
 *
 * Notes:       destination and message must be a null-terminated strings.
 *
 * Returns:     If successful, returns 0. Otherwise, returns -1.
 *
 ******************************************************************************/
int gsm_send_message(const char *destination, const char *message)
{
    // Send AT+CMGS command with destination address.
    sprintf(tx_buf, "%s=\"%s\"\r", AT_CMGS, destination);

    if (serial_write(fd, tx_buf, strlen(tx_buf)) == -1)
    {
        DEBUG_LOG(stdout, "%s: failed to write AT+CMGS command\n", __FILE__);
        serial_ioflush(fd);
        return -1;
    }

    // Give modem enough time to process command and write response to serial.
    SLEEP_MSECONDS(500);

    // Check if modem responded with the prompt character '>'.
    ssize_t nbytes;
    if ((nbytes = serial_read(fd, rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        DEBUG_LOG(stdout, "%s: failed to read after AT+CMGS command\n", __FILE__);
        serial_ioflush(fd);
        return -1;
    }
    rx_buf[nbytes] = '\0';
    printf("bytes read: %ld\n", nbytes);
    printf("rs_buf: %s\n", rx_buf);

    SLEEP_MSECONDS(500);

    if (strchr(rx_buf, '>') == NULL)
    {
        DEBUG_LOG(stdout, "%s: Didn't recieve '>' prompt\n", __FILE__);
        serial_ioflush(fd);
        return -1;
    }

    // Send the message. Note that the modem waits for a Cntl-Z character to
    // indicate the end of the message to be sent.
    strcpy(tx_buf, message);
    nbytes = strlen(tx_buf);
    tx_buf[nbytes++] = CTRL_Z;

    if (serial_write(fd, tx_buf, nbytes) == -1)
    {
        DEBUG_LOG(stdout, "%s: failed to send message '%s'\n", __FILE__, message);
        serial_ioflush(fd);
        return -1;
    }

    // Give modem enough time to process command and write response to serial.
    SLEEP_SECONDS(5);

    // Check if message was successfully sent. 
    if ((nbytes = serial_read(fd, rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        DEBUG_LOG(stdout, "%s: failed to read after message sent\n", __FILE__);
        serial_ioflush(fd);
        return -1;
    }
    rx_buf[nbytes] = '\0';
    return (strstr(rx_buf, "+CMGS") ? 0 : -1);
}

/****************************************************************************** 
 *
 * Function:    gsm_set_functionality_mode()
 *
 * Description: Sets the functionality mode of the modem 
 *
 * Returns:     If successful, returns 0. Otherwise, returns -1.
 *
 ******************************************************************************/
int gsm_set_functionality_mode(gsm_functionality_mode_t mode)
{
    if (mode == GSM_FUNCTIONALITY_MODE_ERROR)
    {
        return -1;
    }

    // Send AT+CFUN command to change mode.
    sprintf(tx_buf, "%s=%u\r", AT_CFUN, (unsigned) mode);

    if (serial_write(fd, tx_buf, strlen(tx_buf)) == -1)
    {
        serial_ioflush(fd);
        return -1;
    }

    SLEEP_SECONDS(2);

    ssize_t nbytes;
    if ((nbytes = serial_read(fd, rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(fd);
        return -1;
    }
    rx_buf[nbytes] = '\0';
    return (strstr(rx_buf, AT_OK) ? 0 : -1);
}

/****************************************************************************** 
 *
 * Function:    gsm_get_functionality_mode()
 *
 * Description: Returns the functionality mode of the modem. 
 *
 * Returns:     On success, returns the mode of the modem. Otherwise, returns
 *              GSM_FUNCTIONALITY_MODE_ERROR.
 *
 ******************************************************************************/
gsm_functionality_mode_t gsm_get_functionality_mode()
{

    // Send AT+CFUN command to change mode.
    sprintf(tx_buf, "%s?\r", AT_CFUN);

    if (serial_write(fd, tx_buf, strlen(tx_buf)) == -1)
    {
        serial_ioflush(fd);
        return GSM_FUNCTIONALITY_MODE_ERROR;
    }

    // Give modem enough time to proces command and write output to serial.
    SLEEP_MSECONDS(500);

    // Check that modem responded with OK.
    ssize_t nbytes;
    if ((nbytes = serial_read(fd, rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(fd);
        return GSM_FUNCTIONALITY_MODE_ERROR;
    }
    rx_buf[nbytes] = '\0';

    // Find the returned value for the mode.
    const char KEY[] = "+CFUN:";
    char *pkey;

    if ((pkey = strstr(rx_buf, KEY)) == NULL)
    {
        return GSM_FUNCTIONALITY_MODE_ERROR;
    }
    pkey += strlen(KEY);
 
    // Return appropriate value.
    gsm_functionality_mode_t mode;
    switch (atoi(pkey))
    {
        case GSM_MINIMUM_FUNCTIONALITY_MODE:
            mode = GSM_MINIMUM_FUNCTIONALITY_MODE;
        case GSM_FULL_FUNCTIONALITY_MODE:
            mode = GSM_FULL_FUNCTIONALITY_MODE;
        case GSM_FLIGHT_MODE:
            mode = GSM_FLIGHT_MODE;
        default:
            mode = GSM_FUNCTIONALITY_MODE_ERROR;
    }


    return mode;
}
