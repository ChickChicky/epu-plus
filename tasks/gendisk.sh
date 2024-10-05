#!/usr/bin/env bash

## Generates a blank FAT16 floppy disk ##
set -xe

# Creates a FAT16 disk image

IMAGE_FILE=boot.img

if [ -f $IMAGE_FILE ]; then
    echo -e "\x1b[31mFile already exists, delete it if you want to create a new image\x1b[39m"
    exit 1
fi

# Creates a disk with 65536 sectors of 512 bytes
dd if=/dev/zero count=65536 bs=512 of=$IMAGE_FILE
(echo o; echo n; echo p; echo 1; echo 2048; echo 65535; echo t; echo 6; echo w; echo q) | fdisk $IMAGE_FILE
mkfs.fat -F 16 $IMAGE_FILE

# Resize the disk to 2048 sectors
truncate -s 1048576 $IMAGE_FILE
echo "000020: 00 00 08 00" | xxd -r - $IMAGE_FILE