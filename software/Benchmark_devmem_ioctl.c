#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "KR260_ioctl.h"

#define MIN_ADDR 0xA0002000
#define MAX_ADDR 0xA000F000
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

// Generate a random aligned address within the specified range
uint32_t generate_aligned_address(int wordsize) {
    uint32_t range = MAX_ADDR - MIN_ADDR;
    uint32_t alignment;
    uint32_t addr;
    
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
    
    // Generate random address and align it
    addr = MIN_ADDR + (rand() % range);
    addr = addr & ~(alignment - 1);  // Mask out lower bits to ensure alignment
    
    // Make sure address doesn't exceed max_addr
    if (addr > MAX_ADDR)
        addr = MAX_ADDR & ~(alignment - 1);
        
    return addr;
}

// Benchmark read operations
void benchmark_read(int wordsize) {
    struct timespec start, end;
    uint64_t elapsed_us, total_us = 0;
    uint64_t value;
    uint32_t addr;
    double avg_time;
    
    printf("Benchmarking READ operations with wordsize %d...\n", wordsize);
    
    // Pre-generate all addresses to remove that overhead from the timing
    uint32_t addresses[NUM_OPERATIONS];
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        addresses[i] = generate_aligned_address(wordsize);
    }
    
    // Perform the benchmark
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        addr = addresses[i];
        
        // Measure only the time spent in user_read
        clock_gettime(CLOCK_MONOTONIC, &start);
        value = user_read(addr, wordsize);
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        elapsed_us = time_diff_us(start, end);
        total_us += elapsed_us;
        
        // printf("  Read %2d: Address=0x%08X, Value=0x%lx, Time=%lu μs\n", 
            //    i+1, addr, value, elapsed_us);
    }
    
    avg_time = (double)total_us / NUM_OPERATIONS;
    printf("Total read time for wordsize %d: %lu μs\n", wordsize, total_us);
    printf("Average read time for wordsize %d: %.2f μs\n\n", wordsize, avg_time);
    
    // Save the average time for the summary table
    // (Hacky way to return the value - in a real application you'd use a proper return)
    if (wordsize == 8) wordsize_8_read_avg = avg_time;
    else if (wordsize == 16) wordsize_16_read_avg = avg_time;
    else if (wordsize == 32) wordsize_32_read_avg = avg_time;
    else if (wordsize == 64) wordsize_64_read_avg = avg_time;
}

// Benchmark write operations
void benchmark_write(int wordsize) {
    struct timespec start, end;
    uint64_t elapsed_us, total_us = 0;
    uint64_t value;
    uint32_t addr;
    uint64_t written_value = 0x12345678; // Test pattern
    double avg_time;
    
    printf("Benchmarking WRITE operations with wordsize %d...\n", wordsize);
    
    // Pre-generate all addresses and values to remove that overhead from the timing
    uint32_t addresses[NUM_OPERATIONS];
    uint64_t values[NUM_OPERATIONS];
    
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        addresses[i] = generate_aligned_address(wordsize);
        
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
        addr = addresses[i];
        written_value = values[i];
        
        // Measure only the time spent in user_write
        clock_gettime(CLOCK_MONOTONIC, &start);
        value = user_write(addr, wordsize, written_value);
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        elapsed_us = time_diff_us(start, end);
        total_us += elapsed_us;
        
        // printf("  Write %2d: Address=0x%08X, Value=0x%lx, Time=%lu μs\n", 
            //    i+1, addr, value, elapsed_us);
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
    int wordsizes[] = {8, 16, 32, 64};
    int num_sizes = sizeof(wordsizes) / sizeof(wordsizes[0]);
    
    // Initialize random number generator
    srand(time(NULL));
    
    printf("AES Register Access Precise Benchmark\n");
    printf("====================================\n");
    printf("Address range: 0x%08X - 0x%08X\n", MIN_ADDR, MAX_ADDR);
    printf("Operations per test: %d\n\n", NUM_OPERATIONS);
    
    // Benchmark read operations for each wordsize
    printf("\n===== READ BENCHMARKS =====\n\n");
    for (int i = 0; i < num_sizes; i++) {
        benchmark_read(wordsizes[i]);
    }
    
    // Benchmark write operations for each wordsize
    printf("\n===== WRITE BENCHMARKS =====\n\n");
    for (int i = 0; i < num_sizes; i++) {
        benchmark_write(wordsizes[i]);
    }
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
    
    return 0;
    
}