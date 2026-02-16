#!/bin/bash

# Setup script for v4l2loopback virtual camera
# This script helps set up a virtual camera device for use with Zoom/video calls

echo "=== Face Swapper - Virtual Camera Setup ==="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "This script needs sudo privileges to load kernel modules."
    echo "Please run: sudo $0"
    exit 1
fi

# Check if v4l2loopback module is available
if ! modinfo v4l2loopback > /dev/null 2>&1; then
    echo "Error: v4l2loopback module not found."
    echo ""
    echo "Please install v4l2loopback:"
    echo "  Ubuntu/Debian: sudo apt-get install v4l2loopback-dkms"
    echo "  Arch Linux: sudo pacman -S v4l2loopback-dkms"
    echo "  Fedora: sudo dnf install v4l2loopback"
    exit 1
fi

# Check if module is already loaded
if lsmod | grep -q v4l2loopback; then
    echo "v4l2loopback is already loaded."
    echo "To reload, first remove it: sudo modprobe -r v4l2loopback"
    echo ""
    read -p "Do you want to reload it? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        modprobe -r v4l2loopback
    else
        echo "Keeping existing module."
        exit 0
    fi
fi

# Load v4l2loopback module
echo "Loading v4l2loopback module..."
modprobe v4l2loopback devices=1 video_nr=2 card_label="FaceSwapper" exclusive_caps=1

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Virtual camera device created successfully!"
    echo ""
    echo "Device location: /dev/video2"
    echo ""
    echo "To make this persistent across reboots, add this line to /etc/modules-load.d/v4l2loopback.conf:"
    echo "  v4l2loopback"
    echo ""
    echo "And create /etc/modprobe.d/v4l2loopback.conf with:"
    echo "  options v4l2loopback devices=1 video_nr=2 card_label=\"FaceSwapper\" exclusive_caps=1"
    echo ""
    echo "You can now run the face swapper and select /dev/video2 as your camera in Zoom!"
else
    echo "Error: Failed to load v4l2loopback module."
    exit 1
fi
