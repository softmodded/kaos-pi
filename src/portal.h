#ifndef PORTAL_H
#define PORTAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Skylander Portal Protocol Implementation
 * Based on reverse engineering of the official Skylander Portal
 */

// Portal Commands (from host)
#define CMD_ACTIVATE        0x41  // 'A' - Activate portal
#define CMD_DEACTIVATE      0x44  // 'D' - Deactivate portal
#define CMD_COLOR           0x43  // 'C' - Set LED color
#define CMD_READ            0x51  // 'Q' - Read block from Skylander
#define CMD_WRITE           0x57  // 'W' - Write block to Skylander
#define CMD_STATUS          0x53  // 'S' - Get portal status
#define CMD_READY           0x52  // 'R' - Portal ready query

// Portal Response Types
#define RESP_READY          0x52  // 'R' - Portal ready
#define RESP_STATUS         0x53  // 'S' - Status response
#define RESP_READ           0x51  // 'Q' - Read response
#define RESP_WRITE          0x57  // 'W' - Write response
#define RESP_ARRIVAL        0x49  // 'I' - Skylander arrived
#define RESP_REMOVAL        0x52  // 'R' - Skylander removed

// Skylander Slots
#define MAX_SKYLANDERS      16
#define SLOT_PLAYER_1       0x00
#define SLOT_PLAYER_2       0x01
#define SLOT_TRAP           0x10

// Skylander Data
#define SKYLANDER_BLOCK_SIZE    16
#define SKYLANDER_BLOCKS        64
#define SKYLANDER_DATA_SIZE     (SKYLANDER_BLOCK_SIZE * SKYLANDER_BLOCKS)  // 1024 bytes

// File extensions
#define EXT_BIN     ".bin"
#define EXT_DMP     ".dmp"
#define EXT_DUMP    ".dump"
#define EXT_SKY     ".sky"

// Portal State
typedef enum {
    PORTAL_STATE_IDLE,
    PORTAL_STATE_ACTIVATED,
    PORTAL_STATE_READY
} portal_state_t;

// Skylander Slot
typedef struct {
    bool active;                                    // Is slot occupied
    uint8_t data[SKYLANDER_DATA_SIZE];             // Skylander data (1KB)
    char filename[256];                             // Source filename
    uint32_t last_read_block;                       // Last block read
    uint32_t last_write_block;                      // Last block written
} skylander_slot_t;

// Portal State
typedef struct {
    portal_state_t state;                           // Current state
    skylander_slot_t slots[MAX_SKYLANDERS];        // Skylander slots
    uint8_t led_color[3];                           // RGB LED color
    bool auto_sense;                                // Auto-send status updates
} portal_t;

// Function Prototypes

/**
 * Initialize the portal
 */
int portal_init(portal_t* portal);

/**
 * Cleanup portal resources
 */
void portal_cleanup(portal_t* portal);

/**
 * Process a command from the host
 * Returns 1 if response should be sent, 0 otherwise, -1 on error
 */
int portal_process_command(portal_t* portal, const uint8_t* cmd, size_t cmd_len,
                          uint8_t* response, size_t* response_len);

/**
 * Load a Skylander into a slot
 * Returns 0 on success, -1 on error
 */
int portal_load_skylander(portal_t* portal, uint8_t slot, const char* filename);

/**
 * Unload a Skylander from a slot
 */
void portal_unload_skylander(portal_t* portal, uint8_t slot);

/**
 * Get skylander in slot (if any)
 * Returns pointer to slot or NULL if empty
 */
skylander_slot_t* portal_get_skylander(portal_t* portal, uint8_t slot);

/**
 * Check if a slot is occupied
 */
bool portal_slot_active(portal_t* portal, uint8_t slot);

/**
 * Get portal status as bitmask
 * Each bit represents a slot (1 = occupied, 0 = empty)
 */
uint16_t portal_get_status(portal_t* portal);

/**
 * Activate the portal
 */
void portal_activate(portal_t* portal);

/**
 * Deactivate the portal
 */
void portal_deactivate(portal_t* portal);

/**
 * Set LED color
 */
void portal_set_color(portal_t* portal, uint8_t r, uint8_t g, uint8_t b);

/**
 * Read a block from a Skylander
 * Returns 0 on success, -1 on error
 */
int portal_read_block(portal_t* portal, uint8_t slot, uint8_t block,
                     uint8_t* data);

/**
 * Write a block to a Skylander
 * Returns 0 on success, -1 on error
 */
int portal_write_block(portal_t* portal, uint8_t slot, uint8_t block,
                      const uint8_t* data);

/**
 * Save Skylander data back to file
 * Returns 0 on success, -1 on error
 */
int portal_save_skylander(portal_t* portal, uint8_t slot);

/**
 * List available Skylander files
 * Returns array of filenames (must be freed by caller)
 */
char** portal_list_skylanders(int* count);

/**
 * Check if filename has valid Skylander extension
 */
bool portal_is_valid_extension(const char* filename);

#endif // PORTAL_H