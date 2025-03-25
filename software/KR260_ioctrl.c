#include "KR260_ioctl.h"

#define AES_BASE_ADDR 0xA0000000

// Global file descriptor for the AES device
static int aes_fd = -1;

// Opens the AES device if not already open
static int ensure_device_open(void) {
    if (aes_fd < 0) {
        aes_fd = open("/dev/aes256gcm", O_RDWR);
        if (aes_fd < 0) {
            perror("Failed to open AES device");
            return -1;
        }
    }
    return 0;
}

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
    int fd;
    struct aes_reg_data reg;
    uint64_t result = 0;
    
    // Open device
    fd = open("/dev/aes256gcm", O_RDWR);
    if (fd < 0) {
        perror("Failed to open AES device");
        return 0;
    }
    
    // Convert physical address to offset
    if (addr >= AES_BASE_ADDR && addr < (AES_BASE_ADDR + 0x10000)) {
        reg.offset = addr - AES_BASE_ADDR;
    } else {
        // Address is out of range
        fprintf(stderr, "Error: Address 0x%lx is outside AES device range (0x%x-0x%x)\n",
                (unsigned long)addr, AES_BASE_ADDR, AES_BASE_ADDR + 0xFFFF);
        close(fd);
        return 0;  // Return 0 instead of attempting invalid access
    }
    
    // Set access width
    switch (wordsize) {
        case 64:
            reg.width = 64;
            break;
        case 32:
            reg.width = 32;
            break;
        case 16:
            reg.width = 16;
            break;
        case 8:
            reg.width = 8;
            break;
        default:
            fprintf(stderr, "Invalid wordsize specified\n");
            close(fd);
            return 0;
    }
    
    // Read the register value using ioctl
    if (ioctl(fd, AES_IOC_READ_REG, &reg) < 0) {
        perror("ioctl read failed");
        close(fd);
        return 0;
    }
    
    close(fd);  // Close file descriptor to prevent leaks
    
    // Return the value as is - the driver will have returned the correct value size
    result = reg.value;
    
    return result;
}

uint64_t user_write(off_t addr, int wordsize, uint64_t data) {
    int fd;
    struct aes_reg_data reg;
    
    // Open device
    fd = open("/dev/aes256gcm", O_RDWR);
    if (fd < 0) {
        perror("Failed to open AES device");
        return 0;
    }
    
    // Convert physical address to offset
    if (addr >= AES_BASE_ADDR && addr < (AES_BASE_ADDR + 0x10000)) {
        reg.offset = addr - AES_BASE_ADDR;
    } else {
        // Address is out of range
        fprintf(stderr, "Error: Address 0x%lx is outside AES device range (0x%x-0x%x)\n",
                (unsigned long)addr, AES_BASE_ADDR, AES_BASE_ADDR + 0xFFFF);
        close(fd);
        return 0;  // Return 0 instead of attempting invalid access
    }
    
    // Set value and access width
    reg.value = data;
    
    switch (wordsize) {
        case 64:
            reg.width = 64;
            break;
        case 32:
            reg.width = 32;
            break;
        case 16:
            reg.width = 16;
            break;
        case 8:
            reg.width = 8;
            break;
        default:
            fprintf(stderr, "Invalid wordsize specified\n");
            close(fd);
            return 0;
    }
    
    // Write the register value using ioctl
    if (ioctl(fd, AES_IOC_WRITE_REG, &reg) < 0) {
        perror("ioctl write failed");
        close(fd);
        return 0;
    }
    
    close(fd);  // Close file descriptor to prevent leaks
    
    // Return the data that was written
    return data;
}
    
// Function to close the device when done
void close_device(void) {
    if (aes_fd >= 0) {
        close(aes_fd);
        aes_fd = -1;
    }
}