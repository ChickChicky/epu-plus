# EPU-PLUS

An upgrade to my previous virtual custom architectures that this time is entirely virtual. I also stepped up on the virtualization to now be able to create and read FAT16 'floppy disks'.

## Quick start

Set up the environment with:

```sh
$ chmod +x *.sh # Enables the execution of the scripts
$ ./build.sh    # Build the emulator
$ ./gendisk.sh  # Creates a disk image
```

After this, you can set the boot file like so:

```sh
$ mkdir boot-floppy                   # Mounts the disk
$ sudo mount boot.img boot-floppy     # |
$ sudo cp BOOT_FILE boot-floppy/boot  # Copies the boot file into the disk
$ sudo unmount boot.img               # Unmounts the disk
$ rm boot-floppy                      # |
```

## Errors

* When the system boots, the first kind of error that can occur is with an orange spiral filling the screen up. In that case, it is a significant JS-side error and you should report to the console for more information.
* A blue floppy disk icon means that the boot disk image could not be found, make sure it is available in the same directory as the page.
* A red floppy disk icon means that the boot disk image is not the expected size (1M).
* A yellow floppy disk icon means that the *boot* file could not be found at the root of the file system or that it itself is corrupted.