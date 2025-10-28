#include "skylander_crypt.h"
#include "rijndael.h"
#include <string.h>
#include <stdio.h>

// Skylander encryption key (from reverse engineering)
static const uint8_t SKYLANDER_KEY[16] = {
    0x20, 0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67,
    0x68, 0x74, 0x20, 0x28, 0x43, 0x29, 0x20, 0x32
};

// AES round keys
static unsigned long encrypt_rk[MAXNR + 1][4];
static unsigned long decrypt_rk[MAXNR + 1][4];
static int nrounds = 0;
static int crypto_initialized = 0;

/**
 * Initialize the encryption system
 */
static void init_crypto(void) {
    if (!crypto_initialized) {
        nrounds = rijndaelSetupEncrypt(encrypt_rk[0], SKYLANDER_KEY, KEYBITS);
        rijndaelSetupDecrypt(decrypt_rk[0], SKYLANDER_KEY, KEYBITS);
        crypto_initialized = 1;
    }
}

/**
 * Encrypt a single block of Skylander data
 */
void skylander_encrypt(uint8_t* buffer, uint32_t block) {
    init_crypto();
    
    uint8_t iv[16];
    memset(iv, 0, sizeof(iv));
    
    // Set IV based on block number
    iv[0] = (block >> 24) & 0xFF;
    iv[1] = (block >> 16) & 0xFF;
    iv[2] = (block >> 8) & 0xFF;
    iv[3] = block & 0xFF;
    
    // XOR with IV
    for (int i = 0; i < 16; i++) {
        buffer[i] ^= iv[i];
    }
    
    // Encrypt
    uint8_t output[16];
    rijndaelEncrypt(encrypt_rk[0], nrounds, buffer, output);
    memcpy(buffer, output, 16);
}

/**
 * Decrypt a single block of Skylander data
 */
void skylander_decrypt(uint8_t* buffer, uint32_t block) {
    init_crypto();
    
    uint8_t iv[16];
    memset(iv, 0, sizeof(iv));
    
    // Set IV based on block number
    iv[0] = (block >> 24) & 0xFF;
    iv[1] = (block >> 16) & 0xFF;
    iv[2] = (block >> 8) & 0xFF;
    iv[3] = block & 0xFF;
    
    // Decrypt
    uint8_t output[16];
    rijndaelDecrypt(decrypt_rk[0], nrounds, buffer, output);
    memcpy(buffer, output, 16);
    
    // XOR with IV
    for (int i = 0; i < 16; i++) {
        buffer[i] ^= iv[i];
    }
}

/**
 * Encrypt full Skylander data
 */
void skylander_encrypt_full(uint8_t* data, size_t length) {
    size_t blocks = length / BLOCK_SIZE;
    
    for (size_t i = 0; i < blocks; i++) {
        skylander_encrypt(data + (i * BLOCK_SIZE), i);
    }
}

/**
 * Decrypt full Skylander data
 */
void skylander_decrypt_full(uint8_t* data, size_t length) {
    size_t blocks = length / BLOCK_SIZE;
    
    for (size_t i = 0; i < blocks; i++) {
        skylander_decrypt(data + (i * BLOCK_SIZE), i);
    }
}

/**
 * Calculate Skylander checksum
 * Uses simple additive checksum with carry
 */
uint16_t skylander_checksum(const uint8_t* data, size_t length) {
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    
    // Fold 32-bit checksum to 16-bit
    while (checksum >> 16) {
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }
    
    return (uint16_t)(~checksum);
}

/**
 * Verify Skylander data checksum
 * Returns 1 if valid, 0 if invalid
 */
int skylander_verify_checksum(const uint8_t* data, size_t length) {
    if (length < 2) {
        return 0;
    }
    
    // Last 2 bytes are the checksum
    uint16_t stored_checksum = (data[length - 2] << 8) | data[length - 1];
    uint16_t calculated_checksum = skylander_checksum(data, length - 2);
    
    return stored_checksum == calculated_checksum;
}