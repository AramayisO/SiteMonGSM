/**
 * @file serial.h
 *
 * @brief A module providing an API to read/write raw bytes from/to a serial port.
 * @author Aramayis Orkusyan
 * @date December 7, 2019 
 * @copyright GNU General Public License v3.0
 */

#ifndef SITE_MON_GSM_SERIAL_H
#define SITE_MON_GSM_SERIAL_H

#include <stdint.h>
#include <termios.h>
#include <sys/types.h>

/**
 * Opens the specified serial port for sending and receiving raw binary bytes.
 *
 * @param device The serial port to open (e.g. /dev/ttyUSB0).
 * @param baud The baud rate at which bits will be transmitted and received. 
 * @return On success, returns a non-negative file descriptor. Otherwise, returns -1.
 */
int serial_open(const char *device, speed_t baud);

/**
 * Write bytes to serial port. 
 *
 * @param fd File descriptor specifying the serial port to write to.
 * @param buffer Pointer to the buffer of bytes to be written.
 * @param nbytes Number of bytes to attempt to write to the serial port. 
 * @return On success, returns the number of bytes written. Otherwise, returns -1. 
 */
ssize_t serial_write(int fd, uint8_t *buffer, size_t nbytes);

/**
 * Read bytes from serial port.
 *
 * @param fd File descriptor specifying the serial port to read from.
 * @param buffer Pointer to the buffer in which to read bytes into.
 * @param nbytes Number of bytes to attempt to read from the serial port. 
 * @return On success, returns the number of bytes actually read. Otherwise, returns -1.
 */
ssize_t serial_read(int fd, uint8_t *buffer, size_t nbytes);

/**
 * Flush all data received but not read and all data written but not yet transmitted.
 *
 * @param fd File descriptor specifying serial port port to flush.
 * @return On success, returns 0. Otherwise, returns -1.
 */
int serial_ioflush(int fd);

/**
 * Close connection to serial port.
 *
 * @param fd File descriptor specifying port connection to close.
 * @return On success, returns 0. Otherwise, returns -1.
 */
int serial_close(int fd);

#endif
