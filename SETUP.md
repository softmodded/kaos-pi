# 🚀 KAOS-Pi Quick Setup Guide

**Get your Skylander Portal running in 15 minutes!**

## Prerequisites

✅ Raspberry Pi Zero W (or Zero 2 W)  
✅ MicroSD card with Raspberry Pi OS installed  
✅ SSH or keyboard/monitor access to your Pi  
✅ Internet connection  

## Step-by-Step Installation

### 1. Download KAOS-Pi

SSH into your Raspberry Pi and clone the repository:

```bash
cd ~
git clone https://github.com/yourusername/kaos-pi.git
cd kaos-pi
```

### 2. Run the Installer

Execute the installation script:

```bash
chmod +x install.sh
sudo ./install.sh
```

This will take 5-10 minutes and will:
- ✓ Install build tools and dependencies
- ✓ Configure USB gadget mode (dwc2, libcomposite)
- ✓ Build the KAOS-Pi application
- ✓ Install to system directories
- ✓ Create a systemd service

### 3. Reboot (REQUIRED!)

USB gadget mode only works after a reboot:

```bash
sudo reboot
```

⚠️ **Don't skip this step!** The Pi must reboot for USB gadget to work.

### 4. Start KAOS-Pi

After the Pi reboots, start the service:

```bash
sudo systemctl start kaos-pi
```

Enable auto-start on boot:

```bash
sudo systemctl enable kaos-pi
```

### 5. Access the Web Interface

Find your Pi's IP address:

```bash
hostname -I
```

Then open your browser to:

```
http://YOUR_PI_IP:8080
```

For example: `http://192.168.1.100:8080`

### 6. Upload Skylander Files

1. Open the web interface
2. Drag and drop your `.bin`, `.dmp`, `.dump`, or `.sky` files
3. Click "Load Slot 1" or "Load Slot 2" to place a Skylander on the portal

### 7. Connect to Your Console/PC

**IMPORTANT:** Use the **USB port** (not the PWR port) on your Pi Zero W!

```
┌─────────────┐
│ Pi Zero W   │
│  [PWR] ○    │ ← Power supply here
│             │
│  [USB] ●    │ ← Console/PC connects here
└─────────────┘
```

Plug the USB cable into your game console or PC. The portal should be detected automatically!

## 🎮 Testing

### Check if it's running:

```bash
sudo systemctl status kaos-pi
```

Should show: `Active: active (running)`

### View live logs:

```bash
sudo journalctl -u kaos-pi -f
```

You should see messages about USB gadget initialization and web server starting.

### Check USB gadget:

```bash
ls -l /dev/hidg0
```

Should show: `crw-rw---- 1 root root ...`

## 🔧 Troubleshooting

### Problem: Portal not detected by console/PC

**Solution:**
1. Verify you rebooted after installation
2. Check that you're using the USB port (not PWR)
3. Verify modules are loaded:
   ```bash
   lsmod | grep dwc2
   lsmod | grep libcomposite
   ```
4. Check logs:
   ```bash
   sudo journalctl -u kaos-pi -f
   ```

### Problem: Web interface not accessible

**Solution:**
1. Check service status: `sudo systemctl status kaos-pi`
2. Verify port 8080 is listening: `sudo netstat -tlnp | grep 8080`
3. Try localhost: `http://localhost:8080`
4. Check firewall settings

### Problem: Permission denied errors

**Solution:**
Always run with sudo: `sudo kaos-pi`

The systemd service runs as root automatically.

### Problem: File upload fails

**Solution:**
1. Check file size (must be exactly 1024 bytes)
2. Verify file extension (`.bin`, `.dmp`, `.dump`, `.sky`)
3. Fix permissions:
   ```bash
   sudo chmod 777 /var/lib/kaos-pi/skylanders
   ```

## 📋 Quick Commands Reference

```bash
# Start service
sudo systemctl start kaos-pi

# Stop service
sudo systemctl stop kaos-pi

# Restart service
sudo systemctl restart kaos-pi

# Check status
sudo systemctl status kaos-pi

# View logs
sudo journalctl -u kaos-pi -f

# Enable on boot
sudo systemctl enable kaos-pi

# Disable on boot
sudo systemctl disable kaos-pi

# Run manually
sudo kaos-pi

# Run with custom port
sudo kaos-pi -p 80
```

## 📁 File Locations

| Item | Location |
|------|----------|
| Binary | `/usr/local/bin/kaos-pi` |
| Skylander Files | `/var/lib/kaos-pi/skylanders/` |
| Service File | `/etc/systemd/system/kaos-pi.service` |
| Config (boot) | `/boot/config.txt` or `/boot/firmware/config.txt` |

## 🎉 Success!

If you can:
- ✅ Access the web interface
- ✅ Upload Skylander files
- ✅ See the portal detected by your console/PC

**You're all set! Enjoy your KAOS-Pi Portal!**

---

## Next Steps

- Add more Skylander dumps to your collection
- Configure auto-start on boot
- Set up remote access to manage from anywhere
- Join the Skylanders community!

## Need More Help?

- 📖 Read the full [README.md](README.md)
- 🐛 Check [Troubleshooting section](README.md#-troubleshooting)
- 💬 Open an issue on GitHub
- 📝 Review logs: `sudo journalctl -u kaos-pi -f`

---

**Happy Gaming! 🎮**