# EPU-PLUS

An upgrade to my previous virtual custom architectures that this time is entirely virtual. I also stepped up on the virtualization to now be able to create and read FAT16 'floppy disks'.

## Quick start

Set up the environment with:

```sh
$ chmod +x tasks/*.sh # Enables the execution of the scripts
$ tasks/build.sh    # Build the emulator
$ tasks/gendisk.sh  # Creates a disk image
```

There is a WIP assembler, that comes with a 'drawing' program

```sh
$ npx tsx src/assembler/assembler.ts src/assembler/draw.asm boot.bin # Builds the program
$ sudo tasks/updboot.sh                                              # Saves it to the boot disk
```

## Errors

* When the system boots, the first kind of error that can occur is with an orange spiral filling the screen up. In that case, it is a significant JS-side error and you should report to the console for more information.
* A blue floppy disk icon means that the boot disk image could not be found, make sure it is available in the same directory as the page.
* A red floppy disk icon means that the boot disk image is not the expected size (1M).
* A yellow floppy disk icon means that the *boot* file could not be found at the root of the file system or that it itself is corrupted.