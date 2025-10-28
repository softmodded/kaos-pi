#include "portal.h"
#include "crypto/skylander_crypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Skylander storage directory
#define SKYLANDERS_DIR "./skylanders"

/**
 * Initialize the portal
 */
int portal_init(portal_t* portal) {
    if (!portal) return -1;
    
    memset(portal, 0, sizeof(portal_t));
    portal->state = PORTAL_STATE_IDLE;
    portal->auto_sense = true;
    
    // Initialize all slots as inactive
    for (int i = 0; i < MAX_SKYLANDERS; i++) {
        portal->slots[i].active = false;
    }
    
    // Create skylanders directory if it doesn't exist
    struct stat st;
    if (stat(SKYLANDERS_DIR, &st) != 0) {
        mkdir(SKYLANDERS_DIR, 0755);
    }
    
    printf("Portal initialized\n");
    return 0;
}

/**
 * Cleanup portal resources
 */
void portal_cleanup(portal_t* portal) {
    if (!portal) return;
    
    // Unload all Skylanders
    for (int i = 0; i < MAX_SKYLANDERS; i++) {
        if (portal->slots[i].active) {
            portal_save_skylander(portal, i);
            portal_unload_skylander(portal, i);
        }
    }
    
    printf("Portal cleaned up\n");
}

/**
 * Activate the portal
 */
void portal_activate(portal_t* portal) {
    if (!portal) return;
    
    portal->state = PORTAL_STATE_ACTIVATED;
    printf("Portal activated\n");
}

/**
 * Deactivate the portal
 */
void portal_deactivate(portal_t* portal) {
    if (!portal) return;
    
    portal->state = PORTAL_STATE_IDLE;
    printf("Portal deactivated\n");
}

/**
 * Set LED color
 */
void portal_set_color(portal_t* portal, uint8_t r, uint8_t g, uint8_t b) {
    if (!portal) return;
    
    portal->led_color[0] = r;
    portal->led_color[1] = g;
    portal->led_color[2] = b;
    
    printf("LED color set to R:%d G:%d B:%d\n", r, g, b);
}

/**
 * Get portal status as bitmask
 */
uint16_t portal_get_status(portal_t* portal) {
    if (!portal) return 0;
    
    uint16_t status = 0;
    for (int i = 0; i < MAX_SKYLANDERS; i++) {
        if (portal->slots[i].active) {
            status |= (1 << i);
        }
    }
    
    return status;
}

/**
 * Check if a slot is occupied
 */
bool portal_slot_active(portal_t* portal, uint8_t slot) {
    if (!portal || slot >= MAX_SKYLANDERS) return false;
    return portal->slots[slot].active;
}

/**
 * Get skylander in slot
 */
skylander_slot_t* portal_get_skylander(portal_t* portal, uint8_t slot) {
    if (!portal || slot >= MAX_SKYLANDERS) return NULL;
    if (!portal->slots[slot].active) return NULL;
    return &portal->slots[slot];
}

/**
 * Check if filename has valid Skylander extension
 */
bool portal_is_valid_extension(const char* filename) {
    if (!filename) return false;
    
    size_t len = strlen(filename);
    if (len < 4) return false;
    
    const char* ext = filename + len - 4;
    
    return (strcasecmp(ext, EXT_BIN) == 0 ||
            strcasecmp(ext, EXT_DMP) == 0 ||
            strcasecmp(ext, EXT_SKY) == 0 ||
            (len >= 5 && strcasecmp(filename + len - 5, EXT_DUMP) == 0));
}

/**
 * Load a Skylander into a slot
 */
int portal_load_skylander(portal_t* portal, uint8_t slot, const char* filename) {
    if (!portal || !filename || slot >= MAX_SKYLANDERS) {
        return -1;
    }
    
    // Check file extension
    if (!portal_is_valid_extension(filename)) {
        fprintf(stderr, "Invalid file extension: %s\n", filename);
        return -1;
    }
    
    // Build full path
    char filepath[512];
    if (filename[0] == '/') {
        strncpy(filepath, filename, sizeof(filepath) - 1);
    } else {
        snprintf(filepath, sizeof(filepath), "%s/%s", SKYLANDERS_DIR, filename);
    }
    
    // Open file
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        perror("Failed to open Skylander file");
        fprintf(stderr, "File: %s\n", filepath);
        return -1;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (size < SKYLANDER_DATA_SIZE) {
        fprintf(stderr, "File too small: %ld bytes (expected %d)\n", 
                size, SKYLANDER_DATA_SIZE);
        fclose(fp);
        return -1;
    }
    
    // Read data
    skylander_slot_t* skylander = &portal->slots[slot];
    size_t read_bytes = fread(skylander->data, 1, SKYLANDER_DATA_SIZE, fp);
    fclose(fp);
    
    if (read_bytes != SKYLANDER_DATA_SIZE) {
        fprintf(stderr, "Failed to read complete Skylander data\n");
        return -1;
    }
    
    // Mark slot as active
    skylander->active = true;
    strncpy(skylander->filename, filename, sizeof(skylander->filename) - 1);
    skylander->last_read_block = 0;
    skylander->last_write_block = 0;
    
    printf("Loaded Skylander '%s' into slot %d\n", filename, slot);
    
    return 0;
}

/**
 * Unload a Skylander from a slot
 */
void portal_unload_skylander(portal_t* portal, uint8_t slot) {
    if (!portal || slot >= MAX_SKYLANDERS) return;
    
    skylander_slot_t* skylander = &portal->slots[slot];
    
    if (skylander->active) {
        printf("Unloaded Skylander from slot %d\n", slot);
        memset(skylander, 0, sizeof(skylander_slot_t));
        skylander->active = false;
    }
}

/**
 * Read a block from a Skylander
 */
int portal_read_block(portal_t* portal, uint8_t slot, uint8_t block,
                     uint8_t* data) {
    if (!portal || !data || slot >= MAX_SKYLANDERS) {
        return -1;
    }
    
    skylander_slot_t* skylander = portal_get_skylander(portal, slot);
    if (!skylander) {
        fprintf(stderr, "No Skylander in slot %d\n", slot);
        return -1;
    }
    
    if (block >= SKYLANDER_BLOCKS) {
        fprintf(stderr, "Invalid block number: %d\n", block);
        return -1;
    }
    
    // Copy block data
    uint32_t offset = block * SKYLANDER_BLOCK_SIZE;
    memcpy(data, skylander->data + offset, SKYLANDER_BLOCK_SIZE);
    
    skylander->last_read_block = block;
    
    return 0;
}

/**
 * Write a block to a Skylander
 */
int portal_write_block(portal_t* portal, uint8_t slot, uint8_t block,
                      const uint8_t* data) {
    if (!portal || !data || slot >= MAX_SKYLANDERS) {
        return -1;
    }
    
    skylander_slot_t* skylander = portal_get_skylander(portal, slot);
    if (!skylander) {
        fprintf(stderr, "No Skylander in slot %d\n", slot);
        return -1;
    }
    
    if (block >= SKYLANDER_BLOCKS) {
        fprintf(stderr, "Invalid block number: %d\n", block);
        return -1;
    }
    
    // Write block data
    uint32_t offset = block * SKYLANDER_BLOCK_SIZE;
    memcpy(skylander->data + offset, data, SKYLANDER_BLOCK_SIZE);
    
    skylander->last_write_block = block;
    
    printf("Wrote block %d to slot %d\n", block, slot);
    
    return 0;
}

/**
 * Save Skylander data back to file
 */
int portal_save_skylander(portal_t* portal, uint8_t slot) {
    if (!portal || slot >= MAX_SKYLANDERS) {
        return -1;
    }
    
    skylander_slot_t* skylander = portal_get_skylander(portal, slot);
    if (!skylander) {
        return -1;
    }
    
    // Build full path
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", SKYLANDERS_DIR, skylander->filename);
    
    // Open file for writing
    FILE* fp = fopen(filepath, "wb");
    if (!fp) {
        perror("Failed to save Skylander file");
        return -1;
    }
    
    // Write data
    size_t written = fwrite(skylander->data, 1, SKYLANDER_DATA_SIZE, fp);
    fclose(fp);
    
    if (written != SKYLANDER_DATA_SIZE) {
        fprintf(stderr, "Failed to write complete Skylander data\n");
        return -1;
    }
    
    printf("Saved Skylander to '%s'\n", filepath);
    return 0;
}

/**
 * List available Skylander files
 */
char** portal_list_skylanders(int* count) {
    if (!count) return NULL;
    
    *count = 0;
    
    DIR* dir = opendir(SKYLANDERS_DIR);
    if (!dir) {
        return NULL;
    }
    
    // Count valid files
    struct dirent* entry;
    int file_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && portal_is_valid_extension(entry->d_name)) {
            file_count++;
        }
    }
    
    if (file_count == 0) {
        closedir(dir);
        return NULL;
    }
    
    // Allocate array
    char** files = (char**)malloc(sizeof(char*) * file_count);
    if (!files) {
        closedir(dir);
        return NULL;
    }
    
    // Read filenames
    rewinddir(dir);
    int index = 0;
    while ((entry = readdir(dir)) != NULL && index < file_count) {
        if (entry->d_type == DT_REG && portal_is_valid_extension(entry->d_name)) {
            files[index] = strdup(entry->d_name);
            index++;
        }
    }
    
    closedir(dir);
    *count = index;
    return files;
}

/**
 * Process a command from the host
 */
int portal_process_command(portal_t* portal, const uint8_t* cmd, size_t cmd_len,
                          uint8_t* response, size_t* response_len) {
    if (!portal || !cmd || !response || !response_len || cmd_len == 0) {
        return -1;
    }
    
    uint8_t command = cmd[0];
    *response_len = 0;
    
    switch (command) {
        case CMD_ACTIVATE: {
            // Activate portal
            portal_activate(portal);
            
            // Send ready response
            response[0] = RESP_READY;
            *response_len = 1;
            return 1;
        }
        
        case CMD_DEACTIVATE: {
            // Deactivate portal
            portal_deactivate(portal);
            return 0;
        }
        
        case CMD_COLOR: {
            // Set LED color: C R G B slot
            if (cmd_len >= 5) {
                uint8_t r = cmd[1];
                uint8_t g = cmd[2];
                uint8_t b = cmd[3];
                portal_set_color(portal, r, g, b);
            }
            return 0;
        }
        
        case CMD_STATUS: {
            // Get portal status
            uint16_t status = portal_get_status(portal);
            
            response[0] = RESP_STATUS;
            response[1] = (status >> 8) & 0xFF;
            response[2] = status & 0xFF;
            *response_len = 3;
            return 1;
        }
        
        case CMD_READ: {
            // Read block: Q slot block
            if (cmd_len < 3) {
                return -1;
            }
            
            uint8_t slot = cmd[1];
            uint8_t block = cmd[2];
            
            uint8_t block_data[SKYLANDER_BLOCK_SIZE];
            if (portal_read_block(portal, slot, block, block_data) == 0) {
                response[0] = RESP_READ;
                response[1] = slot;
                response[2] = block;
                memcpy(response + 3, block_data, SKYLANDER_BLOCK_SIZE);
                *response_len = 3 + SKYLANDER_BLOCK_SIZE;
                return 1;
            }
            return -1;
        }
        
        case CMD_WRITE: {
            // Write block: W slot block [16 bytes]
            if (cmd_len < 19) {
                return -1;
            }
            
            uint8_t slot = cmd[1];
            uint8_t block = cmd[2];
            
            if (portal_write_block(portal, slot, block, cmd + 3) == 0) {
                response[0] = RESP_WRITE;
                response[1] = slot;
                response[2] = block;
                *response_len = 3;
                return 1;
            }
            return -1;
        }
        
        case CMD_READY: {
            // Portal ready query
            if (portal->state == PORTAL_STATE_ACTIVATED || 
                portal->state == PORTAL_STATE_READY) {
                response[0] = RESP_READY;
                *response_len = 1;
                return 1;
            }
            return 0;
        }
        
        default:
            printf("Unknown command: 0x%02X\n", command);
            return 0;
    }
}