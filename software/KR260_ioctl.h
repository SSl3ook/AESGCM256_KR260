#ifndef KR260_IOCTL_H
#define KR260_IOCTL_H

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

//change input/output func from UART to match terminal
#define gen_printf			printf
#define get_char			getch

/* Define ioctl commands - must match the driver definitions */
#define AES_IOC_MAGIC 'a'
#define AES_IOC_READ_REG   _IOR(AES_IOC_MAGIC, 1, struct aes_reg_data)
#define AES_IOC_WRITE_REG  _IOW(AES_IOC_MAGIC, 2, struct aes_reg_data)

/* Structure for register access */
struct aes_reg_data {
    uint32_t offset;    /* Register offset */
    uint64_t value;     /* Value to read or write */
    uint8_t width;      /* Access width in bits: 8, 16, 32, 64 */
};

// Function to read a single character from keyboard without echoing it
int getch(void);

// Function to read from a hardware register using ioctl
uint64_t user_read(off_t addr, int wordsize);

// Function to write to a hardware register using ioctl
uint64_t user_write(off_t addr, int wordsize, uint64_t data);

// Function to close the device when done
void close_device(void);

#endif // KR260_IOCTL_H