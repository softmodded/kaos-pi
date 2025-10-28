#ifndef USB_GADGET_H
#define USB_GADGET_H

#include <stdint.h>

/**
 * USB Gadget Configuration for Skylander Portal Emulation
 * Configures the Raspberry Pi as a USB HID device matching the Skylander Portal
 */

// USB Vendor and Product IDs (Activision Skylander Portal)
#define USB_VENDOR_ID   0x1430
#define USB_PRODUCT_ID  0x0150

// USB Device Information
#define USB_MANUFACTURER "Activision"
#define USB_PRODUCT      "Skylands Portal"
#define USB_SERIAL       "KAOS-PI-001"

// USB HID Report Descriptor Size
#define HID_REPORT_DESC_SIZE 32

// USB Endpoint Configuration
#define USB_EP_IN   0x81  // Interrupt IN endpoint
#define USB_EP_OUT  0x01  // Interrupt OUT endpoint
#define USB_EP_SIZE 32    // Max packet size

// Portal Response Buffer Size
#define PORTAL_BUFFER_SIZE 64

// USB Gadget Paths (Linux ConfigFS)
#define GADGET_BASE_PATH "/sys/kernel/config/usb_gadget/skylander"
#define GADGET_FUNC_PATH GADGET_BASE_PATH "/functions/hid.usb0"
#define GADGET_CONFIG_PATH GADGET_BASE_PATH "/configs/c.1"

// Function Prototypes

/**
 * Initialize USB gadget mode
 * Configures the Pi as a Skylander Portal USB device
 * Returns 0 on success, -1 on error
 */
int usb_gadget_init(void);

/**
 * Cleanup and remove USB gadget configuration
 */
void usb_gadget_cleanup(void);

/**
 * Start the USB gadget (bind to UDC)
 * Returns 0 on success, -1 on error
 */
int usb_gadget_start(void);

/**
 * Stop the USB gadget (unbind from UDC)
 */
void usb_gadget_stop(void);

/**
 * Read data from USB host
 * Returns number of bytes read, -1 on error
 */
int usb_gadget_read(uint8_t* buffer, size_t max_length);

/**
 * Write data to USB host
 * Returns number of bytes written, -1 on error
 */
int usb_gadget_write(const uint8_t* buffer, size_t length);

/**
 * Check if USB gadget is connected to host
 * Returns 1 if connected, 0 if not
 */
int usb_gadget_is_connected(void);

/**
 * Get the UDC (USB Device Controller) name
 * Returns the UDC name or NULL if not found
 */
const char* usb_gadget_get_udc(void);

#endif // USB_GADGET_H