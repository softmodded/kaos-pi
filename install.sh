#!/bin/bash

###############################################################################
# KAOS-Pi Installation Script
# Skylander Portal Emulator for Raspberry Pi Zero W
###############################################################################

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored message
print_msg() {
    echo -e "${BLUE}[KAOS-Pi]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "Please run as root (use sudo)"
        exit 1
    fi
}

# Check if running on Raspberry Pi
check_platform() {
    if [ ! -f /proc/device-tree/model ]; then
        print_warning "Cannot detect Raspberry Pi model"
        return
    fi

    MODEL=$(cat /proc/device-tree/model)
    print_msg "Detected: $MODEL"

    if [[ ! "$MODEL" =~ "Raspberry Pi" ]]; then
        print_warning "This script is designed for Raspberry Pi"
        read -p "Continue anyway? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
}

# Install dependencies
install_dependencies() {
    print_msg "Installing dependencies..."

    apt-get update
    apt-get install -y \
        build-essential \
        cmake \
        git \
        libc6-dev \
        gcc \
        g++ \
        make

    print_success "Dependencies installed"
}

# Enable USB gadget mode
enable_usb_gadget() {
    print_msg "Configuring USB gadget mode..."

    # Enable dwc2 overlay in /boot/config.txt
    CONFIG_FILE="/boot/config.txt"
    FIRMWARE_CONFIG="/boot/firmware/config.txt"

    # Check which config file exists
    if [ -f "$FIRMWARE_CONFIG" ]; then
        CONFIG_FILE="$FIRMWARE_CONFIG"
    fi

    if ! grep -q "^dtoverlay=dwc2" "$CONFIG_FILE"; then
        echo "dtoverlay=dwc2" >> "$CONFIG_FILE"
        print_success "Added dtoverlay=dwc2 to $CONFIG_FILE"
    else
        print_msg "dtoverlay=dwc2 already configured"
    fi

    # Enable dwc2 and libcomposite modules
    MODULES_FILE="/etc/modules"

    if ! grep -q "^dwc2" "$MODULES_FILE"; then
        echo "dwc2" >> "$MODULES_FILE"
        print_success "Added dwc2 to $MODULES_FILE"
    fi

    if ! grep -q "^libcomposite" "$MODULES_FILE"; then
        echo "libcomposite" >> "$MODULES_FILE"
        print_success "Added libcomposite to $MODULES_FILE"
    fi

    # Load modules now (if not already loaded)
    modprobe dwc2 2>/dev/null || true
    modprobe libcomposite 2>/dev/null || true

    # Mount configfs if not mounted
    if ! mountpoint -q /sys/kernel/config; then
        mount -t configfs none /sys/kernel/config 2>/dev/null || true
    fi

    # Add to /etc/fstab if not present
    if ! grep -q "configfs" /etc/fstab; then
        echo "configfs /sys/kernel/config configfs defaults 0 0" >> /etc/fstab
        print_success "Added configfs to /etc/fstab"
    fi

    print_success "USB gadget mode configured"
}

# Build the application
build_application() {
    print_msg "Building KAOS-Pi..."

    # Create build directory
    mkdir -p build
    cd build

    # Run CMake
    cmake .. -DCMAKE_BUILD_TYPE=Release

    # Build
    make -j$(nproc)

    cd ..

    print_success "Build complete"
}

# Install the application
install_application() {
    print_msg "Installing KAOS-Pi..."

    # Install binary
    install -m 755 build/kaos-pi /usr/local/bin/kaos-pi

    # Create data directory
    mkdir -p /var/lib/kaos-pi/skylanders
    chmod 755 /var/lib/kaos-pi
    chmod 777 /var/lib/kaos-pi/skylanders

    # Create local skylanders directory
    mkdir -p ./skylanders
    chmod 777 ./skylanders

    print_success "KAOS-Pi installed to /usr/local/bin/kaos-pi"
}

# Create systemd service
create_service() {
    print_msg "Creating systemd service..."

    cat > /etc/systemd/system/kaos-pi.service << 'EOF'
[Unit]
Description=KAOS-Pi Skylander Portal Emulator
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/var/lib/kaos-pi
ExecStart=/usr/local/bin/kaos-pi
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
EOF

    # Reload systemd
    systemctl daemon-reload

    print_success "Systemd service created"
}

# Print completion message
print_completion() {
    echo
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║              KAOS-Pi Installation Complete!                   ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo
    print_success "Installation successful!"
    echo
    print_msg "Next steps:"
    echo
    echo "  1. Reboot your Raspberry Pi:"
    echo "     ${YELLOW}sudo reboot${NC}"
    echo
    echo "  2. After reboot, start KAOS-Pi:"
    echo "     ${YELLOW}sudo systemctl start kaos-pi${NC}"
    echo
    echo "  3. Enable auto-start on boot:"
    echo "     ${YELLOW}sudo systemctl enable kaos-pi${NC}"
    echo
    echo "  4. Check status:"
    echo "     ${YELLOW}sudo systemctl status kaos-pi${NC}"
    echo
    echo "  5. View logs:"
    echo "     ${YELLOW}sudo journalctl -u kaos-pi -f${NC}"
    echo
    echo "  6. Access web interface:"
    echo "     ${YELLOW}http://$(hostname -I | awk '{print $1}'):8080${NC}"
    echo
    print_warning "IMPORTANT: You MUST reboot for USB gadget mode to work!"
    echo
}

# Main installation flow
main() {
    echo
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║              KAOS-Pi Installation Script                      ║"
    echo "║         Skylander Portal Emulator for Raspberry Pi            ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo

    check_root
    check_platform

    print_msg "Starting installation..."
    echo

    install_dependencies
    enable_usb_gadget
    build_application
    install_application
    create_service

    print_completion
}

# Run main
main
