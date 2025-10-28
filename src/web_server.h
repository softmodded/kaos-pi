#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdint.h>
#include "portal.h"

/**
 * Web Server for Skylander File Management
 * Provides HTTP interface for uploading and managing Skylander files
 */

// Web server configuration
#define WEB_SERVER_PORT 8080
#define WEB_SERVER_MAX_CONNECTIONS 10
#define WEB_SERVER_UPLOAD_MAX_SIZE (2 * 1024 * 1024)  // 2MB max upload

// Web server state
typedef struct {
    int socket_fd;
    int port;
    int running;
    portal_t* portal;
} web_server_t;

// Function Prototypes

/**
 * Initialize the web server
 * Returns 0 on success, -1 on error
 */
int web_server_init(web_server_t* server, portal_t* portal, int port);

/**
 * Start the web server in a separate thread
 * Returns 0 on success, -1 on error
 */
int web_server_start(web_server_t* server);

/**
 * Stop the web server
 */
void web_server_stop(web_server_t* server);

/**
 * Cleanup web server resources
 */
void web_server_cleanup(web_server_t* server);

/**
 * Main server loop (runs in separate thread)
 */
void* web_server_thread(void* arg);

/**
 * Handle a client connection
 */
void web_server_handle_client(web_server_t* server, int client_fd);

/**
 * Get server status (for monitoring)
 */
int web_server_is_running(web_server_t* server);

#endif // WEB_SERVER_H