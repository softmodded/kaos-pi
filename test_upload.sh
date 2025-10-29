#!/bin/bash

# Test script for uploading a Skylander file to the KAOS-Pi web server

PORT=8080
HOST="localhost"

# Check if a file was provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <skylander_file.bin>"
    echo ""
    echo "Example:"
    echo "  $0 test_skylander.bin"
    exit 1
fi

FILENAME="$1"

# Check if file exists
if [ ! -f "$FILENAME" ]; then
    echo "Error: File '$FILENAME' not found"
    exit 1
fi

# Check file size (should be 1024 bytes for Skylanders)
FILESIZE=$(stat -f%z "$FILENAME" 2>/dev/null || stat -c%s "$FILENAME" 2>/dev/null)
if [ "$FILESIZE" -ne 1024 ]; then
    echo "Warning: File size is $FILESIZE bytes (expected 1024 bytes)"
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo "Uploading $FILENAME ($FILESIZE bytes) to $HOST:$PORT..."

# Upload using curl
RESPONSE=$(curl -v -X POST \
    -F "file=@$FILENAME" \
    "http://$HOST:$PORT/upload" \
    2>&1)

# Check result
if echo "$RESPONSE" | grep -q "200 OK"; then
    echo "✓ Upload successful!"

    # Test listing files
    echo ""
    echo "Files on server:"
    curl -s "http://$HOST:$PORT/list" | python3 -m json.tool 2>/dev/null || curl -s "http://$HOST:$PORT/list"
else
    echo "✗ Upload failed"
    echo ""
    echo "Response:"
    echo "$RESPONSE"
fi
