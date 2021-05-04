# UltraOS
An operating system that doesn't try to be UNIX. Made completely from scratch with its own bootloader. 😊

Desktop
![alt-text](https://i.ibb.co/S7PTxxH/image.png)

Panic screen
![alt-text](https://i.ibb.co/wNCc5vC/image.png)

## Current Features
- Support for both i386 and AMD64
- Symmetric Multiprocessing
- ISTs for each core to protect against kernel stack corruptions
- A well optimized thread-safe memory manager with support for stack overflow detection, with most components covered by unit tests
- Support for symbolicated backtraces thanks to the kernel symbol file loaded by the bootloader
- Support for deadlock detection for all lock types
- Window manager
- WC cached framebuffer for fast rendering on real hardware
- Multicore RR preemptive O(1) scheduler
- PS/2 controller driver + keyboard & mouse
- AHCI driver with async request queue, as well as full controller initialization according
  to the specification. Confirmed to work on all emulators and many real computers.
- Own BIOS bootloader
- Fully modern C++17 kernel
- Almost everything in kernel/Common is fully covered by unit tests
- No third party code

### Tested on modern hardware :heavy_check_mark:

## Getting started

### Windows

Windows support is fully based on WSL, so you can't currently build the system without it.

- Install QEMU, add it into your `Path`
- Install WSL Ubuntu, or any other distro with apt
- Install nasm for your WSL
- run `run_qemu.bat`, you can choose to build for i386 or AMD64 by passing "32" or "64" respectively, the default build is AMD64

### Linux

Currently only works for distributions with the apt package manager (used for pulling the toolchain dependencies).

- Install QEMU
- Install nasm
- run `run_qemu.sh`, you can choose to build for i386 or AMD64 by passing "32" or "64" respectively, the default build is AMD64

### MacOS

Mostly experimental support, but should work by following the same steps as Linux.
(Expects to have brew available for pulling dependencies)

---

### Virtualization Software

Pretty much any virtualization software you could find is supported as long as it can virtualize x86.
The system is confirmed to correctly work on all popular virtual machines.

QEMU is supported out of the box with most optimal parameters by running the `run_qemu.{sh/bar}` script, which also recompiles/rebuilds the system
if necessary.

For other virtual machines you can simply grab the VMDK image from the `Images` folder, generated via either `Scripts/build_ultra.{sh/bat}` or `run_qemu.{sh/bar}`.

### Real Hardware
I've personally tested the system on multiple PCs as well as laptops and encountered no problems, so it will probably work on yours, too.
However, keep in mind that the system is still in very early development stage and as a result might not be super stable/able to handle edge cases/peculiar hardware, so run at your own risk.

- Build the system using `./Scripts/build_ultra.{sh/bat}`
- Find a free USB stick (the default image size is 64MB atm)
- Find some software to write raw data to usb, I personally use `SUSE ImageWriter`
- Write the `Ultra{32/64}HDD-flat.vmdk` to the stick
- Insert the USB stick into your computer and boot from it (you might have to enable legacy boot/disable secure boot)

If you encounter any bugs/crashes/weird behavior please let me know so I can fix it. :)

