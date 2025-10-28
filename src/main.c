#include "usb_gadget.h"
#include "portal.h"
#include "web_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

/**
 * KAOS-Pi: Skylander Portal Emulator for Raspberry Pi
 * 
 * Main application entry point
 */

// Global state
static portal_t portal;
static web_server_t web_server;
static int running = 1;

/**
 * Signal handler for graceful shutdown
 */
void signal_handler(int signo) {
    printf("\nReceived signal %d, shutting down...\n", signo);
    running = 0;
}

/**
 * Portal communication thread
 * Handles USB communication with the host
 */
void* portal_thread(void* arg) {
    uint8_t buffer[PORTAL_BUFFER_SIZE];
    uint8_t response[PORTAL_BUFFER_SIZE];
    
    printf("Portal communication thread started\n");
    
    while (running) {
        // Read from USB
        int bytes = usb_gadget_read(buffer, sizeof(buffer));
        
        if (bytes > 0) {
            printf("Received %d bytes: ", bytes);
            for (int i = 0; i < bytes && i < 16; i++) {
                printf("%02X ", buffer[i]);
            }
            printf("\n");
            
            // Process command
            size_t response_len = 0;
            int should_respond = portal_process_command(&portal, buffer, bytes, 
                                                        response, &response_len);
            
            // Send response if needed
            if (should_respond > 0 && response_len > 0) {
                printf("Sending %zu bytes: ", response_len);
                for (size_t i = 0; i < response_len && i < 16; i++) {
                    printf("%02X ", response[i]);
                }
                printf("\n");
                
                usb_gadget_write(response, response_len);
            }
        } else if (bytes < 0) {
            // Error occurred
            if (!running) break;
            perror("USB read error");
            usleep(100000); // 100ms delay before retry
        } else {
            // No data, small delay
            usleep(10000); // 10ms
        }
    }
    
    printf("Portal communication thread stopped\n");
    return NULL;
}

/**
 * Print usage information
 */
void print_usage(const char* program) {
    printf("Usage: %s [OPTIONS]\n", program);
    printf("\n");
    printf("KAOS-Pi: Skylander Portal Emulator for Raspberry Pi Zero W\n");
    printf("\n");
    printf("Options:\n");
    printf("  -p PORT     Web server port (default: 8080)\n");
    printf("  -h          Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s              # Start with default settings\n", program);
    printf("  %s -p 80        # Use port 80 for web interface\n", program);
    printf("\n");
}

/**
 * Check if running with necessary privileges
 */
int check_privileges(void) {
    // Check if we can access configfs
    if (access("/sys/kernel/config/usb_gadget", F_OK) != 0) {
        fprintf(stderr, "ERROR: Cannot access USB gadget configfs.\n");
        fprintf(stderr, "Make sure:\n");
        fprintf(stderr, "  1. You are running as root (sudo)\n");
        fprintf(stderr, "  2. ConfigFS is mounted: mount -t configfs none /sys/kernel/config\n");
        fprintf(stderr, "  3. dwc2 module is loaded: modprobe dwc2\n");
        fprintf(stderr, "  4. libcomposite module is loaded: modprobe libcomposite\n");
        return -1;
    }
    
    return 0;
}

/**
 * Print system information
 */
void print_system_info(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                    KAOS-Pi Portal Emulator                     ║\n");
    printf("║              Skylander Portal for Raspberry Pi                 ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // Get IP address
    FILE* fp = popen("hostname -I 2>/dev/null | awk '{print $1}'", "r");
    if (fp) {
        char ip[64] = {0};
        if (fgets(ip, sizeof(ip), fp)) {
            // Remove newline
            ip[strcspn(ip, "\n")] = 0;
            if (strlen(ip) > 0) {
                printf("📡 Web Interface: http://%s:%d\n", ip, WEB_SERVER_PORT);
            }
        }
        pclose(fp);
    }
    
    printf("📡 Web Interface: http://localhost:%d\n", WEB_SERVER_PORT);
    printf("🎮 USB Portal: Waiting for host connection...\n");
    printf("\n");
    printf("Press Ctrl+C to stop\n");
    printf("\n");
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    int web_port = WEB_SERVER_PORT;
    
    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
            case 'p':
                web_port = atoi(optarg);
                if (web_port <= 0 || web_port > 65535) {
                    fprintf(stderr, "Invalid port number: %s\n", optarg);
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Check privileges
    if (check_privileges() != 0) {
        return 1;
    }
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Print banner
    print_system_info();
    
    // Initialize portal
    printf("Initializing portal...\n");
    if (portal_init(&portal) < 0) {
        fprintf(stderr, "Failed to initialize portal\n");
        return 1;
    }
    
    // Initialize USB gadget
    printf("Setting up USB gadget...\n");
    if (usb_gadget_init() < 0) {
        fprintf(stderr, "Failed to initialize USB gadget\n");
        portal_cleanup(&portal);
        return 1;
    }
    
    // Start USB gadget
    printf("Starting USB gadget...\n");
    if (usb_gadget_start() < 0) {
        fprintf(stderr, "Failed to start USB gadget\n");
        usb_gadget_cleanup();
        portal_cleanup(&portal);
        return 1;
    }
    
    // Initialize web server
    printf("Initializing web server on port %d...\n", web_port);
    if (web_server_init(&web_server, &portal, web_port) < 0) {
        fprintf(stderr, "Failed to initialize web server\n");
        usb_gadget_cleanup();
        portal_cleanup(&portal);
        return 1;
    }
    
    // Start web server
    printf("Starting web server...\n");
    if (web_server_start(&web_server) < 0) {
        fprintf(stderr, "Failed to start web server\n");
        web_server_cleanup(&web_server);
        usb_gadget_cleanup();
        portal_cleanup(&portal);
        return 1;
    }
    
    // Create portal communication thread
    pthread_t portal_tid;
    if (pthread_create(&portal_tid, NULL, portal_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create portal thread\n");
        web_server_cleanup(&web_server);
        usb_gadget_cleanup();
        portal_cleanup(&portal);
        return 1;
    }
    
    printf("\n✅ KAOS-Pi is running!\n\n");
    
    // Main loop - just wait for signal
    while (running) {
        sleep(1);
    }
    
    printf("\nShutting down...\n");
    
    // Cleanup
    printf("Stopping portal thread...\n");
    pthread_cancel(portal_tid);
    pthread_join(portal_tid, NULL);
    
    printf("Stopping web server...\n");
    web_server_cleanup(&web_server);
    
    printf("Stopping USB gadget...\n");
    usb_gadget_cleanup();
    
    printf("Cleaning up portal...\n");
    portal_cleanup(&portal);
    
    printf("Shutdown complete. Goodbye!\n");
    
    return 0;
}