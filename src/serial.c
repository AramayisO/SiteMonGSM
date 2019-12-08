#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include "serial.h"

int serial_open(const char *device, speed_t baud)
{
    int fd;

    // Attempt to open the serial port for read and write and without switching
    // controlling terminal.
    if ((fd = open(device, O_RDWR | O_NOCTTY)) == -1)
    {
        return -1;
    }

    // Flush all data received but not read and data written but not transmitted.
    if (tcflush(fd, TCIOFLUSH))
    {
        close(fd);
        return -1;
    }

    // Get current configuration.
    struct termios term_config;

    if (tcgetattr(fd, &term_config) == -1)
    {
        close(fd);
        return -1; 
    }

    // Set terminal to "raw" mode: input available character by character,
    // echoing disabled, all special processing of terminal input and 
    // output characters is disabled.
    term_config.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    term_config.c_oflag &= ~(ONLCR | OCRNL);
    term_config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Set timeouts: read() will return as soon as at least one byte is
    // available or when 100ms has passed.
    term_config.c_cc[VTIME] = 1;
    term_config.c_cc[VMIN]  = 0;

    // Set output baud rate.
    cfsetospeed(&term_config, baud);

    // Set input baud rate same as output baud rate.
    cfsetispeed(&term_config, cfgetospeed(&term_config));
    
    if (tcsetattr(fd, TCSANOW, &term_config) == -1)
    {
        close(fd);
        return -1;
    }

    return fd;
}

ssize_t serial_write(int fd, uint8_t *buffer, size_t nbytes)
{
    if ((fd < 0) || (buffer == NULL))
    {
        return -1;
    }

    return write(fd, buffer, nbytes);
}

ssize_t serial_read(int fd, uint8_t *buffer, size_t nbytes)
{
    if ((fd == 0) || (buffer == NULL))
    {
        return -1;
    }

    ssize_t nread  = 0;
    ssize_t result = 0;

    while (nbytes)
    {
        result = read(fd, buffer, nbytes);

        if (result == -1)
        {
            return -1;
        }
        else if (result == 0)
        {
            return nread;
        }

        nread  += result;
        nbytes -= result;
    }

    return nread;
}

int serial_close(int fd)
{
    return close(fd);
}

