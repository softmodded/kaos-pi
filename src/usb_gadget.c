#include "usb_gadget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

// USB HID Report Descriptor for Skylander Portal
// This descriptor matches the official Skylander Portal
static const uint8_t hid_report_descriptor[] = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (0x01)
    0xA1, 0x01,        // Collection (Application)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x20,        //   Usage Maximum (0x20)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x20,        //   Report Count (32)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x20,        //   Usage Maximum (0x20)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

// File descriptors
static int hidg_fd = -1;
static char udc_name[256] = {0};

/**
 * Write a string to a sysfs file
 */
static int write_sysfs(const char* path, const char* value) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open sysfs file");
        fprintf(stderr, "Path: %s\n", path);
        return -1;
    }
    
    ssize_t len = strlen(value);
    ssize_t written = write(fd, value, len);
    close(fd);
    
    if (written != len) {
        perror("Failed to write to sysfs");
        fprintf(stderr, "Path: %s, Value: %s\n", path, value);
        return -1;
    }
    
    return 0;
}

/**
 * Create a directory (if it doesn't exist)
 */
static int mkdir_p(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return 0; // Already exists
    }
    
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create directory");
        fprintf(stderr, "Path: %s\n", path);
        return -1;
    }
    
    return 0;
}

/**
 * Write binary data to a file
 */
static int write_binary_file(const char* path, const uint8_t* data, size_t length) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to open file for writing");
        fprintf(stderr, "Path: %s\n", path);
        return -1;
    }
    
    ssize_t written = write(fd, data, length);
    close(fd);
    
    if (written != (ssize_t)length) {
        perror("Failed to write binary data");
        return -1;
    }
    
    return 0;
}

/**
 * Remove directory recursively
 */
static void rmdir_recursive(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        
        if (entry->d_type == DT_DIR) {
            rmdir_recursive(fullpath);
        } else {
            unlink(fullpath);
        }
    }
    
    closedir(dir);
    rmdir(path);
}

/**
 * Find the USB Device Controller (UDC) name
 */
const char* usb_gadget_get_udc(void) {
    if (udc_name[0] != '\0') {
        return udc_name;
    }
    
    DIR* dir = opendir("/sys/class/udc");
    if (!dir) {
        perror("Failed to open /sys/class/udc");
        return NULL;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            strncpy(udc_name, entry->d_name, sizeof(udc_name) - 1);
            break;
        }
    }
    
    closedir(dir);
    
    if (udc_name[0] == '\0') {
        fprintf(stderr, "No UDC found. Is dwc2 module loaded?\n");
        return NULL;
    }
    
    printf("Found UDC: %s\n", udc_name);
    return udc_name;
}

/**
 * Initialize USB gadget mode
 */
int usb_gadget_init(void) {
    char path[512];
    
    printf("Initializing USB Gadget for Skylander Portal...\n");
    
    // Check if configfs is mounted
    struct stat st;
    if (stat("/sys/kernel/config/usb_gadget", &st) != 0) {
        fprintf(stderr, "USB Gadget ConfigFS not found. Is configfs mounted?\n");
        fprintf(stderr, "Try: mount -t configfs none /sys/kernel/config\n");
        return -1;
    }
    
    // Remove existing gadget if present
    usb_gadget_cleanup();
    
    // Create gadget directory
    if (mkdir_p(GADGET_BASE_PATH) < 0) {
        return -1;
    }
    
    // Set USB device class
    snprintf(path, sizeof(path), "%s/bDeviceClass", GADGET_BASE_PATH);
    if (write_sysfs(path, "0x00") < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/bDeviceSubClass", GADGET_BASE_PATH);
    if (write_sysfs(path, "0x00") < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/bDeviceProtocol", GADGET_BASE_PATH);
    if (write_sysfs(path, "0x00") < 0) return -1;
    
    // Set VID and PID
    snprintf(path, sizeof(path), "%s/idVendor", GADGET_BASE_PATH);
    if (write_sysfs(path, "0x1430") < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/idProduct", GADGET_BASE_PATH);
    if (write_sysfs(path, "0x0150") < 0) return -1;
    
    // Set USB version (USB 2.0)
    snprintf(path, sizeof(path), "%s/bcdUSB", GADGET_BASE_PATH);
    if (write_sysfs(path, "0x0200") < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/bcdDevice", GADGET_BASE_PATH);
    if (write_sysfs(path, "0x0100") < 0) return -1;
    
    // Create strings directory
    snprintf(path, sizeof(path), "%s/strings/0x409", GADGET_BASE_PATH);
    if (mkdir_p(path) < 0) return -1;
    
    // Set device strings
    snprintf(path, sizeof(path), "%s/strings/0x409/manufacturer", GADGET_BASE_PATH);
    if (write_sysfs(path, USB_MANUFACTURER) < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/strings/0x409/product", GADGET_BASE_PATH);
    if (write_sysfs(path, USB_PRODUCT) < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/strings/0x409/serialnumber", GADGET_BASE_PATH);
    if (write_sysfs(path, USB_SERIAL) < 0) return -1;
    
    // Create configuration
    snprintf(path, sizeof(path), "%s/configs/c.1", GADGET_BASE_PATH);
    if (mkdir_p(path) < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/configs/c.1/strings/0x409", GADGET_BASE_PATH);
    if (mkdir_p(path) < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/configs/c.1/strings/0x409/configuration", GADGET_BASE_PATH);
    if (write_sysfs(path, "Skylander Portal Config") < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/configs/c.1/MaxPower", GADGET_BASE_PATH);
    if (write_sysfs(path, "500") < 0) return -1;
    
    // Create HID function
    snprintf(path, sizeof(path), "%s/functions/hid.usb0", GADGET_BASE_PATH);
    if (mkdir_p(path) < 0) return -1;
    
    // Configure HID function
    snprintf(path, sizeof(path), "%s/functions/hid.usb0/protocol", GADGET_BASE_PATH);
    if (write_sysfs(path, "0") < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/functions/hid.usb0/subclass", GADGET_BASE_PATH);
    if (write_sysfs(path, "0") < 0) return -1;
    
    snprintf(path, sizeof(path), "%s/functions/hid.usb0/report_length", GADGET_BASE_PATH);
    if (write_sysfs(path, "32") < 0) return -1;
    
    // Write HID report descriptor
    snprintf(path, sizeof(path), "%s/functions/hid.usb0/report_desc", GADGET_BASE_PATH);
    if (write_binary_file(path, hid_report_descriptor, sizeof(hid_report_descriptor)) < 0) {
        return -1;
    }
    
    // Link function to configuration
    char link_src[512], link_dst[512];
    snprintf(link_src, sizeof(link_src), "%s/functions/hid.usb0", GADGET_BASE_PATH);
    snprintf(link_dst, sizeof(link_dst), "%s/configs/c.1/hid.usb0", GADGET_BASE_PATH);
    
    if (symlink(link_src, link_dst) < 0 && errno != EEXIST) {
        perror("Failed to link function to configuration");
        return -1;
    }
    
    printf("USB Gadget initialized successfully\n");
    return 0;
}

/**
 * Start the USB gadget
 */
int usb_gadget_start(void) {
    char path[512];
    
    const char* udc = usb_gadget_get_udc();
    if (!udc) {
        return -1;
    }
    
    printf("Starting USB Gadget with UDC: %s\n", udc);
    
    // Bind to UDC
    snprintf(path, sizeof(path), "%s/UDC", GADGET_BASE_PATH);
    if (write_sysfs(path, udc) < 0) {
        fprintf(stderr, "Failed to bind to UDC. Make sure no other gadget is active.\n");
        return -1;
    }
    
    // Open HID device file
    sleep(1); // Give kernel time to create the device
    
    hidg_fd = open("/dev/hidg0", O_RDWR | O_NONBLOCK);
    if (hidg_fd < 0) {
        perror("Failed to open /dev/hidg0");
        fprintf(stderr, "The HID gadget device was not created.\n");
        return -1;
    }
    
    printf("USB Gadget started successfully (fd=%d)\n", hidg_fd);
    return 0;
}

/**
 * Stop the USB gadget
 */
void usb_gadget_stop(void) {
    char path[512];
    
    if (hidg_fd >= 0) {
        close(hidg_fd);
        hidg_fd = -1;
    }
    
    // Unbind from UDC
    snprintf(path, sizeof(path), "%s/UDC", GADGET_BASE_PATH);
    write_sysfs(path, "");
    
    printf("USB Gadget stopped\n");
}

/**
 * Cleanup USB gadget configuration
 */
void usb_gadget_cleanup(void) {
    char path[512];
    
    usb_gadget_stop();
    
    // Unlink function from configuration
    snprintf(path, sizeof(path), "%s/configs/c.1/hid.usb0", GADGET_BASE_PATH);
    unlink(path);
    
    // Remove configuration strings
    snprintf(path, sizeof(path), "%s/configs/c.1/strings/0x409", GADGET_BASE_PATH);
    rmdir(path);
    
    // Remove configuration
    snprintf(path, sizeof(path), "%s/configs/c.1", GADGET_BASE_PATH);
    rmdir(path);
    
    // Remove function
    snprintf(path, sizeof(path), "%s/functions/hid.usb0", GADGET_BASE_PATH);
    rmdir(path);
    
    // Remove strings
    snprintf(path, sizeof(path), "%s/strings/0x409", GADGET_BASE_PATH);
    rmdir(path);
    
    // Remove gadget
    rmdir(GADGET_BASE_PATH);
    
    printf("USB Gadget cleaned up\n");
}

/**
 * Read data from USB host
 */
int usb_gadget_read(uint8_t* buffer, size_t max_length) {
    if (hidg_fd < 0) {
        return -1;
    }
    
    ssize_t bytes = read(hidg_fd, buffer, max_length);
    if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("USB read error");
        return -1;
    }
    
    return bytes > 0 ? bytes : 0;
}

/**
 * Write data to USB host
 */
int usb_gadget_write(const uint8_t* buffer, size_t length) {
    if (hidg_fd < 0) {
        return -1;
    }
    
    ssize_t bytes = write(hidg_fd, buffer, length);
    if (bytes < 0) {
        perror("USB write error");
        return -1;
    }
    
    return bytes;
}

/**
 * Check if USB gadget is connected
 */
int usb_gadget_is_connected(void) {
    return hidg_fd >= 0;
}