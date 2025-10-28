#!/bin/bash

# Skylander Portal USB Gadget Setup Script
# This script configures a Raspberry Pi to act as a USB gadget (Skylander Portal)

set -e

echo "========================================="
echo "Skylander Portal USB Gadget Setup"
echo "========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run as root"
    echo "Please run: sudo $0"
    exit 1
fi

# Detect Raspberry Pi model
echo "[1/6] Detecting Raspberry Pi model..."
PI_MODEL=$(cat /proc/device-tree/model 2>/dev/null || echo "Unknown")
echo "Detected: $PI_MODEL"

# Check if Pi Zero or Pi 4 (USB OTG capable)
if [[ ! "$PI_MODEL" =~ "Pi Zero" ]] && [[ ! "$PI_MODEL" =~ "Pi 4" ]] && [[ ! "$PI_MODEL" =~ "Raspberry Pi Zero" ]]; then
    echo "Warning: USB gadget mode works best on Pi Zero, Pi Zero W, or Pi 4"
    echo "Other models may not support USB gadget mode properly"
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Enable dwc2 overlay in /boot/config.txt
echo ""
echo "[2/6] Configuring /boot/config.txt..."
BOOT_CONFIG="/boot/config.txt"
if [ ! -f "$BOOT_CONFIG" ]; then
    BOOT_CONFIG="/boot/firmware/config.txt"
fi

if [ ! -f "$BOOT_CONFIG" ]; then
    echo "Error: Cannot find boot config file"
    echo "Tried: /boot/config.txt and /boot/firmware/config.txt"
    exit 1
fi

if grep -q "^dtoverlay=dwc2" "$BOOT_CONFIG"; then
    echo "dtoverlay=dwc2 already enabled"
else
    echo "Adding dtoverlay=dwc2 to $BOOT_CONFIG"
    echo "dtoverlay=dwc2" >> "$BOOT_CONFIG"
fi

# Enable kernel modules in /etc/modules
echo ""
echo "[3/6] Configuring kernel modules..."
if grep -q "^dwc2" /etc/modules; then
    echo "dwc2 module already configured"
else
    echo "Adding dwc2 to /etc/modules"
    echo "dwc2" >> /etc/modules
fi

if grep -q "^libcomposite" /etc/modules; then
    echo "libcomposite module already configured"
else
    echo "Adding libcomposite to /etc/modules"
    echo "libcomposite" >> /etc/modules
fi

# Mount configfs
echo ""
echo "[4/6] Setting up configfs..."
if mountpoint -q /sys/kernel/config; then
    echo "configfs already mounted"
else
    echo "Mounting configfs..."
    mount -t configfs none /sys/kernel/config
fi

# Add to /etc/fstab if not present
if grep -q "configfs" /etc/fstab; then
    echo "configfs already in /etc/fstab"
else
    echo "Adding configfs to /etc/fstab for automatic mounting"
    echo "none /sys/kernel/config configfs defaults 0 0" >> /etc/fstab
fi

# Load kernel modules now (without reboot)
echo ""
echo "[5/6] Loading kernel modules..."
if lsmod | grep -q dwc2; then
    echo "dwc2 module already loaded"
else
    echo "Loading dwc2 module..."
    modprobe dwc2 || echo "Warning: Could not load dwc2 module (may require reboot)"
fi

if lsmod | grep -q libcomposite; then
    echo "libcomposite module already loaded"
else
    echo "Loading libcomposite module..."
    modprobe libcomposite || echo "Warning: Could not load libcomposite module (may require reboot)"
fi

# Check for UDC
echo ""
echo "[6/6] Checking for USB Device Controller (UDC)..."
if [ -d "/sys/class/udc" ]; then
    UDC_COUNT=$(ls -1 /sys/class/udc 2>/dev/null | wc -l)
    if [ "$UDC_COUNT" -gt 0 ]; then
        echo "Found UDC(s):"
        ls -1 /sys/class/udc
        echo "✓ USB gadget support is available!"
    else
        echo "⚠ No UDC found yet"
        echo "This may require a reboot to take effect"
    fi
else
    echo "⚠ /sys/class/udc not found"
    echo "This may require a reboot to take effect"
fi

# Create data directory
echo ""
echo "Creating data directory..."
DATA_DIR="/var/lib/kaos-pi/skylanders"
mkdir -p "$DATA_DIR"
chmod 755 "$DATA_DIR"
echo "Created: $DATA_DIR"

# Summary
echo ""
echo "========================================="
echo "Setup Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "1. REBOOT your Raspberry Pi for all changes to take effect:"
echo "   sudo reboot"
echo ""
echo "2. After reboot, verify the setup:"
echo "   ls /sys/class/udc"
echo "   (Should show a UDC device)"
echo ""
echo "3. Run the Skylander Portal software:"
echo "   sudo ./kaos-pi"
echo ""
echo "Notes:"
echo "- Make sure to connect the Pi via the USB OTG port"
echo "  (On Pi Zero: the micro USB port labeled 'USB', not 'PWR')"
echo "  (On Pi 4: the USB-C port can be used with dt_overlay=dwc2,dr_mode=peripheral)"
echo "- The software MUST run as root (sudo) to access USB gadget"
echo "- Place Skylander .bin files in: $DATA_DIR"
echo ""
