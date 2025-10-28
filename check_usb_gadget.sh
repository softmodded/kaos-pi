#!/bin/bash

# USB Gadget Diagnostic Script
# Checks if the Raspberry Pi is properly configured for USB gadget mode

echo "========================================="
echo "USB Gadget Diagnostic Tool"
echo "========================================="
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓${NC} $2"
    else
        echo -e "${RED}✗${NC} $2"
    fi
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Check 1: Raspberry Pi Model
echo "[1] Checking Raspberry Pi model..."
if [ -f /proc/device-tree/model ]; then
    MODEL=$(cat /proc/device-tree/model)
    echo "    Model: $MODEL"
    if [[ "$MODEL" =~ "Pi Zero" ]] || [[ "$MODEL" =~ "Pi 4" ]]; then
        print_status 0 "Model supports USB gadget mode"
    else
        print_warning "Model may not support USB gadget mode"
        echo "    Best results on: Pi Zero, Pi Zero W, Pi Zero 2 W, or Pi 4"
    fi
else
    print_status 1 "Cannot detect Pi model"
fi
echo ""

# Check 2: Boot configuration
echo "[2] Checking /boot/config.txt..."
BOOT_CONFIG="/boot/config.txt"
if [ ! -f "$BOOT_CONFIG" ]; then
    BOOT_CONFIG="/boot/firmware/config.txt"
fi

if [ -f "$BOOT_CONFIG" ]; then
    if grep -q "^dtoverlay=dwc2" "$BOOT_CONFIG"; then
        print_status 0 "dtoverlay=dwc2 is enabled"
    else
        print_status 1 "dtoverlay=dwc2 NOT found in $BOOT_CONFIG"
        echo "    Run: echo 'dtoverlay=dwc2' | sudo tee -a $BOOT_CONFIG"
    fi
else
    print_status 1 "Cannot find boot config file"
fi
echo ""

# Check 3: Kernel modules configuration
echo "[3] Checking /etc/modules..."
if grep -q "^dwc2" /etc/modules; then
    print_status 0 "dwc2 configured in /etc/modules"
else
    print_status 1 "dwc2 NOT in /etc/modules"
    echo "    Run: echo 'dwc2' | sudo tee -a /etc/modules"
fi

if grep -q "^libcomposite" /etc/modules; then
    print_status 0 "libcomposite configured in /etc/modules"
else
    print_status 1 "libcomposite NOT in /etc/modules"
    echo "    Run: echo 'libcomposite' | sudo tee -a /etc/modules"
fi
echo ""

# Check 4: Loaded kernel modules
echo "[4] Checking loaded kernel modules..."
if lsmod | grep -q dwc2; then
    print_status 0 "dwc2 module is loaded"
    lsmod | grep dwc2
else
    print_status 1 "dwc2 module NOT loaded"
    echo "    Run: sudo modprobe dwc2"
fi

if lsmod | grep -q libcomposite; then
    print_status 0 "libcomposite module is loaded"
else
    print_status 1 "libcomposite module NOT loaded"
    echo "    Run: sudo modprobe libcomposite"
fi
echo ""

# Check 5: ConfigFS
echo "[5] Checking configfs..."
if [ -d /sys/kernel/config ]; then
    print_status 0 "/sys/kernel/config exists"

    if mountpoint -q /sys/kernel/config; then
        print_status 0 "configfs is mounted"
    else
        print_status 1 "configfs is NOT mounted"
        echo "    Run: sudo mount -t configfs none /sys/kernel/config"
    fi

    if [ -d /sys/kernel/config/usb_gadget ]; then
        print_status 0 "/sys/kernel/config/usb_gadget exists"
    else
        print_status 1 "/sys/kernel/config/usb_gadget NOT found"
    fi
else
    print_status 1 "/sys/kernel/config does NOT exist"
fi
echo ""

# Check 6: UDC (USB Device Controller)
echo "[6] Checking for USB Device Controller (UDC)..."
if [ -d /sys/class/udc ]; then
    UDC_COUNT=$(ls -1 /sys/class/udc 2>/dev/null | wc -l)
    if [ "$UDC_COUNT" -gt 0 ]; then
        print_status 0 "Found $UDC_COUNT UDC(s):"
        for udc in /sys/class/udc/*; do
            echo "    - $(basename $udc)"
        done
    else
        print_status 1 "No UDC devices found"
        echo "    The dwc2 driver may not be working properly"
        echo "    Try rebooting after running setup_usb_gadget.sh"
    fi
else
    print_status 1 "/sys/class/udc does NOT exist"
    echo "    USB gadget support is not available"
fi
echo ""

# Check 7: Existing USB gadgets
echo "[7] Checking for existing USB gadgets..."
if [ -d /sys/kernel/config/usb_gadget ]; then
    GADGET_COUNT=$(ls -1 /sys/kernel/config/usb_gadget 2>/dev/null | wc -l)
    if [ "$GADGET_COUNT" -gt 0 ]; then
        print_warning "Found $GADGET_COUNT existing gadget(s):"
        for gadget in /sys/kernel/config/usb_gadget/*; do
            gadget_name=$(basename $gadget)
            echo "    - $gadget_name"
            if [ -f "$gadget/UDC" ]; then
                udc_val=$(cat "$gadget/UDC" 2>/dev/null)
                if [ -n "$udc_val" ]; then
                    echo "      Status: ACTIVE (bound to $udc_val)"
                else
                    echo "      Status: inactive"
                fi
            fi
        done
        echo "    Note: Only one gadget can be active at a time"
    else
        print_status 0 "No existing gadgets (clean state)"
    fi
fi
echo ""

# Check 8: HID gadget device
echo "[8] Checking for HID gadget device..."
if [ -e /dev/hidg0 ]; then
    print_status 0 "/dev/hidg0 exists"
    ls -l /dev/hidg0
else
    print_status 1 "/dev/hidg0 does NOT exist"
    echo "    This will be created when the gadget is started"
fi
echo ""

# Check 9: Permissions
echo "[9] Checking permissions..."
if [ "$EUID" -eq 0 ]; then
    print_status 0 "Running as root (required for USB gadget)"
else
    print_warning "NOT running as root"
    echo "    USB gadget operations require root privileges"
    echo "    Run with: sudo $0"
fi
echo ""

# Check 10: Data directory
echo "[10] Checking data directory..."
DATA_DIR="/var/lib/kaos-pi/skylanders"
if [ -d "$DATA_DIR" ]; then
    print_status 0 "Data directory exists: $DATA_DIR"
    SKY_COUNT=$(ls -1 "$DATA_DIR"/*.bin 2>/dev/null | wc -l)
    if [ "$SKY_COUNT" -gt 0 ]; then
        echo "    Found $SKY_COUNT Skylander file(s)"
    else
        print_warning "No .bin files found in $DATA_DIR"
        echo "    Place Skylander dumps here"
    fi
else
    print_status 1 "Data directory does NOT exist: $DATA_DIR"
    echo "    Run: sudo mkdir -p $DATA_DIR"
fi
echo ""

# Summary
echo "========================================="
echo "Summary"
echo "========================================="
echo ""

READY=true

if ! lsmod | grep -q dwc2; then
    READY=false
    echo "❌ dwc2 module not loaded"
fi

if ! lsmod | grep -q libcomposite; then
    READY=false
    echo "❌ libcomposite module not loaded"
fi

if [ ! -d /sys/class/udc ] || [ $(ls -1 /sys/class/udc 2>/dev/null | wc -l) -eq 0 ]; then
    READY=false
    echo "❌ No UDC available"
fi

if ! mountpoint -q /sys/kernel/config; then
    READY=false
    echo "❌ configfs not mounted"
fi

if $READY; then
    echo -e "${GREEN}✓ System appears ready for USB gadget mode!${NC}"
    echo ""
    echo "To start the Skylander Portal:"
    echo "  sudo ./kaos-pi"
else
    echo -e "${RED}✗ System is NOT ready for USB gadget mode${NC}"
    echo ""
    echo "Run the setup script to configure:"
    echo "  sudo ./setup_usb_gadget.sh"
    echo ""
    echo "Then reboot:"
    echo "  sudo reboot"
fi
echo ""
