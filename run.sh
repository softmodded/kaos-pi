#!/bin/bash

###############################################################################
# KAOS-Pi Development Run Script
# Quick script to build and run KAOS-Pi for testing
###############################################################################

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_msg() {
    echo -e "${BLUE}[KAOS-Pi]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    print_error "Please run as root (use sudo)"
    exit 1
fi

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Run this script from the project root."
    exit 1
fi

# Parse command line arguments
BUILD_ONLY=0
CLEAN=0
PORT=8080

while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            BUILD_ONLY=1
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --build-only    Only build, don't run"
            echo "  --clean         Clean build directory before building"
            echo "  -p, --port      Web server port (default: 8080)"
            echo "  -h, --help      Show this help message"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ $CLEAN -eq 1 ]; then
    print_msg "Cleaning build directory..."
    rm -rf build
    print_success "Clean complete"
fi

# Create build directory
mkdir -p build

# Build
print_msg "Building KAOS-Pi..."
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
cd ..

if [ $? -eq 0 ]; then
    print_success "Build successful"
else
    print_error "Build failed"
    exit 1
fi

# Exit if build-only
if [ $BUILD_ONLY -eq 1 ]; then
    print_msg "Build-only mode, exiting"
    exit 0
fi

# Check for USB gadget support
print_msg "Checking USB gadget support..."

if ! lsmod | grep -q dwc2; then
    print_msg "Loading dwc2 module..."
    modprobe dwc2 || print_error "Failed to load dwc2 module"
fi

if ! lsmod | grep -q libcomposite; then
    print_msg "Loading libcomposite module..."
    modprobe libcomposite || print_error "Failed to load libcomposite module"
fi

if ! mountpoint -q /sys/kernel/config; then
    print_msg "Mounting configfs..."
    mount -t configfs none /sys/kernel/config || print_error "Failed to mount configfs"
fi

# Create skylanders directory if it doesn't exist
mkdir -p skylanders
chmod 777 skylanders

# Stop any running instance
if systemctl is-active --quiet kaos-pi; then
    print_msg "Stopping systemd service..."
    systemctl stop kaos-pi
fi

# Remove any existing USB gadget
if [ -d "/sys/kernel/config/usb_gadget/skylander" ]; then
    print_msg "Cleaning up existing USB gadget..."

    # Unbind UDC if bound
    if [ -f "/sys/kernel/config/usb_gadget/skylander/UDC" ]; then
        echo "" > /sys/kernel/config/usb_gadget/skylander/UDC 2>/dev/null || true
    fi

    # Wait a bit
    sleep 1

    # Remove symlink
    rm -f /sys/kernel/config/usb_gadget/skylander/configs/c.1/hid.usb0 2>/dev/null || true

    # Remove directories
    rmdir /sys/kernel/config/usb_gadget/skylander/configs/c.1/strings/0x409 2>/dev/null || true
    rmdir /sys/kernel/config/usb_gadget/skylander/configs/c.1 2>/dev/null || true
    rmdir /sys/kernel/config/usb_gadget/skylander/functions/hid.usb0 2>/dev/null || true
    rmdir /sys/kernel/config/usb_gadget/skylander/strings/0x409 2>/dev/null || true
    rmdir /sys/kernel/config/usb_gadget/skylander 2>/dev/null || true
fi

# Run the application
print_msg "Starting KAOS-Pi..."
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                    Starting KAOS-Pi                            ║"
echo "║              Press Ctrl+C to stop                              ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

exec ./build/kaos-pi -p $PORT
