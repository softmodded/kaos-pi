# 🎮 KAOS-Pi

**Skylander Portal Emulator for Raspberry Pi Zero W**

A complete remake of the [KAOS](https://github.com/NicoAICP/KAOS) project, ported to run on Raspberry Pi Zero W under Raspberry Pi OS. This version adds a modern web interface for easy Skylander file management and portal control.

![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Zero%20W-red)
![Language](https://img.shields.io/badge/language-C-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## ✨ Features

- 🎮 **Full Skylander Portal Emulation** - Emulates an official Activision Skylander Portal over USB
- 🌐 **Web Interface** - Upload and manage Skylander files through your browser
- 📁 **Multiple Format Support** - Compatible with `.bin`, `.dmp`, `.dump`, and `.sky` file formats
- 🔌 **USB OTG** - Acts as a USB HID device, compatible with game consoles and PCs
- 💾 **Persistent Storage** - Skylander data is saved automatically
- 🚀 **Systemd Integration** - Can run as a service on boot
- 🔒 **Encryption Support** - Handles Skylander encryption/decryption
- 📱 **Responsive Design** - Web interface works on mobile devices

## 📋 Requirements

### Hardware
- **Raspberry Pi Zero W** (or Zero 2 W)
- MicroSD card (8GB or larger recommended)
- USB cable with data support (for connecting to console/PC)
- Power supply for the Pi

### Software
- Raspberry Pi OS (Bullseye or newer)
- Root access (sudo)

## 🚀 Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/kaos-pi.git
cd kaos-pi
```

### 2. Run Installation Script

```bash
chmod +x install.sh
sudo ./install.sh
```

The installation script will:
- Install required dependencies (gcc, cmake, etc.)
- Configure USB gadget mode (dwc2, libcomposite)
- Build the application
- Install to `/usr/local/bin/kaos-pi`
- Create a systemd service
- Set up the data directory

### 3. Reboot

**IMPORTANT:** USB gadget mode requires a reboot to take effect.

```bash
sudo reboot
```

### 4. Start the Service

After reboot, start KAOS-Pi:

```bash
sudo systemctl start kaos-pi
```

Enable auto-start on boot:

```bash
sudo systemctl enable kaos-pi
```

### 5. Access Web Interface

Open your browser and navigate to:

```
http://raspberrypi.local:8080
```

Or use your Pi's IP address:

```
http://192.168.1.xxx:8080
```

## 📖 Usage

### Web Interface

The web interface provides an easy way to manage your Skylander files:

1. **Upload Files** - Drag and drop `.bin`, `.dmp`, `.dump`, or `.sky` files
2. **Load Skylanders** - Click "Load Slot 1" or "Load Slot 2" to place a Skylander on the portal
3. **View Status** - See which Skylanders are currently loaded
4. **Delete Files** - Remove unwanted Skylander files

### Command Line

You can also run KAOS-Pi manually:

```bash
sudo kaos-pi
```

Options:
- `-p PORT` - Set web server port (default: 8080)
- `-h` - Show help message

Example:
```bash
sudo kaos-pi -p 80
```

### Systemd Service Commands

```bash
# Start the service
sudo systemctl start kaos-pi

# Stop the service
sudo systemctl stop kaos-pi

# Restart the service
sudo systemctl restart kaos-pi

# Check status
sudo systemctl status kaos-pi

# View logs
sudo journalctl -u kaos-pi -f

# Enable on boot
sudo systemctl enable kaos-pi

# Disable on boot
sudo systemctl disable kaos-pi
```

## 🔌 Hardware Connection

### Raspberry Pi Zero W USB OTG

The Raspberry Pi Zero W has **two** micro USB ports:

1. **PWR IN** (labeled "PWR") - For power only
2. **USB** (labeled "USB") - For data/OTG

**IMPORTANT:** Connect your console/PC to the **USB port** (not PWR IN)!

```
Pi Zero W          Game Console / PC
┌─────────┐       ┌──────────┐
│  PWR ○  │◄─────►│  Power   │ (Power supply)
│         │       └──────────┘
│  USB ●  │◄─────►┐           │
└─────────┘       │  Console  │ (USB for portal)
                  └───────────┘
```

## 📁 File Formats

KAOS-Pi supports the following Skylander dump formats:

- **`.bin`** - Raw binary dump (1024 bytes)
- **`.dmp`** - Standard dump format
- **`.dump`** - Alternative dump format
- **`.sky`** - RPCS3 emulator format

All files must be exactly **1024 bytes** (1KB) in size.

## 🛠️ Manual Building

If you prefer to build manually:

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake git

# Create build directory
mkdir build
cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install
sudo make install
```

## 🐛 Troubleshooting

### USB Gadget Not Detected

**Problem:** The portal is not detected when plugged into a console/PC.

**Solutions:**
1. Make sure you rebooted after installation
2. Check that dwc2 and libcomposite modules are loaded:
   ```bash
   lsmod | grep dwc2
   lsmod | grep libcomposite
   ```
3. Verify configfs is mounted:
   ```bash
   mount | grep configfs
   ```
4. Check if `/dev/hidg0` exists:
   ```bash
   ls -l /dev/hidg0
   ```
5. Use the **USB port** (not PWR port) on Pi Zero W

### Web Interface Not Accessible

**Problem:** Cannot access web interface at port 8080.

**Solutions:**
1. Check if KAOS-Pi is running:
   ```bash
   sudo systemctl status kaos-pi
   ```
2. Check if port 8080 is listening:
   ```bash
   sudo netstat -tlnp | grep 8080
   ```
3. Try accessing via IP address instead of hostname
4. Check firewall rules (if any)

### "Permission Denied" Errors

**Problem:** Permission errors when running.

**Solutions:**
1. Always run with `sudo`:
   ```bash
   sudo kaos-pi
   ```
2. The service runs as root automatically
3. USB gadget requires root privileges

### Portal Commands Not Working

**Problem:** Portal connects but doesn't respond to commands.

**Solutions:**
1. Check logs for errors:
   ```bash
   sudo journalctl -u kaos-pi -f
   ```
2. Verify Skylander files are properly loaded
3. Try unloading and reloading Skylanders

### Files Not Uploading

**Problem:** Cannot upload files through web interface.

**Solutions:**
1. Check file size (must be exactly 1024 bytes)
2. Verify file extension is valid (`.bin`, `.dmp`, `.dump`, `.sky`)
3. Check permissions on `skylanders/` directory:
   ```bash
   sudo chmod 777 /var/lib/kaos-pi/skylanders
   ```

## 🔧 Configuration

### Change Web Server Port

Edit `/etc/systemd/system/kaos-pi.service`:

```ini
ExecStart=/usr/local/bin/kaos-pi -p 80
```

Then reload and restart:

```bash
sudo systemctl daemon-reload
sudo systemctl restart kaos-pi
```

### Change Skylanders Directory

By default, Skylanders are stored in `/var/lib/kaos-pi/skylanders/`.

To change this, modify the `SKYLANDERS_DIR` constant in `src/portal.c` and rebuild.

## 📊 Technical Details

### Architecture

```
┌─────────────────────────────────────────┐
│           Web Browser (Port 8080)       │
└─────────────────┬───────────────────────┘
                  │ HTTP
┌─────────────────▼───────────────────────┐
│         Web Server (web_server.c)       │
│  - File Upload                          │
│  - File Management                      │
│  - Status API                           │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│         Portal Logic (portal.c)         │
│  - Skylander Management                 │
│  - Command Processing                   │
│  - Encryption/Decryption                │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│      USB Gadget (usb_gadget.c)          │
│  - HID Device Emulation                 │
│  - ConfigFS Setup                       │
│  - USB Communication                    │
└─────────────────┬───────────────────────┘
                  │ USB OTG
┌─────────────────▼───────────────────────┐
│        Game Console / PC                │
│  - Skylanders Game                      │
│  - Detects as Official Portal           │
└─────────────────────────────────────────┘
```

### USB Descriptor

KAOS-Pi emulates the official Activision Skylander Portal with:

- **Vendor ID:** `0x1430` (Activision)
- **Product ID:** `0x0150` (Skylander Portal)
- **Device Class:** HID (Human Interface Device)
- **Endpoint:** Interrupt IN/OUT

### Protocol

The portal uses a custom HID protocol:

- **Read Block:** `Q <slot> <block>` - Read 16 bytes from block
- **Write Block:** `W <slot> <block> [data]` - Write 16 bytes to block
- **Status:** `S` - Get portal status (active slots)
- **Activate:** `A` - Activate portal
- **Color:** `C <R> <G> <B> <slot>` - Set LED color

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📜 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- Original [KAOS](https://github.com/NicoAICP/KAOS) project by NicoAICP
- Skylander community for reverse engineering work
- Raspberry Pi Foundation for the amazing hardware

## ⚠️ Disclaimer

This project is for educational purposes only. Skylanders and Portal of Power are trademarks of Activision. This project is not affiliated with or endorsed by Activision.

## 📞 Support

If you encounter issues:

1. Check the [Troubleshooting](#-troubleshooting) section
2. Review logs: `sudo journalctl -u kaos-pi -f`
3. Open an issue on GitHub

---

**Made with ❤️ for the Skylanders community**