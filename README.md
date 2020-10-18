# UltraOS
An operating system that doesn't try to be UNIX. Made completely from scratch with its own bootloader. 😊

![alt-text](https://i.ibb.co/2dx8Shv/Untitled.png)

## Current Features
- Support for both i386 and AMD64
- Symmetric Multiprocessing
- PS/2 controller driver + keyboard & mouse
- Window manager
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
- run `run_qemu.bat`, you can choose to build for i386 or AMD64 by passing "32" or "64" respectively, the default build is AMD64.

### Linux
- Install QEMU
- Install nasm
- run `run_qemu.sh`, you can choose to build for i386 or AMD64 by passing "32" or "64" respectively, the default build is AMD64.

Virtualization software other than QEMU (like VMware, VirtualBox) is also usable by manually loading the VMDK image from `Images` (generated via `Scripts/build_ultra.{sh/bat}`).
