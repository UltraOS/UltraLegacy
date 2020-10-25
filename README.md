# UltraOS
An operating system that doesn't try to be UNIX. Made completely from scratch with its own bootloader. 😊

![alt-text](https://i.ibb.co/Q8HWWLF/new-desktop.png)

## Current Features
- Support for both i386 and AMD64
- Symmetric Multiprocessing
- PS/2 controller driver + keyboard & mouse
- Window manager
- WC cached framebuffer for fast rendering on real hardware
- Multicore RR preemptive scheduler
- Own BIOS bootloader
- Fully modern C++17 kernel
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
- Install QEMU
- Install nasm
- run `run_qemu.sh`, you can choose to build for i386 or AMD64 by passing "32" or "64" respectively, the default build is AMD64

Virtualization software other than QEMU (like VMware, VirtualBox) is also usable by manually loading the VMDK image from `Images` (generated via `Scripts/build_ultra.{sh/bat}`).

### Real Hardware
I've personally tested the system on multiple PCs as well as laptops and encountered no problems, so it will probably work on yours, too.
However, keep in mind that the system is still in very early development stage and as a result might not be super stable/able to handle edge cases/peculiar hardware, so run at your own risk.

- Build the system using `./Scripts/build_ultra.{sh/bat}`
- Find a free USB stick (the default image size is 64MB atm)
- Find some software to write raw data to usb, I personally use `SUSE ImageWriter`
- Write the `Ultra{32/64}HDD-flat.vmdk` to the stick
- Insert the USB stick into your computer and boot from it (you might have to enable legacy boot/disable secure boot)

If you encounter any bugs/crashes/weird behavior please let me know so I can fix it. :)

