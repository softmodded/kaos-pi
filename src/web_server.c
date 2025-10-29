#include "web_server.h"
#include "portal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>

#define WEB_SERVER_MAX_CONNECTIONS 10

// HTML content for the web interface
static const char* HTML_INDEX = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>KAOS-Pi Portal Control</title>\n"
"    <style>\n"
"        * {\n"
"            margin: 0;\n"
"            padding: 0;\n"
"            box-sizing: border-box;\n"
"        }\n"
"        \n"
"        body {\n"
"            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
"            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"
"            min-height: 100vh;\n"
"            padding: 20px;\n"
"        }\n"
"        \n"
"        .container {\n"
"            max-width: 800px;\n"
"            margin: 0 auto;\n"
"        }\n"
"        \n"
"        .header {\n"
"            text-align: center;\n"
"            color: white;\n"
"            margin-bottom: 30px;\n"
"        }\n"
"        \n"
"        .header h1 {\n"
"            font-size: 2.5em;\n"
"            margin-bottom: 10px;\n"
"            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);\n"
"        }\n"
"        \n"
"        .header p {\n"
"            font-size: 1.1em;\n"
"            opacity: 0.9;\n"
"        }\n"
"        \n"
"        .card {\n"
"            background: white;\n"
"            border-radius: 12px;\n"
"            padding: 25px;\n"
"            margin-bottom: 20px;\n"
"            box-shadow: 0 4px 6px rgba(0,0,0,0.1);\n"
"        }\n"
"        \n"
"        .card h2 {\n"
"            color: #333;\n"
"            margin-bottom: 15px;\n"
"            padding-bottom: 10px;\n"
"            border-bottom: 2px solid #667eea;\n"
"        }\n"
"        \n"
"        .upload-zone {\n"
"            border: 3px dashed #667eea;\n"
"            border-radius: 8px;\n"
"            padding: 40px;\n"
"            text-align: center;\n"
"            cursor: pointer;\n"
"            transition: all 0.3s;\n"
"            background: #f8f9ff;\n"
"        }\n"
"        \n"
"        .upload-zone:hover {\n"
"            border-color: #764ba2;\n"
"            background: #f0f1ff;\n"
"        }\n"
"        \n"
"        .upload-zone.drag-over {\n"
"            border-color: #764ba2;\n"
"            background: #e8e9ff;\n"
"        }\n"
"        \n"
"        input[type=\"file\"] {\n"
"            display: none;\n"
"        }\n"
"        \n"
"        .btn {\n"
"            padding: 10px 20px;\n"
"            border: none;\n"
"            border-radius: 6px;\n"
"            cursor: pointer;\n"
"            font-size: 14px;\n"
"            font-weight: 600;\n"
"            transition: all 0.3s;\n"
"        }\n"
"        \n"
"        .btn-load {\n"
"            background: #667eea;\n"
"            color: white;\n"
"            margin-right: 5px;\n"
"        }\n"
"        \n"
"        .btn-load:hover {\n"
"            background: #5568d3;\n"
"        }\n"
"        \n"
"        .btn-delete {\n"
"            background: #ef4444;\n"
"            color: white;\n"
"        }\n"
"        \n"
"        .btn-delete:hover {\n"
"            background: #dc2626;\n"
"        }\n"
"        \n"
"        .file-item {\n"
"            display: flex;\n"
"            justify-content: space-between;\n"
"            align-items: center;\n"
"            padding: 15px;\n"
"            margin: 10px 0;\n"
"            background: #f8f9fa;\n"
"            border-radius: 8px;\n"
"            transition: all 0.3s;\n"
"        }\n"
"        \n"
"        .file-item:hover {\n"
"            background: #e9ecef;\n"
"            transform: translateX(5px);\n"
"        }\n"
"        \n"
"        .file-name {\n"
"            font-weight: 500;\n"
"            color: #333;\n"
"        }\n"
"        \n"
"        .file-actions {\n"
"            display: flex;\n"
"            gap: 5px;\n"
"        }\n"
"        \n"
"        .slots {\n"
"            display: flex;\n"
"            gap: 20px;\n"
"            margin-top: 20px;\n"
"        }\n"
"        \n"
"        .slot {\n"
"            flex: 1;\n"
"            padding: 20px;\n"
"            background: #f8f9fa;\n"
"            border-radius: 8px;\n"
"            border: 3px solid #dee2e6;\n"
"            transition: all 0.3s;\n"
"        }\n"
"        \n"
"        .slot.active {\n"
"            background: #d4edda;\n"
"            border-color: #28a745;\n"
"        }\n"
"        \n"
"        .slot-label {\n"
"            font-weight: bold;\n"
"            color: #667eea;\n"
"            margin-bottom: 10px;\n"
"        }\n"
"        \n"
"        .slot-name {\n"
"            color: #666;\n"
"            font-size: 14px;\n"
"        }\n"
"        \n"
"        .status {\n"
"            padding: 10px;\n"
"            margin-top: 15px;\n"
"            border-radius: 6px;\n"
"            text-align: center;\n"
"            font-weight: 500;\n"
"            display: none;\n"
"        }\n"
"        \n"
"        .status.show {\n"
"            display: block;\n"
"        }\n"
"        \n"
"        .status.success {\n"
"            background: #d4edda;\n"
"            color: #155724;\n"
"        }\n"
"        \n"
"        .status.error {\n"
"            background: #f8d7da;\n"
"            color: #721c24;\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <div class=\"header\">\n"
"            <h1>🎮 KAOS-Pi Portal</h1>\n"
"            <p>Skylander Portal Emulator</p>\n"
"        </div>\n"
"        \n"
"        <div class=\"card\">\n"
"            <h2>Portal Status</h2>\n"
"            <div class=\"slots\" id=\"slotsContainer\">\n"
"                <div class=\"slot\">\n"
"                    <div class=\"slot-label\">Slot 1</div>\n"
"                    <div class=\"slot-name\">Empty</div>\n"
"                </div>\n"
"                <div class=\"slot\">\n"
"                    <div class=\"slot-label\">Slot 2</div>\n"
"                    <div class=\"slot-name\">Empty</div>\n"
"                </div>\n"
"            </div>\n"
"        </div>\n"
"        \n"
"        <div class=\"card\">\n"
"            <h2>Upload Skylander</h2>\n"
"            <div class=\"upload-zone\" id=\"uploadZone\">\n"
"                <input type=\"file\" id=\"fileInput\" accept=\".bin,.dmp,.dump,.sky\" multiple>\n"
"                <p style=\"font-size: 18px; color: #667eea; margin-bottom: 10px;\">📁 Drop files here or click to browse</p>\n"
"                <p style=\"color: #666; font-size: 14px;\">Supported: .bin, .dmp, .dump, .sky (1024 bytes)</p>\n"
"            </div>\n"
"            <div id=\"statusMessage\" class=\"status\"></div>\n"
"        </div>\n"
"        \n"
"        <div class=\"card\">\n"
"            <h2>Available Skylanders</h2>\n"
"            <div id=\"fileListContainer\">\n"
"                <p style=\"color: #999; padding: 20px; text-align: center;\">Loading...</p>\n"
"            </div>\n"
"        </div>\n"
"    </div>\n"
"    \n"
"    <script>\n"
"        const uploadZone = document.getElementById('uploadZone');\n"
"        const fileInput = document.getElementById('fileInput');\n"
"        \n"
"        uploadZone.addEventListener('click', () => fileInput.click());\n"
"        \n"
"        uploadZone.addEventListener('dragover', (e) => {\n"
"            e.preventDefault();\n"
"            uploadZone.classList.add('drag-over');\n"
"        });\n"
"        \n"
"        uploadZone.addEventListener('dragleave', () => {\n"
"            uploadZone.classList.remove('drag-over');\n"
"        });\n"
"        \n"
"        uploadZone.addEventListener('drop', (e) => {\n"
"            e.preventDefault();\n"
"            uploadZone.classList.remove('drag-over');\n"
"            handleFiles(e.dataTransfer.files);\n"
"        });\n"
"        \n"
"        fileInput.addEventListener('change', (e) => {\n"
"            handleFiles(e.target.files);\n"
"        });\n"
"        \n"
"        function handleFiles(files) {\n"
"            for (let file of files) {\n"
"                uploadFile(file);\n"
"            }\n"
"        }\n"
"        \n"
"        function showStatus(message, isError = false) {\n"
"            const status = document.getElementById('statusMessage');\n"
"            status.textContent = message;\n"
"            status.className = 'status show ' + (isError ? 'error' : 'success');\n"
"            setTimeout(() => status.classList.remove('show'), 3000);\n"
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
    *dst = '\0';
}

/**
 * Get query parameter value
 */
static char* get_query_param(const char* query, const char* param) {
    if (!query || !param) return NULL;
    
    char search[256];
    snprintf(search, sizeof(search), "%s=", param);
    
    const char* pos = strstr(query, search);
    if (!pos) return NULL;
    
    pos += strlen(search);
    const char* end = strchr(pos, '&');
    size_t len = end ? (size_t)(end - pos) : strlen(pos);
    
    char* value = malloc(len + 1);
    if (!value) return NULL;
    
    strncpy(value, pos, len);
    value[len] = '\0';
    
    char* decoded = malloc(len + 1);
    if (!decoded) {
        free(value);
        return NULL;
    }
    
    url_decode(decoded, value);
    free(value);
    
    return decoded;
}

/**
 * Send HTTP response
 */
static void send_response(int client_fd, int status_code, const char* status_text,
                         const char* content_type, const char* body) {
    char header[2048];
    size_t body_len = body ? strlen(body) : 0;
    
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        status_code, status_text, content_type, body_len);
    
    write(client_fd, header, strlen(header));
    if (body) {
        write(client_fd, body, body_len);
    }
}

/**
 * Handle file upload
 */
static void handle_upload(web_server_t* server, int client_fd, const char* headers, size_t total_len) {
    printf("Upload request received, total length: %zu\n", total_len);
    
    // Find Content-Type header to get boundary
    const char* content_type = strstr(headers, "Content-Type:");
    if (!content_type) {
        printf("ERROR: No Content-Type header found\n");
        send_response(client_fd, 400, "Bad Request", "text/plain", "No Content-Type");
        return;
    }
    
    const char* boundary_start = strstr(content_type, "boundary=");
    if (!boundary_start) {
        printf("ERROR: No boundary found in Content-Type\n");
        send_response(client_fd, 400, "Bad Request", "text/plain", "No boundary in Content-Type");
        return;
    }
    
    // Extract boundary (skip "boundary=")
    boundary_start += 9;
    char boundary[128];
    size_t i = 0;
    while (i < sizeof(boundary) - 1 && boundary_start[i] && 
           boundary_start[i] != '\r' && boundary_start[i] != '\n' && 
           boundary_start[i] != ';' && boundary_start[i] != ' ') {
        boundary[i] = boundary_start[i];
        i++;
    }
    boundary[i] = '\0';
    printf("Boundary: [%s]\n", boundary);
    
    // Find the body (after headers)
    const char* body = strstr(headers, "\r\n\r\n");
    if (!body) {
        printf("ERROR: No body separator found\n");
        send_response(client_fd, 400, "Bad Request", "text/plain", "No body separator");
        return;
    }
    body += 4;
    
    // Find filename in body
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
    printf("Filename: [%s]\n", filename);
    
    // Find file data (after Content-Type line and blank line in the part)
    const char* data_start = strstr(filename_end, "\r\n\r\n");
    if (!data_start) {
        printf("ERROR: No data separator found after filename\n");
        send_response(client_fd, 400, "Bad Request", "text/plain", "No data separator");
        return;
    }
    data_start += 4;
    
    // Find end boundary - try multiple formats
    const char* data_end = NULL;
    char boundary_end[256];
    
    // Try format: \r\n--boundary--
    snprintf(boundary_end, sizeof(boundary_end), "\r\n--%s--", boundary);
    data_end = strstr(data_start, boundary_end);
    
    // Try format: \r\n--boundary
    if (!data_end) {
        snprintf(boundary_end, sizeof(boundary_end), "\r\n--%s", boundary);
        data_end = strstr(data_start, boundary_end);
    }
    
    // Try format: --boundary (without \r\n)
    if (!data_end) {
        snprintf(boundary_end, sizeof(boundary_end), "--%s", boundary);
        data_end = strstr(data_start, boundary_end);
    }
    
    if (!data_end) {
        printf("ERROR: No end boundary found\n");
        printf("Looking for boundary: [%s]\n", boundary);
        printf("Data starts at offset: %ld\n", data_start - headers);
        printf("First 200 chars of data:\n%.200s\n", data_start);
        printf("Last 200 chars of request:\n%.200s\n", headers + total_len - 200);
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
 * Handle Skylander load request
 */
static void handle_load(web_server_t* server, int client_fd, const char* query) {
    char* filename = get_query_param(query, "file");
    char* slot_str = get_query_param(query, "slot");
    
    if (!filename || !slot_str) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "Missing parameters");
        free(filename);
        free(slot_str);
        return;
    }
    
    int slot = atoi(slot_str);
    free(slot_str);
    
    if (slot < 0 || slot >= 2) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "Invalid slot");
        free(filename);
        return;
    }
    
    if (portal_load_skylander(server->portal, slot, filename) == 0) {
        send_response(client_fd, 200, "OK", "text/plain", "Loaded successfully");
    } else {
        send_response(client_fd, 500, "Internal Server Error", "text/plain", "Load failed");
    }
    
    free(filename);
}

/**
 * Handle file delete request
 */
static void handle_delete(web_server_t* server, int client_fd, const char* query) {
    char* filename = get_query_param(query, "file");
    
    if (!filename) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "Missing filename");
        return;
    }
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "/var/lib/kaos-pi/skylanders/%s", filename);
    
    if (unlink(filepath) == 0) {
        send_response(client_fd, 200, "OK", "text/plain", "Deleted successfully");
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
    char* buffer = malloc(65536);  // 64KB buffer for file uploads
    if (!buffer) {
        close(client_fd);
        return;
    }
    
    ssize_t bytes = 0;
    ssize_t total_bytes = 0;
    
    // Read initial chunk to get headers
    bytes = read(client_fd, buffer, 4096);
    if (bytes <= 0) {
        free(buffer);
        close(client_fd);
        return;
    }
    total_bytes = bytes;
    buffer[total_bytes] = '\0';
    
    // Parse request line
    char method[16], path[256], version[16];
    sscanf(buffer, "%s %s %s", method, path, version);
    
    printf("Request: %s %s\n", method, path);
    
    // For POST requests, check Content-Length and read full body
    if (strcmp(method, "POST") == 0) {
        const char* content_length_str = strstr(buffer, "Content-Length:");
        if (content_length_str) {
            int content_length = 0;
            sscanf(content_length_str, "Content-Length: %d", &content_length);
            
            // Find end of headers
            const char* body_start = strstr(buffer, "\r\n\r\n");
            if (body_start) {
                body_start += 4;
                int headers_len = body_start - buffer;
                int body_received = total_bytes - headers_len;
                int body_remaining = content_length - body_received;
                
                // Read remaining body data
                while (body_remaining > 0 && total_bytes < 65535) {
                    bytes = read(client_fd, buffer + total_bytes, 
                                body_remaining < (65536 - total_bytes - 1) ? body_remaining : (65536 - total_bytes - 1));
                    if (bytes <= 0) break;
                    total_bytes += bytes;
                    body_remaining -= bytes;
                }
                buffer[total_bytes] = '\0';
            }
        }
    }
    
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
    
    printf("Web server started on port %d\n", server->port);
    return 0;
}

/**
 * Stop the web server
 */
void web_server_stop(web_server_t* server) {
    if (!server) return;
    
    server->running = 0;
    
    if (server->socket_fd >= 0) {
        close(server->socket_fd);
        server->socket_fd = -1;
    }
    
    printf("Web server stopped\n");
}