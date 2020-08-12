# UltraOS
An operating system that doesn't try to be UNIX. Made completely from scratch with its own bootloader. 😊

## Current Features
- Fully modern C++17 kernel
- Absolutely no third party code
- Support for both x86 and x86-64
- Own bootloader for both x86 and x86-64
- SMP support
- Basic multicore RR preemptive scheduler

### Tested on modern hardware :heavy_check_mark:

## Getting started

### Windows

Windows support is fully based on WSL, so you can't currently build the system without it.

- Install QEMU, add it into your `Path`
- Install WSL Ubuntu, or any other distro with apt
- Install nasm for your WSL
- run `run_qemu.bat`, you can choose to build for x86 or x86-64 by passing "32" or "64" respectively, the default build is x86-64.

### Linux
- Install QEMU
- Install nasm
- run `run_qemu.sh`, you can choose to build for x86 or x86-64 by passing "32" or "64" respectively, the default build is x86-64.

Virtualization software other than QEMU (like VMware, VirtualBox) is also usable by manually loading the VMDK image from `Images` (generated via `Scripts/build_ultra.{sh/bat}`).
