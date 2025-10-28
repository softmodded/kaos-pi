#ifndef SKYLANDER_CRYPT_H
#define SKYLANDER_CRYPT_H

#include <stdint.h>
#include <stddef.h>

/**
 * Skylander encryption/decryption utilities
 * Based on the original KAOS implementation
 */

// Encryption constants
#define BLOCK_COUNT 0x10
#define BLOCK_SIZE 0x10

// Function prototypes
void skylander_encrypt(uint8_t* buffer, uint32_t block);
void skylander_decrypt(uint8_t* buffer, uint32_t block);
void skylander_encrypt_full(uint8_t* data, size_t length);
void skylander_decrypt_full(uint8_t* data, size_t length);

// Checksum utilities
uint16_t skylander_checksum(const uint8_t* data, size_t length);
int skylander_verify_checksum(const uint8_t* data, size_t length);

#endif // SKYLANDER_CRYPT_H