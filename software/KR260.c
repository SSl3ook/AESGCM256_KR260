#include "KR260.h"

//reads from keypress
int getch(void) 
{ 
    struct termios oldattr, newattr; 
    int ch; 
    // Get the current terminal settings and save them in oldattr
    tcgetattr(STDIN_FILENO, &oldattr); 
    // Copy the current settings to newattr to modify them
    newattr = oldattr; 
    // Modify the settings to disable canonical mode and echoing
    newattr.c_lflag &= ~(ICANON | ECHO); 
    // Apply the modified settings immediately
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr); 
    // Read a single character from standard input
    ch = getchar(); 
    // Restore the original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); 
    return ch; 
} 

uint64_t user_read(off_t addr, int wordsize) {
    int mem_fd;
    void *base, *offset;
    long pagesize;
    uint64_t data = 0;

    // Get the page size
    pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize <= 0) {
        perror("sysconf");
        return 0;
    }

    // Open /dev/mem
    mem_fd = open("/dev/mem", O_RDONLY);
    if (mem_fd < 0) {
        perror("open /dev/mem failed");
        return 0;
    }

    // Memory map the specified address
    base = mmap(NULL, pagesize, PROT_READ, MAP_SHARED, mem_fd, addr & ~(pagesize - 1));
    if (base == MAP_FAILED) {
        perror("mmap failed");
        close(mem_fd); // Close the file descriptor before returning
        return 0;
    }

    // Calculate the offset within the page
    offset = base + (addr & (pagesize - 1));

    // Read the data based on the word size
    switch (wordsize) {
        case 64:
            data = *(volatile uint64_t *)offset;
            break;
        case 32:
            data = *(volatile uint32_t *)offset;
            break;
        case 16:
            data = *(volatile uint16_t *)offset;
            break;
        case 8:
            data = *(volatile uint8_t *)offset;
            break;
        default:
            fprintf(stderr, "Invalid wordsize specified\n");
            munmap(base, pagesize); // Unmap the memory before returning
            close(mem_fd); // Close the file descriptor before returning
            return 0;
    }

    // Unmap the memory
    if (munmap(base, pagesize)) {
        perror("munmap failed");
    }

    // Close the file descriptor
    if (close(mem_fd)) {
        perror("cannot close /dev/mem");
    }

    return data;
}


uint64_t user_write(off_t addr, int wordsize, uint64_t data) {
    int mem_fd;
    void *base, *offset;
    long pagesize;
    uint64_t result = 0;

    // Get the page size
    pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize <= 0) {
        perror("sysconf");
        return 0;
    }

    // Open /dev/mem for reading and writing
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("open /dev/mem failed");
        return 0;
    }

    // Memory map the specified address with read and write permissions
    base = mmap(NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr & ~(pagesize - 1));
    if (base == MAP_FAILED) {
        perror("mmap failed");
        close(mem_fd); // Close the file descriptor before returning
        return 0;
    }

    // Calculate the offset within the page
    offset = base + (addr & (pagesize - 1));

    // Write the data based on the word size
    switch (wordsize) {
        case 64:
            *(volatile uint64_t *)offset = data;
            result = *(volatile uint64_t *)offset;
            break;
        case 32:
            *(volatile uint32_t *)offset = (uint32_t)data;
            result = *(volatile uint32_t *)offset;
            break;
        case 16:
            *(volatile uint16_t *)offset = (uint16_t)data;
            result = *(volatile uint16_t *)offset;
            break;
        case 8:
            *(volatile uint8_t *)offset = (uint8_t)data;
            result = *(volatile uint8_t *)offset;
            break;
        default:
            fprintf(stderr, "Invalid wordsize specified\n");
            munmap(base, pagesize); // Unmap the memory before returning
            close(mem_fd); // Close the file descriptor before returning
            return 0;
    }

    // Unmap the memory
    if (munmap(base, pagesize)) {
        perror("munmap failed");
    }

    // Close the file descriptor
    if (close(mem_fd)) {
        perror("cannot close /dev/mem");
    }

    return result;
}
