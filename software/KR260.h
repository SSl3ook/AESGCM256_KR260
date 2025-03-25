#ifndef KR260_H
#define KR260_H

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <termios.h>

//change input/output func from UART to match terminal
#define gen_printf			printf
#define get_char			getch

// Function to read a single character from keyboard without echoing it
int getch(void);

// Function to read from a memory-mapped address
uint64_t user_read(off_t addr, int wordsize);

// Function to write to a memory-mapped address
uint64_t user_write(off_t addr, int wordsize, uint64_t data);

#endif // KR260_H
