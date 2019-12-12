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

typedef struct gsm
{
    int  fd;
    char tx_buf[GSM_TX_BUF_SIZE];
    char rx_buf[GSM_RX_BUF_SIZE];
    struct
    {
        char manufacturer[64]; 
        char model[64];        
        char revision[64];     
        char svn[64];           
        char imei[64];         
        char gcap[64];         
    } identification;
} gsm_t;

// Global singleton GSM object
static gsm_t gsm;

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
    ssize_t nbytes;

    // Send AT command to GSM modem.
    sprintf(gsm.tx_buf, "%s\r", AT);

    if (serial_write(gsm.fd, gsm.tx_buf, strlen(gsm.tx_buf)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }

    // Give enough time for GSM modem to have a chance to respond.
    SLEEP_MSECONDS(500);

    // Check if GSM modem sent OK response.
    if ((nbytes = serial_read(gsm.fd, gsm.rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }
    gsm.rx_buf[nbytes] = '\0';
    return (strstr(gsm.rx_buf, AT_OK) ? 1 : 0);
}

/****************************************************************************** 
 *
 * Function:    gsm_read_identification()
 *
 * Description: Reads product identification information from GSM modem and
 *              stores information into gsm.identification structure.
 *
 * Returns:     If successful, returns 0. Otherwise, returns -1.
 *
 ******************************************************************************/
static int gsm_read_identification()
{
    ssize_t nbytes;

    // Send ATI command
    sprintf(gsm.tx_buf, "%s\r", ATI);

    if (serial_write(gsm.fd, gsm.tx_buf, strlen(gsm.tx_buf)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }

    // Give enough time for modem to respond.
    SLEEP_MSECONDS(500);

    // Read response from modem. 
    if ((nbytes = serial_read(gsm.fd, gsm.rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }
    gsm.rx_buf[nbytes] = '\0';

    char *key = strtok(gsm.rx_buf, ":\n");
    char *value;

    while (key != NULL)
    {
        if((key = strtok(NULL, ":")) == NULL)
            break;

        if ((value = strtok(NULL, "\r")) == NULL)
            break;

        // Note that we copy starting at value + 1 because
        // value[0] is white space.
        if (strcmp(key, "Manufacturer") == 0)
            strcpy(gsm.identification.manufacturer, value + 1);
        else if (strcmp(key, "Model"))
            strcpy(gsm.identification.model, value + 1);
        else if (strcmp(key, "Revision"))
            strcpy(gsm.identification.revision, value + 1);
        else if (strcmp(key, "SVN"))
            strcpy(gsm.identification.svn, value + 1);
        else if (strcmp(key, "IMEI"))
            strcpy(gsm.identification.imei, value + 1);
        else if (strcmp(key, "+GCAP"))
            strcpy(gsm.identification.gcap, value + 1);
    }
    return 0;
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
    ssize_t nbytes;

    // Send AT_CMGF command.
    sprintf(gsm.tx_buf, "%s=%u\r", AT_CMGF, fmt);

    if (serial_write(gsm.fd, gsm.tx_buf, strlen(gsm.tx_buf)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }

    // Give modem enough time to process command and write response to serial.
    SLEEP_MSECONDS(500);

    // Check if modem sent OK response.
    if ((nbytes = serial_read(gsm.fd, gsm.rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }
    gsm.rx_buf[nbytes] = '\0';

    return (strstr(gsm.rx_buf, AT_OK) ? 0 : -1);
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
    ssize_t nbytes;

    // Send AT+CSCS command.
    sprintf(gsm.tx_buf, "%s=\"%s\"\r", AT_CSCS, charset);

    if (serial_write(gsm.fd, gsm.tx_buf, strlen(gsm.tx_buf)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }

    // Give modem enough time to process command and write response to serial.
    SLEEP_MSECONDS(500);

    // Check if modem sent OK response.
    if ((nbytes = serial_read(gsm.fd, gsm.rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }
    gsm.rx_buf[nbytes] = '\0';

    return (strstr(gsm.rx_buf, AT_OK) ? 0 : -1);
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
    if((gsm.fd = serial_open(serial_port, B115200)) == -1)
    {
        return -1;
    }

    // Check that modem is connected and responding to AT commands.
    if (!gsm_check_liveness())
    {
        DEBUG_LOG(stdout, "gsm_init: failed liveness check\n");
        serial_close(gsm.fd);
        return -1;
    }
    DEBUG_LOG(stdout, "gsm_init: passed liveness check\n");

    // Read product identification info from SIM controller.
    if (gsm_read_identification() == -1)
    {
        DEBUG_LOG(stdout, "gsm_init: failed to read identification registers\n");
        serial_close(gsm.fd);
        return -1;
    }
    DEBUG_LOG(stdout, "gsm_init: read identification registers\n");

    // Set the message format to text mode.
    if (gsm_set_message_format(GSM_MESSAGE_FORMAT_TEXT_MODE) == -1)
    {
        DEBUG_LOG(stdout, "gsm_init: failed to set message format to text mode\n");
        serial_close(gsm.fd);
        return -1;
    }
    DEBUG_LOG(stdout, "gsm_init: set message format to text mode\n");

    // Set the character set to GSM.
    if (gsm_set_character_set(GSM_CHARSET_GSM) == -1)
    {
        DEBUG_LOG(stdout, "gsm_init: failed to set character set to GSM\n");
        serial_close(gsm.fd);
        return -1;
    }
    DEBUG_LOG(stdout, "gsm_init: set character set to GSM\n");

    return 0;
}

/****************************************************************************** 
 *
 * Function:    gsm_print_identification()
 *
 * Description: Prints product identification information to specified stream.
 *
 * Returns:     None defined.
 *
 ******************************************************************************/
void gsm_print_identification(FILE *stream)
{
    fprintf(stream, "Manufacturer: %s\n", gsm.identification.manufacturer);
    fprintf(stream, "Model:        %s\n", gsm.identification.model);
    fprintf(stream, "Revision:     %s\n", gsm.identification.revision);
    fprintf(stream, "SVN:          %s\n", gsm.identification.svn);
    fprintf(stream, "IMEI:         %s\n", gsm.identification.imei);
    fprintf(stream, "GCAP:         %s\n", gsm.identification.gcap);
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
    ssize_t nbytes;

    // Send AT+CMGS command with destination address.
    sprintf(gsm.tx_buf, "%s=\"%s\"\r", AT_CMGS, destination);

    if (serial_write(gsm.fd, gsm.tx_buf, strlen(gsm.tx_buf)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }

    // Give modem enough time to process command and write response to serial.
    SLEEP_MSECONDS(500);

    // Check if modem responded with the prompt character '>'.
    if ((nbytes = serial_read(gsm.fd, gsm.rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }
    gsm.rx_buf[nbytes] = '\0';

    if (strchr(gsm.rx_buf, '>') == NULL)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }

    // Send the message. Note that the modem waits for a Cntl-Z character to
    // indicate the end of the message to be sent.
    strcpy(gsm.tx_buf, message);
    nbytes = strlen(gsm.tx_buf);
    gsm.tx_buf[nbytes++] = CTRL_Z;

    if (serial_write(gsm.fd, gsm.tx_buf, nbytes) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }

    // Give modem enough time to process command and write response to serial.
    SLEEP_SECONDS(5);

    // Check if message was successfully sent. 
    if ((nbytes = serial_read(gsm.fd, gsm.rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }
    gsm.rx_buf[nbytes] = '\0';

    return (strstr(gsm.rx_buf, "+CMGS") ? 0 : -1);
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

    ssize_t nbytes;

    // Send AT+CFUN command to change mode.
    sprintf(gsm.tx_buf, "%s=%u\r", AT_CFUN, (unsigned) mode);

    if (serial_write(gsm.fd, gsm.tx_buf, strlen(gsm.tx_buf)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }

    // Give modem enough time to proces command and write output to serial.
    SLEEP_SECONDS(1);

    // Check that modem responded with OK.
    if ((nbytes = serial_read(gsm.fd, gsm.rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(gsm.fd);
        return -1;
    }
    gsm.rx_buf[nbytes] = '\0';

    return (strstr(gsm.rx_buf, AT_OK) ? 0 : -1);
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
    ssize_t nbytes;

    // Send AT+CFUN command to change mode.
    sprintf(gsm.tx_buf, "%s?\r", AT_CFUN);

    if (serial_write(gsm.fd, gsm.tx_buf, strlen(gsm.tx_buf)) == -1)
    {
        serial_ioflush(gsm.fd);
        return GSM_FUNCTIONALITY_MODE_ERROR;
    }

    // Give modem enough time to proces command and write output to serial.
    SLEEP_MSECONDS(500);

    // Check that modem responded with OK.
    if ((nbytes = serial_read(gsm.fd, gsm.rx_buf, GSM_RX_BUF_CAPACITY)) == -1)
    {
        serial_ioflush(gsm.fd);
        return GSM_FUNCTIONALITY_MODE_ERROR;
    }
    gsm.rx_buf[nbytes] = '\0';

    // Find the returned value for the mode.
    const char KEY[] = "+CFUN:";
    char *pkey;

    if ((pkey = strstr(gsm.rx_buf, KEY)) == NULL)
    {
        return -1;
    }
    pkey += strlen(KEY);
 
    // Return appropriate value.
    switch (atoi(pkey))
    {
        case GSM_MINIMUM_FUNCTIONALITY_MODE:
            return GSM_MINIMUM_FUNCTIONALITY_MODE;
        case GSM_FULL_FUNCTIONALITY_MODE:
            return GSM_FULL_FUNCTIONALITY_MODE;
        case GSM_FLIGHT_MODE:
            return GSM_FLIGHT_MODE;
        default:
            return GSM_FUNCTIONALITY_MODE_ERROR;
    }

    return GSM_FUNCTIONALITY_MODE_ERROR;
}