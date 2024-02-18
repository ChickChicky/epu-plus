#!/usr/bin/env bash

# Creates a fat16 bootable disk image

IMAGE_FILE=boot.img

if [ -f $IMAGE_FILE ]; then
    echo -e "\x1b[31mFile already exists, delete it if you want to create a new image\x1b[39m"
    exit 1
fi  

dd if=/dev/zero count=65536 bs=1024 of=$IMAGE_FILE
(echo o; echo n; echo p; echo 1; echo 2048; echo 99999; echo t; echo 6; echo w; echo q) | fdisk $IMAGE_FILE
mkfs.fat -F 16 $IMAGE_FILE