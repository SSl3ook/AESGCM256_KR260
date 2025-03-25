#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <time.h>

#define AES256_KEY_SIZE 32
#define AES256_GCM_IV_SIZE 12
#define AES256_GCM_TAG_SIZE 16
#define PLAINTEXT_SIZE 1048576

void handleErrors() {
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(const unsigned char *plaintext, int plaintext_len, const unsigned char *key,
            const unsigned char *iv, unsigned char *ciphertext, unsigned char *tag) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    struct timespec start, end;
    double cpu_time_used;

    clock_gettime(CLOCK_MONOTONIC, &start);
    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (1.0e-9*end.tv_sec + end.tv_nsec) - (1.0e-9*start.tv_sec + start.tv_nsec);
    printf("1 : %f nanoseconds\n", cpu_time_used);

    clock_gettime(CLOCK_MONOTONIC, &start);
    // Initialize the encryption operation
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
        handleErrors();
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (1.0e-9*end.tv_sec + end.tv_nsec) - (1.0e-9*start.tv_sec + start.tv_nsec);
    printf("2 : %f nanoseconds\n", cpu_time_used);

    clock_gettime(CLOCK_MONOTONIC, &start);
    // Initialize key and IV
    if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (1.0e-9*end.tv_sec + end.tv_nsec) - (1.0e-9*start.tv_sec + start.tv_nsec);
    printf("3 : %f nanoseconds\n", cpu_time_used);

    clock_gettime(CLOCK_MONOTONIC, &start);
    // Provide the message to be encrypted, and obtain the encrypted output
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (1.0e-9*end.tv_sec + end.tv_nsec) - (1.0e-9*start.tv_sec + start.tv_nsec);
    printf("4 : %f nanoseconds\n", cpu_time_used);
    ciphertext_len = len;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    // Finalize the encryption
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (1.0e-9*end.tv_sec + end.tv_nsec) - (1.0e-9*start.tv_sec + start.tv_nsec);
    printf("5 : %f nanoseconds\n", cpu_time_used);
    ciphertext_len += len;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    // Get the tag
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES256_GCM_TAG_SIZE, tag))
        handleErrors();
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (1.0e-9*end.tv_sec + end.tv_nsec) - (1.0e-9*start.tv_sec + start.tv_nsec);
    printf("6 : %f nanoseconds\n", cpu_time_used);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int main() {
    unsigned char key[AES256_KEY_SIZE] = {
        0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
        0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
        0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
        0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78
    };

    unsigned char iv[AES256_GCM_IV_SIZE] = {
        0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
        0x12, 0x34, 0x56, 0x78
    };

    unsigned char plaintext[PLAINTEXT_SIZE];
    unsigned char ciphertext[PLAINTEXT_SIZE];
    unsigned char tag[AES256_GCM_TAG_SIZE];

    int ciphertext_len;
    struct timespec start, end;
    double cpu_time_used;

    // Fill plaintext with bytes from 0x00 to 0xFF repeatedly
    for (int i = 0; i < PLAINTEXT_SIZE; ++i) {
        plaintext[i] = i % 256;
    }

    // Measure encryption time
    clock_gettime(CLOCK_MONOTONIC, &start);

    ciphertext_len = encrypt(plaintext, PLAINTEXT_SIZE, key, iv, ciphertext, tag);

    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (1.0e-9*end.tv_sec + end.tv_nsec) - (1.0e-9*start.tv_sec + start.tv_nsec);
    
    // printf("Ciphertext is:\n");
    // for (int i = 0; i < ciphertext_len; ++i) {
    //     printf("%02X ", ciphertext[i]);
    // }
    // printf("\n");

    printf("Tag is:\n");
    for (int i = 0; i < AES256_GCM_TAG_SIZE; ++i) {
        printf("%02X ", tag[i]);
    }
    printf("\n");

    printf("Encryption Time: %f nanoseconds\n", cpu_time_used);

    return 0;
}
