#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#define BASE_ADDR   0xA0000000
#define START_OFFSET 0x2000
#define END_OFFSET   0xF000
#define MAP_SIZE    (END_OFFSET - START_OFFSET)

#define NUM_OPERATIONS 1000

// Global variables to store timing results for summary
double wordsize_8_read_avg = 0.0;
double wordsize_16_read_avg = 0.0;
double wordsize_32_read_avg = 0.0;
double wordsize_64_read_avg = 0.0;
double wordsize_8_write_avg = 0.0;
double wordsize_16_write_avg = 0.0;
double wordsize_32_write_avg = 0.0;
double wordsize_64_write_avg = 0.0;

// Function to calculate time difference in microseconds
uint64_t time_diff_us(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
}

// Generate a random aligned offset within the specified range
uint32_t generate_aligned_offset(int wordsize) {
    uint32_t range = MAP_SIZE;
    uint32_t alignment;
    uint32_t offset;
    
    // Determine alignment based on wordsize
    switch (wordsize) {
        case 8:
            alignment = 1;  // 1-byte alignment
            break;
        case 16:
            alignment = 2;  // 2-byte alignment
            break;
        case 32:
            alignment = 4;  // 4-byte alignment
            break;
        case 64:
            alignment = 8;  // 8-byte alignment
            break;
        default:
            alignment = 4;  // Default to 4-byte alignment
    }
    
    // Generate random offset and align it
    offset = START_OFFSET + (rand() % range);
    offset = offset & ~(alignment - 1);  // Mask out lower bits to ensure alignment
    
    // Make sure offset doesn't exceed end_offset
    if (offset > END_OFFSET)
        offset = END_OFFSET & ~(alignment - 1);
        
    return offset;
}

// Benchmark read operations
void benchmark_read(void *mapped_base, int wordsize) {
    struct timespec start, end;
    uint64_t elapsed_us, total_us = 0;
    uint64_t value = 0;
    uint32_t offset;
    double avg_time;
    
    printf("Benchmarking READ operations with wordsize %d...\n", wordsize);
    
    // Pre-generate all offsets to remove that overhead from the timing
    uint32_t offsets[NUM_OPERATIONS];
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        offsets[i] = generate_aligned_offset(wordsize);
    }
    
    // Perform the benchmark
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        offset = offsets[i];
        
        // Measure only the time spent reading
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // Direct memory access based on wordsize
        switch (wordsize) {
            case 8:
                value = *((volatile uint8_t *)((char *)mapped_base + offset - START_OFFSET));
                break;
            case 16:
                value = *((volatile uint16_t *)((char *)mapped_base + offset - START_OFFSET));
                break;
            case 32:
                value = *((volatile uint32_t *)((char *)mapped_base + offset - START_OFFSET));
                break;
            case 64:
                value = *((volatile uint64_t *)((char *)mapped_base + offset - START_OFFSET));
                break;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        elapsed_us = time_diff_us(start, end);
        total_us += elapsed_us;
        
        printf("  Read %2d: Address=0x%08X, Value=0x%lx, Time=%lu μs\n", 
               i+1, BASE_ADDR + offset, value, elapsed_us);
    }
    
    avg_time = (double)total_us / NUM_OPERATIONS;
    printf("Total read time for wordsize %d: %lu μs\n", wordsize, total_us);
    printf("Average read time for wordsize %d: %.2f μs\n\n", wordsize, avg_time);
    
    // Save the average time for the summary table
    if (wordsize == 8) wordsize_8_read_avg = avg_time;
    else if (wordsize == 16) wordsize_16_read_avg = avg_time;
    else if (wordsize == 32) wordsize_32_read_avg = avg_time;
    else if (wordsize == 64) wordsize_64_read_avg = avg_time;
}

// Benchmark write operations
void benchmark_write(void *mapped_base, int wordsize) {
    struct timespec start, end;
    uint64_t elapsed_us, total_us = 0;
    uint32_t offset;
    uint64_t written_value = 0x12345678; // Test pattern
    double avg_time;
    
    printf("Benchmarking WRITE operations with wordsize %d...\n", wordsize);
    
    // Pre-generate all offsets and values to remove that overhead from the timing
    uint32_t offsets[NUM_OPERATIONS];
    uint64_t values[NUM_OPERATIONS];
    
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        offsets[i] = generate_aligned_offset(wordsize);
        
        // Adjust write value based on wordsize
        switch (wordsize) {
            case 8:
                values[i] = 0x78;  // Use only the least significant byte
                break;
            case 16:
                values[i] = 0x5678;  // Use only the least significant 2 bytes
                break;
            case 32:
                values[i] = 0x12345678;  // Use all 4 bytes
                break;
            case 64:
                values[i] = 0x1234567812345678ULL;  // Use all 8 bytes
                break;
        }
    }
    
    // Perform the benchmark
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        offset = offsets[i];
        written_value = values[i];
        
        // Measure only the time spent writing
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // Direct memory access based on wordsize
        switch (wordsize) {
            case 8:
                *((volatile uint8_t *)((char *)mapped_base + offset - START_OFFSET)) = (uint8_t)written_value;
                break;
            case 16:
                *((volatile uint16_t *)((char *)mapped_base + offset - START_OFFSET)) = (uint16_t)written_value;
                break;
            case 32:
                *((volatile uint32_t *)((char *)mapped_base + offset - START_OFFSET)) = (uint32_t)written_value;
                break;
            case 64:
                *((volatile uint64_t *)((char *)mapped_base + offset - START_OFFSET)) = written_value;
                break;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        elapsed_us = time_diff_us(start, end);
        total_us += elapsed_us;
        
        printf("  Write %2d: Address=0x%08X, Value=0x%lx, Time=%lu μs\n", 
               i+1, BASE_ADDR + offset, written_value, elapsed_us);
    }
    
    avg_time = (double)total_us / NUM_OPERATIONS;
    printf("Total write time for wordsize %d: %lu μs\n", wordsize, total_us);
    printf("Average write time for wordsize %d: %.2f μs\n\n", wordsize, avg_time);
    
    // Save the average time for the summary table
    if (wordsize == 8) wordsize_8_write_avg = avg_time;
    else if (wordsize == 16) wordsize_16_write_avg = avg_time;
    else if (wordsize == 32) wordsize_32_write_avg = avg_time;
    else if (wordsize == 64) wordsize_64_write_avg = avg_time;
}

int main() {
    int fd;
    void *map_base;
    int wordsizes[] = {8, 16, 32, 64};
    int num_sizes = sizeof(wordsizes) / sizeof(wordsizes[0]);
    
    // Initialize random number generator
    srand(time(NULL));
    
    printf("AES Register Access mmap Benchmark\n");
    printf("=================================\n");
    printf("Address range: 0x%08X - 0x%08X\n", BASE_ADDR + START_OFFSET, BASE_ADDR + END_OFFSET);
    printf("Operations per test: %d\n\n", NUM_OPERATIONS);
    
    // Open /dev/mem for physical memory access
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        printf("Error opening /dev/mem: %s\n", strerror(errno));
        return 1;
    }
    
    // Map the AES register region
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BASE_ADDR + START_OFFSET);
    if (map_base == MAP_FAILED) {
        printf("Error mapping memory: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("Memory mapped successfully at virtual address %p\n\n", map_base);
    
    // Benchmark read operations for each wordsize
    printf("\n===== READ BENCHMARKS =====\n\n");
    for (int i = 0; i < num_sizes; i++) {
        benchmark_read(map_base, wordsizes[i]);
    }
    
    // Benchmark write operations for each wordsize
    printf("\n===== WRITE BENCHMARKS =====\n\n");
    for (int i = 0; i < num_sizes; i++) {
        benchmark_write(map_base, wordsizes[i]);
    }
    
    // Print a summary table
    printf("\nBenchmark Summary\n");
    printf("================\n");
    printf("Wordsize | Avg Read Time (μs) | Avg Write Time (μs)\n");
    printf("---------|-------------------|-------------------\n");
    printf("    8    |       %6.2f        |      %6.2f        \n", 
           wordsize_8_read_avg, wordsize_8_write_avg);
    printf("   16    |       %6.2f        |      %6.2f        \n", 
           wordsize_16_read_avg, wordsize_16_write_avg);
    printf("   32    |       %6.2f        |      %6.2f        \n", 
           wordsize_32_read_avg, wordsize_32_write_avg);
    printf("   64    |       %6.2f        |      %6.2f        \n", 
           wordsize_64_read_avg, wordsize_64_write_avg);
    
    // Unmap and close
    if (munmap(map_base, MAP_SIZE) == -1) {
        printf("Error unmapping memory: %s\n", strerror(errno));
    }
    
    close(fd);
    return 0;
}