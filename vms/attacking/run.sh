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

which ssh-keygen >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ssh-keygen not found" >&2
    echo "Please install the package 'openssh'" >&2
    echo "sudo pacman -Sy openssh" >&2
    exit 1
fi

USER="operator"

CONF_DIR="$(pwd)/conf"
BUILD_DIR="$(pwd)/build"
ATTACKING_PROGRAM_DIR="$(pwd)/../../attacking_program"

SSH_PORT="2222"
SSH_KEY="$BUILD_DIR/id_ed25519"
SSH_CONF="$BUILD_DIR/ssh.conf"

ROCKY_IMG="$BUILD_DIR/$1"
CONFIG_ISO="$BUILD_DIR/config.iso"

ROCKY_IMG_URL="https://dl.rockylinux.org/pub/rocky/10/images/x86_64/$(basename "$ROCKY_IMG")"
ROCKY_CHECKSUM_URL="https://dl.rockylinux.org/pub/rocky/10/images/x86_64/CHECKSUM"

mkdir -p "$BUILD_DIR"

if [ ! -f "$SSH_KEY" ]; then
    echo "Generating SSH key pair..."
    ssh-keygen -t ed25519 -f "$SSH_KEY" -N "" -C "wlkom-attacking"
fi
SSH_PUBKEY="$(cat "$SSH_KEY.pub")"

if [ ! -f "$ROCKY_IMG" ];
then
    echo "Downloading Rocky Linux 10..."
    curl -L --progress-bar -o "$ROCKY_IMG" "$ROCKY_IMG_URL"

    echo "Verifying checksum..."
    EXPECTED_CHKSUM=$(curl -sL "$ROCKY_CHECKSUM_URL" | grep "SHA256 ($(basename "$ROCKY_IMG"))" | awk '{print $NF}')
    CHKSUM=$(sha256sum "$ROCKY_IMG" | awk '{print $1}')
    if [ "$CHKSUM" != "$EXPECTED_CHKSUM" ]; then
        echo "Checksum mismatch! Deleting corrupted image."
        rm -f "$ROCKY_IMG"
        exit 1
    fi
    echo "Checksum OK."

    qemu-img resize "$ROCKY_IMG" 20G

    virt-customize -a "$ROCKY_IMG" \
        --run-command "mkdir -p /home/$USER" \
        --run-command "sudo dnf install -y python3-pip make"
fi


if [ ! -f "$CONFIG_ISO" ];
then
    cp "$CONF_DIR/meta-data.yml" "$BUILD_DIR/meta-data"
    cp "$CONF_DIR/user-data.yml" "$BUILD_DIR/user-data"
    cp "$CONF_DIR/network-config.yml" "$BUILD_DIR/network-config"

    sed -i "s|ROCKY_USER|$USER|g" "$BUILD_DIR/user-data"
    sed -i "s|ROCKY_SSH_PUBKEY|$SSH_PUBKEY|" "$BUILD_DIR/user-data"

    mkisofs \
        -o "$CONFIG_ISO" \
        -volid cidata \
        -joliet -rock \
        "$BUILD_DIR/user-data" "$BUILD_DIR/meta-data" "$BUILD_DIR/network-config"
fi

if [ ! -f "$SSH_CONF" ];
then
    cp "$CONF_DIR/ssh.conf" "$SSH_CONF"

    sed -i "s|SSH_PORT|$SSH_PORT|" "$SSH_CONF"
    sed -i "s|SSH_USER|$USER|" "$SSH_CONF"
    sed -i "s|SSH_KEY|$SSH_KEY|" "$SSH_CONF"
fi

virt-customize -a "$ROCKY_IMG" \
    --copy-in "$ATTACKING_PROGRAM_DIR:/home/$USER/" \
    --run-command "make -C /home/$USER/$(basename "$ATTACKING_PROGRAM_DIR") glob-install"

qemu-system-x86_64 \
    -enable-kvm \
    -m 2048 -smp 2 -cpu host \
    -netdev socket,id=net0,mcast=230.0.0.1:1234,localaddr=127.0.0.1 \
    -device virtio-net-pci,netdev=net0,mac=52:54:00:00:00:01 \
    -netdev user,id=net1,hostfwd=tcp::${SSH_PORT}-:22 \
    -device virtio-net-pci,netdev=net1 \
    -drive file="$ROCKY_IMG",format=qcow2,if=virtio \
    -drive file="$CONFIG_ISO",format=raw,if=virtio,readonly=on &

echo "====== Attacking SSH ======"
echo "  ssh -F $SSH_CONF attacking"
echo "  # or:"
echo "  ssh -i $SSH_KEY -p $SSH_PORT $USER@127.0.0.1"
