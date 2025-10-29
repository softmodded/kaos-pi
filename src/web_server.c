#include "web_server.h"
#include "portal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

// HTML pages embedded in the binary
static const char* HTML_INDEX = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>KAOS-Pi Portal Manager</title>\n"
"    <style>\n"
"        * { margin: 0; padding: 0; box-sizing: border-box; }\n"
"        body {\n"
"            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
"            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"
"            min-height: 100vh;\n"
"            padding: 20px;\n"
"        }\n"
"        .container {\n"
"            max-width: 800px;\n"
"            margin: 0 auto;\n"
"            background: white;\n"
"            border-radius: 10px;\n"
"            box-shadow: 0 10px 40px rgba(0,0,0,0.2);\n"
"            padding: 30px;\n"
"        }\n"
"        h1 {\n"
"            color: #667eea;\n"
"            margin-bottom: 10px;\n"
"            font-size: 2.5em;\n"
"        }\n"
"        .subtitle {\n"
"            color: #666;\n"
"            margin-bottom: 30px;\n"
"            font-size: 1.1em;\n"
"        }\n"
"        .upload-section {\n"
"            background: #f7f9fc;\n"
"            border: 2px dashed #667eea;\n"
"            border-radius: 8px;\n"
"            padding: 30px;\n"
"            text-align: center;\n"
"            margin-bottom: 30px;\n"
"            transition: all 0.3s;\n"
"        }\n"
"        .upload-section:hover {\n"
"            background: #eef1f7;\n"
"            border-color: #764ba2;\n"
"        }\n"
"        .upload-section.drag-over {\n"
"            background: #e3f2fd;\n"
"            border-color: #2196f3;\n"
"        }\n"
"        input[type=\"file\"] {\n"
"            display: none;\n"
"        }\n"
"        .upload-btn {\n"
"            background: #667eea;\n"
"            color: white;\n"
"            padding: 12px 30px;\n"
"            border: none;\n"
"            border-radius: 5px;\n"
"            cursor: pointer;\n"
"            font-size: 16px;\n"
"            transition: all 0.3s;\n"
"        }\n"
"        .upload-btn:hover {\n"
"            background: #764ba2;\n"
"            transform: translateY(-2px);\n"
"            box-shadow: 0 5px 15px rgba(0,0,0,0.2);\n"
"        }\n"
"        .file-list {\n"
"            margin-top: 20px;\n"
"        }\n"
"        .file-item {\n"
"            background: white;\n"
"            border: 1px solid #e0e0e0;\n"
"            border-radius: 5px;\n"
"            padding: 15px;\n"
"            margin-bottom: 10px;\n"
"            display: flex;\n"
"            justify-content: space-between;\n"
"            align-items: center;\n"
"            transition: all 0.3s;\n"
"        }\n"
"        .file-item:hover {\n"
"            box-shadow: 0 3px 10px rgba(0,0,0,0.1);\n"
"        }\n"
"        .file-name {\n"
"            font-weight: 500;\n"
"            color: #333;\n"
"        }\n"
"        .file-actions {\n"
"            display: flex;\n"
"            gap: 10px;\n"
"        }\n"
"        .btn {\n"
"            padding: 8px 15px;\n"
"            border: none;\n"
"            border-radius: 4px;\n"
"            cursor: pointer;\n"
"            font-size: 14px;\n"
"            transition: all 0.3s;\n"
"        }\n"
"        .btn-load {\n"
"            background: #4caf50;\n"
"            color: white;\n"
"        }\n"
"        .btn-load:hover {\n"
"            background: #45a049;\n"
"        }\n"
"        .btn-delete {\n"
"            background: #f44336;\n"
"            color: white;\n"
"        }\n"
"        .btn-delete:hover {\n"
"            background: #da190b;\n"
"        }\n"
"        .status {\n"
"            background: #e8f5e9;\n"
"            border-left: 4px solid #4caf50;\n"
"            padding: 15px;\n"
"            margin-bottom: 20px;\n"
"            border-radius: 4px;\n"
"        }\n"
"        .status.error {\n"
"            background: #ffebee;\n"
"            border-left-color: #f44336;\n"
"        }\n"
"        .portal-status {\n"
"            background: #fff3e0;\n"
"            border-left: 4px solid #ff9800;\n"
"            padding: 15px;\n"
"            margin-bottom: 20px;\n"
"            border-radius: 4px;\n"
"        }\n"
"        .slots {\n"
"            display: grid;\n"
"            grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));\n"
"            gap: 10px;\n"
"            margin-top: 15px;\n"
"        }\n"
"        .slot {\n"
"            background: #f5f5f5;\n"
"            padding: 10px;\n"
"            border-radius: 5px;\n"
"            text-align: center;\n"
"            border: 2px solid transparent;\n"
"        }\n"
"        .slot.active {\n"
"            background: #e8f5e9;\n"
"            border-color: #4caf50;\n"
"        }\n"
"        .slot-label {\n"
"            font-size: 12px;\n"
"            color: #666;\n"
"            margin-bottom: 5px;\n"
"        }\n"
"        .slot-name {\n"
"            font-weight: 500;\n"
"            font-size: 14px;\n"
"            color: #333;\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <h1>🎮 KAOS-Pi Portal Manager</h1>\n"
"        <p class=\"subtitle\">Skylander Portal Emulator for Raspberry Pi</p>\n"
"        \n"
"        <div id=\"statusMsg\" style=\"display:none;\"></div>\n"
"        \n"
"        <div class=\"portal-status\">\n"
"            <h3>Portal Status</h3>\n"
"            <p><strong>State:</strong> <span id=\"portalState\">Connected</span></p>\n"
"            <div class=\"slots\" id=\"slotsContainer\"></div>\n"
"        </div>\n"
"        \n"
"        <div class=\"upload-section\" id=\"dropZone\">\n"
"            <h2>📤 Upload Skylander Files</h2>\n"
"            <p style=\"margin: 15px 0; color: #666;\">Drag & drop files here or click to browse</p>\n"
"            <p style=\"margin-bottom: 15px; font-size: 14px; color: #999;\">Supported: .bin, .dmp, .dump, .sky</p>\n"
"            <input type=\"file\" id=\"fileInput\" multiple accept=\".bin,.dmp,.dump,.sky\">\n"
"            <button class=\"upload-btn\" onclick=\"document.getElementById('fileInput').click()\">Choose Files</button>\n"
"        </div>\n"
"        \n"
"        <div class=\"file-list\">\n"
"            <h2>📁 Available Skylanders</h2>\n"
"            <div id=\"fileListContainer\">Loading...</div>\n"
"        </div>\n"
"    </div>\n"
"    \n"
"    <script>\n"
"        const dropZone = document.getElementById('dropZone');\n"
"        const fileInput = document.getElementById('fileInput');\n"
"        \n"
"        // Prevent default drag behaviors\n"
"        ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {\n"
"            dropZone.addEventListener(eventName, preventDefaults, false);\n"
"            document.body.addEventListener(eventName, preventDefaults, false);\n"
"        });\n"
"        \n"
"        function preventDefaults(e) {\n"
"            e.preventDefault();\n"
"            e.stopPropagation();\n"
"        }\n"
"        \n"
"        // Highlight drop zone when dragging over it\n"
"        ['dragenter', 'dragover'].forEach(eventName => {\n"
"            dropZone.addEventListener(eventName, () => {\n"
"                dropZone.classList.add('drag-over');\n"
"            }, false);\n"
"        });\n"
"        \n"
"        ['dragleave', 'drop'].forEach(eventName => {\n"
"            dropZone.addEventListener(eventName, () => {\n"
"                dropZone.classList.remove('drag-over');\n"
"            }, false);\n"
"        });\n"
"        \n"
"        // Handle dropped files\n"
"        dropZone.addEventListener('drop', (e) => {\n"
"            const files = e.dataTransfer.files;\n"
"            handleFiles(files);\n"
"        }, false);\n"
"        \n"
"        // Handle selected files\n"
"        fileInput.addEventListener('change', (e) => {\n"
"            handleFiles(e.target.files);\n"
"        });\n"
"        \n"
"        function showStatus(message, isError = false) {\n"
"            const statusMsg = document.getElementById('statusMsg');\n"
"            statusMsg.textContent = message;\n"
"            statusMsg.className = isError ? 'status error' : 'status';\n"
"            statusMsg.style.display = 'block';\n"
"            setTimeout(() => {\n"
"                statusMsg.style.display = 'none';\n"
"            }, 5000);\n"
"        }\n"
"        \n"
"        function handleFiles(files) {\n"
"            for (let file of files) {\n"
"                uploadFile(file);\n"
"            }\n"
"        }\n"
"        \n"
"        function uploadFile(file) {\n"
"            const formData = new FormData();\n"
"            formData.append('file', file);\n"
"            \n"
"            fetch('/upload', {\n"
"                method: 'POST',\n"
"                body: formData\n"
"            })\n"
"            .then(response => response.text())\n"
"            .then(data => {\n"
"                showStatus('Uploaded: ' + file.name);\n"
"                loadFileList();\n"
"            })\n"
"            .catch(error => {\n"
"                showStatus('Upload failed: ' + error, true);\n"
"            });\n"
"        }\n"
"        \n"
"        function loadFileList() {\n"
"            fetch('/list')\n"
"            .then(response => response.json())\n"
"            .then(files => {\n"
"                const container = document.getElementById('fileListContainer');\n"
"                if (files.length === 0) {\n"
"                    container.innerHTML = '<p style=\"color: #999; padding: 20px; text-align: center;\">No Skylander files uploaded yet</p>';\n"
"                    return;\n"
"                }\n"
"                \n"
"                container.innerHTML = files.map(file => `\n"
"                    <div class=\"file-item\">\n"
"                        <span class=\"file-name\">📄 ${file}</span>\n"
"                        <div class=\"file-actions\">\n"
"                            <button class=\"btn btn-load\" onclick=\"loadSkylander('${file}', 0)\">Load Slot 1</button>\n"
"                            <button class=\"btn btn-load\" onclick=\"loadSkylander('${file}', 1)\">Load Slot 2</button>\n"
"                            <button class=\"btn btn-delete\" onclick=\"deleteFile('${file}')\">Delete</button>\n"
"                        </div>\n"
"                    </div>\n"
"                `).join('');\n"
"            })\n"
"            .catch(error => {\n"
"                document.getElementById('fileListContainer').innerHTML = '<p style=\"color: red;\">Error loading files</p>';\n"
"            });\n"
"        }\n"
"        \n"
"        function loadSkylander(filename, slot) {\n"
"            fetch(`/load?file=${encodeURIComponent(filename)}&slot=${slot}`, { method: 'POST' })\n"
"            .then(response => response.text())\n"
"            .then(data => {\n"
"                showStatus(`Loaded ${filename} into slot ${slot + 1}`);\n"
"                updatePortalStatus();\n"
"            })\n"
"            .catch(error => {\n"
"                showStatus('Load failed: ' + error, true);\n"
"            });\n"
"        }\n"
"        \n"
"        function deleteFile(filename) {\n"
"            if (!confirm(`Delete ${filename}?`)) return;\n"
"            \n"
"            fetch(`/delete?file=${encodeURIComponent(filename)}`, { method: 'POST' })\n"
"            .then(response => response.text())\n"
"            .then(data => {\n"
"                showStatus('Deleted: ' + filename);\n"
"                loadFileList();\n"
"            })\n"
"            .catch(error => {\n"
"                showStatus('Delete failed: ' + error, true);\n"
"            });\n"
"        }\n"
"        \n"
"        function updatePortalStatus() {\n"
"            fetch('/status')\n"
"            .then(response => response.json())\n"
"            .then(data => {\n"
"                const container = document.getElementById('slotsContainer');\n"
"                let html = '';\n"
"                for (let i = 0; i < 2; i++) {\n"
"                    const slot = data.slots[i];\n"
"                    const active = slot && slot.active;\n"
"                    html += `\n"
"                        <div class=\"slot ${active ? 'active' : ''}\">\n"
"                            <div class=\"slot-label\">Slot ${i + 1}</div>\n"
"                            <div class=\"slot-name\">${active ? slot.filename : 'Empty'}</div>\n"
"                        </div>\n"
"                    `;\n"
"                }\n"
"                container.innerHTML = html;\n"
"            })\n"
"            .catch(error => console.error('Status update failed:', error));\n"
"        }\n"
"        \n"
"        // Initial load\n"
"        loadFileList();\n"
"        updatePortalStatus();\n"
"        setInterval(updatePortalStatus, 5000);\n"
"    </script>\n"
"</body>\n"
"</html>";

/**
 * URL decode helper
 */
static void url_decode(char* dst, const char* src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

/**
 * Get query parameter value
 */
static char* get_query_param(const char* query, const char* param) {
    if (!query || !param) return NULL;
    
    char* search = (char*)malloc(strlen(param) + 2);
    sprintf(search, "%s=", param);
    
    char* pos = strstr(query, search);
    free(search);
    
    if (!pos) return NULL;
    
    pos += strlen(param) + 1;
    char* end = strchr(pos, '&');
    size_t len = end ? (end - pos) : strlen(pos);
    
    char* value = (char*)malloc(len + 1);
    strncpy(value, pos, len);
    value[len] = '\0';
    
    char* decoded = (char*)malloc(len + 1);
    url_decode(decoded, value);
    free(value);
    
    return decoded;
}

/**
 * Send HTTP response
 */
static void send_response(int client_fd, int code, const char* status, 
                         const char* content_type, const char* body) {
    char header[1024];
    int body_len = body ? strlen(body) : 0;
    
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, status, content_type, body_len);
    
    write(client_fd, header, strlen(header));
    if (body) {
        write(client_fd, body, body_len);
    }
}

/**
 * Handle file upload
 */
static void handle_upload(web_server_t* server, int client_fd, const char* body, size_t body_len) {
    // Parse multipart/form-data
    const char* boundary_start = strstr(body, "boundary=");
    if (!boundary_start) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "Invalid upload");
        return;
    }
    
    // Extract boundary
    char boundary[128];
    sscanf(boundary_start, "boundary=%s", boundary);
    
    // Find filename
    const char* filename_start = strstr(body, "filename=\"");
    if (!filename_start) {
        printf("ERROR: No filename found in body\n");
        send_response(client_fd, 400, "Bad Request", "text/plain", "No filename in upload");
        return;
    }
    
    filename_start += 10;
    const char* filename_end = strchr(filename_start, '"');
    if (!filename_end) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "Invalid filename format");
        return;
    }
    
    char filename[256];
    size_t name_len = filename_end - filename_start;
    if (name_len >= sizeof(filename)) name_len = sizeof(filename) - 1;
    strncpy(filename, filename_start, name_len);
    filename[name_len] = '\0';
    printf("Filename: %s\n", filename);
    
    // Find file data (after Content-Type line and blank line)
    const char* data_start = strstr(filename_end, "\r\n\r\n");
    if (!data_start) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "No data separator");
        return;
    }
    data_start += 4;
    
    // Find end of data (boundary)
    char boundary_end[128];
    snprintf(boundary_end, sizeof(boundary_end), "\r\n--%s", boundary);
    const char* data_end = strstr(data_start, boundary_end);
    if (!data_end) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "No end boundary");
        return;
    }
    
    size_t data_len = data_end - data_start;
    printf("File data length: %zu bytes\n", data_len);
    
    // Validate file size (should be 1024 bytes for Skylanders)
    if (data_len != 1024) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "Invalid file size: %zu bytes (expected 1024)", data_len);
        printf("ERROR: %s\n", err_msg);
        send_response(client_fd, 400, "Bad Request", "text/plain", err_msg);
        return;
    }
    
    // Save file to /var/lib/kaos-pi/skylanders/
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "/var/lib/kaos-pi/skylanders/%s", filename);
    
    printf("Saving to: %s\n", filepath);
    FILE* fp = fopen(filepath, "wb");
    if (!fp) {
        perror("Failed to open file for writing");
        printf("ERROR: Could not open %s for writing\n", filepath);
        send_response(client_fd, 500, "Internal Server Error", "text/plain", "Failed to save file");
        return;
    }
    
    size_t written = fwrite(data_start, 1, data_len, fp);
    fclose(fp);
    
    if (written != data_len) {
        printf("ERROR: Only wrote %zu of %zu bytes\n", written, data_len);
        send_response(client_fd, 500, "Internal Server Error", "text/plain", "Failed to write complete file");
        return;
    }
    
    printf("SUCCESS: Uploaded file: %s (%zu bytes)\n", filename, data_len);
    send_response(client_fd, 200, "OK", "text/plain", "Upload successful");
}

/**
 * Handle file list request
 */
static void handle_list(web_server_t* server, int client_fd) {
    int count = 0;
    char** files = portal_list_skylanders(&count);
    
    char json[4096] = "[";
    for (int i = 0; i < count; i++) {
        if (i > 0) strcat(json, ",");
        strcat(json, "\"");
        strcat(json, files[i]);
        strcat(json, "\"");
        free(files[i]);
    }
    strcat(json, "]");
    
    if (files) free(files);
    
    send_response(client_fd, 200, "OK", "application/json", json);
}

/**
 * Handle load skylander request
 */
static void handle_load(web_server_t* server, int client_fd, const char* query) {
    char* filename = get_query_param(query, "file");
    char* slot_str = get_query_param(query, "slot");
    
    if (!filename || !slot_str) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "Missing parameters");
        if (filename) free(filename);
        if (slot_str) free(slot_str);
        return;
    }
    
    int slot = atoi(slot_str);
    
    if (portal_load_skylander(server->portal, slot, filename) == 0) {
        send_response(client_fd, 200, "OK", "text/plain", "Loaded");
    } else {
        send_response(client_fd, 500, "Internal Server Error", "text/plain", "Load failed");
    }
    
    free(filename);
    free(slot_str);
}

/**
 * Handle delete file request
 */
static void handle_delete(web_server_t* server, int client_fd, const char* query) {
    char* filename = get_query_param(query, "file");
    
    if (!filename) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "Missing filename");
        return;
    }
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "./skylanders/%s", filename);
    
    if (unlink(filepath) == 0) {
        send_response(client_fd, 200, "OK", "text/plain", "Deleted");
    } else {
        send_response(client_fd, 500, "Internal Server Error", "text/plain", "Delete failed");
    }
    
    free(filename);
}

/**
 * Handle status request
 */
static void handle_status(web_server_t* server, int client_fd) {
    char json[4096];
    snprintf(json, sizeof(json), "{\"state\":\"active\",\"slots\":[");
    
    for (int i = 0; i < 2; i++) {
        if (i > 0) strcat(json, ",");
        
        skylander_slot_t* slot = portal_get_skylander(server->portal, i);
        if (slot && slot->active) {
            char slot_json[256];
            snprintf(slot_json, sizeof(slot_json), 
                "{\"active\":true,\"filename\":\"%s\"}", slot->filename);
            strcat(json, slot_json);
        } else {
            strcat(json, "{\"active\":false}");
        }
    }
    
    strcat(json, "]}");
    send_response(client_fd, 200, "OK", "application/json", json);
}

/**
 * Handle a client connection
 */
void web_server_handle_client(web_server_t* server, int client_fd) {
    char buffer[16384];
    ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[bytes] = '\0';
    
    // Parse request line
    char method[16], path[256], version[16];
    sscanf(buffer, "%s %s %s", method, path, version);
    
    printf("Request: %s %s\n", method, path);
    
    // Extract query string
    char* query = strchr(path, '?');
    if (query) {
        *query = '\0';
        query++;
    }
    
    // Route requests
    if (strcmp(path, "/") == 0) {
        send_response(client_fd, 200, "OK", "text/html", HTML_INDEX);
    }
    else if (strcmp(path, "/upload") == 0 && strcmp(method, "POST") == 0) {
        handle_upload(server, client_fd, buffer, total_bytes);
    }
    else if (strcmp(path, "/list") == 0) {
        handle_list(server, client_fd);
    }
    else if (strcmp(path, "/load") == 0 && strcmp(method, "POST") == 0) {
        handle_load(server, client_fd, query);
    }
    else if (strcmp(path, "/delete") == 0 && strcmp(method, "POST") == 0) {
        handle_delete(server, client_fd, query);
    }
    else if (strcmp(path, "/status") == 0) {
        handle_status(server, client_fd);
    }
    else {
        send_response(client_fd, 404, "Not Found", "text/plain", "Not found");
    }
    
    free(buffer);
    close(client_fd);
}

/**
 * Main server thread
 */
void* web_server_thread(void* arg) {
    web_server_t* server = (web_server_t*)arg;
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(server->socket_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_fd < 0) {
            if (errno == EINTR || !server->running) break;
            perror("Accept failed");
            continue;
        }
        
        web_server_handle_client(server, client_fd);
    }
    
    return NULL;
}

/**
 * Initialize the web server
 */
int web_server_init(web_server_t* server, portal_t* portal, int port) {
    if (!server || !portal) return -1;
    
    memset(server, 0, sizeof(web_server_t));
    server->portal = portal;
    server->port = port;
    server->socket_fd = -1;
    server->running = 0;
    
    // Create socket
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        perror("Failed to create socket");
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server->socket_fd);
        return -1;
    }
    
    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(server->socket_fd);
        return -1;
    }
    
    // Listen
    if (listen(server->socket_fd, WEB_SERVER_MAX_CONNECTIONS) < 0) {
        perror("Listen failed");
        close(server->socket_fd);
        return -1;
    }
    
    printf("Web server initialized on port %d\n", port);
    return 0;
}

/**
 * Start the web server
 */
int web_server_start(web_server_t* server) {
    if (!server || server->socket_fd < 0) return -1;
    
    server->running = 1;
    
    pthread_t thread;
    if (pthread_create(&thread, NULL, web_server_thread, server) != 0) {
        perror("Failed to create web server thread");
        server->running = 0;
        return -1;
    }
    
    pthread_detach(thread);
    
    printf("Web server started on http://0.0.0.0:%d\n", server->port);
    return 0;
}

/**
 * Stop the web server
 */
void web_server_stop(web_server_t* server) {
    if (!server) return;
    
    server->running = 0;
    
    if (server->socket_fd >= 0) {
        shutdown(server->socket_fd, SHUT_RDWR);
    }
    
    printf("Web server stopped\n");
}

/**
 * Cleanup web server resources
 */
void web_server_cleanup(web_server_t* server) {
    if (!server) return;
    
    web_server_stop(server);
    
    if (server->socket_fd >= 0) {
        close(server->socket_fd);
        server->socket_fd = -1;
    }
    
    printf("Web server cleaned up\n");
}

/**
 * Check if server is running
 */
int web_server_is_running(web_server_t* server) {
    return server && server->running;
}