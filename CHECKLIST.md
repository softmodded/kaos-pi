# ✅ KAOS-Pi Installation Checklist

## Pre-Installation Checklist

Before you begin, make sure you have:

- [ ] Raspberry Pi Zero W or Zero 2 W
- [ ] MicroSD card (8GB minimum, 16GB+ recommended)
- [ ] Raspberry Pi OS installed and updated
- [ ] SSH access or keyboard/monitor connected
- [ ] Internet connection working
- [ ] USB cable for connecting to console/PC
- [ ] Power supply for the Pi (5V, 2A recommended)

## Installation Steps

### Phase 1: Preparation

- [ ] SSH into your Raspberry Pi
- [ ] Update system packages:
  ```bash
  sudo apt-get update
  sudo apt-get upgrade -y
  ```
- [ ] Clone the repository:
  ```bash
  cd ~
  git clone https://github.com/yourusername/kaos-pi.git
  cd kaos-pi
  ```

### Phase 2: Installation

- [ ] Make install script executable:
  ```bash
  chmod +x install.sh
  ```
- [ ] Run the installer:
  ```bash
  sudo ./install.sh
  ```
- [ ] Wait for installation to complete (5-10 minutes)
- [ ] Verify no errors occurred during installation

### Phase 3: Configuration

- [ ] **REBOOT YOUR PI** (this is required!):
  ```bash
  sudo reboot
  ```
- [ ] After reboot, start the service:
  ```bash
  sudo systemctl start kaos-pi
  ```
- [ ] Enable auto-start on boot:
  ```bash
  sudo systemctl enable kaos-pi
  ```

### Phase 4: Verification

- [ ] Check service status:
  ```bash
  sudo systemctl status kaos-pi
  ```
  - Should show: `Active: active (running)`

- [ ] Verify USB gadget device exists:
  ```bash
  ls -l /dev/hidg0
  ```
  - Should show a character device

- [ ] Check modules are loaded:
  ```bash
  lsmod | grep dwc2
  lsmod | grep libcomposite
  ```
  - Both should appear in the output

- [ ] Test web interface:
  - Find your Pi's IP: `hostname -I`
  - Open browser to: `http://YOUR_IP:8080`
  - You should see the KAOS-Pi web interface

## Hardware Connection Checklist

### Raspberry Pi Zero W Port Identification

- [ ] Locate the **TWO** micro USB ports on your Pi Zero W:
  - **PWR** port (power only) - Usually labeled "PWR IN"
  - **USB** port (data + power) - Usually labeled "USB"

- [ ] Connect power supply to **PWR** port
- [ ] Connect console/PC to **USB** port (NOT the PWR port!)

### Correct Connection Diagram
```
Raspberry Pi Zero W
┌─────────────────┐
│  [PWR]  ←───────┼─── Power Supply (5V 2A)
│                 │
│  [USB]  ←───────┼─── Console/PC (USB Cable)
└─────────────────┘
```

## Testing Checklist

### Web Interface Tests

- [ ] Can access web interface at `http://YOUR_IP:8080`
- [ ] Can upload a Skylander file (.bin, .dmp, .dump, or .sky)
- [ ] Uploaded file appears in the file list
- [ ] Can click "Load Slot 1" to load a Skylander
- [ ] Portal status shows loaded Skylander
- [ ] Can delete a file

### USB Portal Tests

- [ ] Plug Pi into game console or PC
- [ ] Console/PC detects a Skylander Portal
- [ ] No error messages on console/PC
- [ ] Portal responds to game commands
- [ ] Loaded Skylanders appear in-game

### Service Tests

- [ ] View live logs:
  ```bash
  sudo journalctl -u kaos-pi -f
  ```
- [ ] Logs show no errors
- [ ] Can stop service: `sudo systemctl stop kaos-pi`
- [ ] Can restart service: `sudo systemctl restart kaos-pi`
- [ ] Service auto-starts after reboot

## Troubleshooting Checklist

If something doesn't work, check these:

### Portal Not Detected

- [ ] Did you reboot after installation?
- [ ] Are you using the USB port (not PWR)?
- [ ] Is the service running? `sudo systemctl status kaos-pi`
- [ ] Does /dev/hidg0 exist? `ls -l /dev/hidg0`
- [ ] Are modules loaded? `lsmod | grep dwc2`

### Web Interface Not Working

- [ ] Is the service running?
- [ ] Can you ping the Pi? `ping raspberrypi.local`
- [ ] Is port 8080 listening? `sudo netstat -tlnp | grep 8080`
- [ ] Try using IP address instead of hostname
- [ ] Check firewall settings

### File Upload Fails

- [ ] Is file exactly 1024 bytes?
- [ ] Does file have valid extension (.bin, .dmp, .dump, .sky)?
- [ ] Check directory permissions:
  ```bash
  sudo chmod 777 /var/lib/kaos-pi/skylanders
  ```

### Permission Errors

- [ ] Running as root? Use `sudo`
- [ ] Service configured to run as root?
- [ ] ConfigFS mounted? `mount | grep configfs`

## Post-Installation Checklist

After everything is working:

- [ ] Bookmark the web interface URL
- [ ] Upload your Skylander collection
- [ ] Test with your favorite game
- [ ] Configure port forwarding (if accessing remotely)
- [ ] Set up backups of your Skylander files
- [ ] Join the Skylanders community!

## Optional Enhancements

- [ ] Change web server port (edit service file)
- [ ] Set up HTTPS (for security)
- [ ] Configure firewall rules
- [ ] Set up VPN for remote access
- [ ] Create backups of configuration

## Quick Reference Commands

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

# Get IP address
hostname -I

# Check USB device
ls -l /dev/hidg0
```

## Success Criteria

Your installation is successful when:

✅ Service shows as "active (running)"
✅ Web interface is accessible
✅ You can upload Skylander files
✅ Portal is detected by console/PC
✅ Skylanders work in-game
✅ No errors in logs

## Getting Help

If you're stuck:

1. Check the [README.md](README.md) for detailed documentation
2. Review the [Troubleshooting section](README.md#-troubleshooting)
3. Check logs: `sudo journalctl -u kaos-pi -f`
4. Open an issue on GitHub with:
   - Your Pi model
   - OS version (`cat /etc/os-release`)
   - Error messages
   - What you've already tried

---

**Congratulations on setting up KAOS-Pi! Happy gaming! 🎮**