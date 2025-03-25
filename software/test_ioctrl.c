#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "KR260_ioctl.h"

int main(int argc, char *argv[]) {
    uint64_t value;
    int choice;
    uint32_t offset;
    uint32_t data;
    
    printf("AES256GCM10G25GIP Register Access Program\n");
    printf("=========================================\n");
    
    while (1) {
        printf("\nChoose an option:\n");
        printf("1. Read register\n");
        printf("2. Write register\n");
        printf("3. Exit\n");
        printf("Your choice: ");
        
        scanf("%d", &choice);
        
        switch (choice) {
            case 1:
                printf("Enter register offset (hex): 0x");
                scanf("%x", &offset);
                
                value = user_read(offset, 8);
                printf("Register 0x%08x = 0x%08lx\n", offset, value);
                break;
                
            case 2:
                printf("Enter register offset (hex): 0x");
                scanf("%x", &offset);
                
                printf("Enter value to write (hex): 0x");
                scanf("%x", &data);
                
                value = user_write(offset, 8, data); 
                printf("After write: Register 0x%08x = 0x%08lx\n", offset, value);
                break;
                
            case 3:
                close_device();  // Close the device file before exiting
                printf("Exiting program\n");
                return 0;
                
            default:
                printf("Invalid choice\n");
                break;
        }
    }
    
    return 0;
}