#!/bin/sh
set -e

which curl >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "curl not found" >&2
    echo "Please install the package 'curl'" >&2
    echo "sudo pacman -Sy curl" >&2
    exit 1
fi

which sha256sum >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "sha256sum not found" >&2
    echo "Please install the package 'coreutils'" >&2
    echo "sudo pacman -Sy coreutils" >&2
    exit 1
fi

which openssl >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "openssl not found" >&2
    echo "Please install the package 'openssl'" >&2
    echo "sudo pacman -Sy openssl" >&2
    exit 1
fi

which mkisofs >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "mkisofs not found" >&2
    echo "Please install the package 'cdrtools'" >&2
    echo "sudo pacman -Sy cdrtools" >&2
    exit 1
fi

which qemu-img >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "qemu-img not found" >&2
    echo "Please install the package 'qemu-img'" >&2
    echo "sudo pacman -Sy qemu-img" >&2
    exit 1
fi

which virt-customize >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "virt-customize not found" >&2
    echo "Please install the package 'guestfs-tools'" >&2
    echo "sudo pacman -Sy guestfs-tools" >&2
    exit 1
fi

which qemu-system-x86_64 >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "qemu-system-x86_64 not found" >&2
    echo "Please install the package 'qemu-full'" >&2
    echo "sudo pacman -Sy qemu-full" >&2
    exit 1
fi

which awk >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "awk not found" >&2
    echo "Please install the package 'awk'" >&2
    echo "sudo pacman -Sy awk" >&2
    exit 1
fi

which sed >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "sed not found" >&2
    echo "Please install the package 'sed'" >&2
    echo "sudo pacman -Sy sed" >&2
    exit 1
fi

USER="victim"
PASSWD="victim"

CONF_DIR="$(pwd)/conf"
BUILD_DIR="$(pwd)/build"
MODULE_DIR="$(pwd)/../../rootkit"

UBUNTU_IMG="$BUILD_DIR/$1"
CONFIG_ISO="$BUILD_DIR/config.iso"

UBUNTU_IMG_URL="https://cloud-images.ubuntu.com/focal/current/$(basename "$UBUNTU_IMG")"
UBUNTU_CHECKSUM_URL="https://cloud-images.ubuntu.com/focal/current/SHA256SUMS"

mkdir -p "$BUILD_DIR"

if [ ! -f "$UBUNTU_IMG" ];
then
    echo "Downloading Ubuntu Focal..."
    curl -L --progress-bar -o "$UBUNTU_IMG" "$UBUNTU_IMG_URL"

    echo "Verifying checksum..."
    EXPECTED_CHKSUM=$(curl -sL "$UBUNTU_CHECKSUM_URL" | grep "$(basename "$UBUNTU_IMG")" | awk '{print $1}')
    CHKSUM=$(sha256sum "$UBUNTU_IMG" | awk '{print $1}')
    if [ "$CHKSUM" != "$EXPECTED_CHKSUM" ]; then
        echo "Checksum mismatch! Deleting corrupted image."
        rm -f "$UBUNTU_IMG"
        exit 1
    fi
    echo "Checksum OK."

    qemu-img resize "$UBUNTU_IMG" 20G

    virt-customize -a "$UBUNTU_IMG" \
        --run-command 'echo "XKBLAYOUT=fr" >> /etc/default/keyboard' \
        --run-command "mkdir -p /home/$USER" \
        --run-command "sudo apt install -y gcc make" \
        --copy-in "$MODULE_DIR:/home/$USER/"
fi

if [ ! -f "$CONFIG_ISO" ];
then
    cp "$CONF_DIR/meta-data.yml" "$BUILD_DIR/meta-data"
    cp "$CONF_DIR/user-data.yml" "$BUILD_DIR/user-data"
    cp "$CONF_DIR/network-config.yml" "$BUILD_DIR/network-config"

    sed -i "s|UBUNTU_USER|$USER|g" "$BUILD_DIR/user-data"
    sed -i "s|UBUNTU_PASSWD|$(openssl passwd -6 "$PASSWD")|" "$BUILD_DIR/user-data"

    mkisofs \
        -o "$CONFIG_ISO" \
        -volid cidata \
        -joliet -rock \
        "$BUILD_DIR/user-data" "$BUILD_DIR/meta-data" "$BUILD_DIR/network-config"
fi

qemu-system-x86_64 \
    -enable-kvm \
    -m 2048 -smp 2 -cpu host \
    -netdev socket,id=net0,mcast=230.0.0.1:1234,localaddr=127.0.0.1 \
    -device virtio-net-pci,netdev=net0,mac=52:54:00:00:00:02 \
    -drive file="$UBUNTU_IMG",format=qcow2 \
    -drive file="$CONFIG_ISO",format=raw &

echo "====== Victim Credentials ======"
echo "  - User:     $USER"
echo "  - Password: $PASSWD"
