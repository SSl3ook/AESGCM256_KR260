#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <time.h>

#define AES256_KEY_SIZE 32
#define AES256_GCM_IV_SIZE 12
#define AES256_GCM_TAG_SIZE 16
#define PLAINTEXT_SIZE 128

void handleErrors() {
    ERR_print_errors_fp(stderr);
    abort();
}

int decrypt_data(const unsigned char *ciphertext, int ciphertext_len, const unsigned char *tag,
            const unsigned char *key, const unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

    // Initialize the decryption operation
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
        handleErrors();

    // Initialize key and IV
    if (1 != EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();

    // Provide the ciphertext and tag to be decrypted, and obtain the plaintext output
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    // Set expected tag value
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES256_GCM_TAG_SIZE, (void *)tag))
        handleErrors();

    // Finalize the decryption
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0) {
        // Success
        plaintext_len += len;
        return plaintext_len;
    } else {
        // Verification failed
        return -1;
    }
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

    unsigned char ciphertext[] = {
        0x96, 0x34, 0xF6, 0x55, 0xAA, 0xB7, 0x88, 0xA2,
        0xAE, 0x68, 0x23, 0x57, 0x1F, 0xDA, 0x40, 0xB6,
        0x42, 0xF5, 0xE5, 0xEC, 0x08, 0x6B, 0x94, 0x12,
        0xF5, 0x30, 0xD0, 0x42, 0x66, 0x34, 0x25, 0xCF,
        0xF8, 0x8A, 0xA8, 0xEB, 0xC0, 0x93, 0xFB, 0xD6,
        0x8A, 0x4A, 0x32, 0x6E, 0xB5, 0x05, 0x73, 0x3F,
        0xFF, 0xC1, 0x52, 0x10, 0x9D, 0xF7, 0xEE, 0x7D,
        0xE8, 0xD5, 0xCD, 0x1D, 0xE5, 0x41, 0x1B, 0x64,
        0x47, 0x6C, 0x43, 0xA9, 0xE0, 0xA3, 0x4C, 0x63,
        0x96, 0x95, 0x39, 0x94, 0xCE, 0x60, 0xE6, 0x31,
        0xFF, 0x92, 0x33, 0x82, 0xF6, 0x5F, 0x3E, 0xFA,
        0xA8, 0x5B, 0xBC, 0x4B, 0x8B, 0x98, 0xD6, 0x41,
        0x70, 0xA6, 0x64, 0x29, 0xCB, 0xB1, 0xA8, 0xDE,
        0x45, 0x72, 0x6B, 0x57, 0x75, 0x1E, 0x7D, 0xC8,
        0xF4, 0x5D, 0xF5, 0x76, 0xEB, 0x1F, 0xF2, 0xD4,
        0x3F, 0x5C, 0xC3, 0xDA, 0xE0, 0xFE, 0x78, 0x4D
    };

    unsigned char tag[AES256_GCM_TAG_SIZE] = {
        0x1B, 0x45, 0xBD, 0x60, 0x6C, 0xDE, 0xED, 0x93,
        0x7E, 0xB0, 0xD0, 0x82, 0xBF, 0x0E, 0xA1, 0xD4
    };

    unsigned char decryptedtext[PLAINTEXT_SIZE];
    int decryptedtext_len;
    clock_t start, end;
    double cpu_time_used;

    // Measure decryption time
    start = clock();

    decryptedtext_len = decrypt_data(ciphertext, sizeof(ciphertext), tag, key, iv, decryptedtext);

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    if (decryptedtext_len < 0) {
        fprintf(stderr, "Decryption failed\n");
        return 1;
    }

    printf("Decrypted Plaintext is:\n");
    for (int i = 0; i < decryptedtext_len; ++i) {
        printf("%02X ", decryptedtext[i]);
    }
    printf("\n\n");

    printf("Decryption Time: %f seconds\n", cpu_time_used);

    return 0;
}
