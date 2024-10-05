#!/usr/bin/env bash

## Mounts the disk image and copies the boot file into it ##
set -xe

diskfile=${1:-'boot.img'}
bootfile=${2:-'boot.bin'}
mountfile=${3:-'updboot'}

mkdir $mountfile
mount $diskfile $mountfile
cp $bootfile $mountfile/BOOT
umount $diskfile
rm -r $mountfile