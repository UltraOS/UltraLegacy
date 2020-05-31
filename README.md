# UltraOS
An operating system that doesn't try to be UNIX. Made completely from scratch with its own bootloader. 😊

## Getting started

Windows
- Install Bochs, add it into your `Path`
- Install WSL Ubuntu, or any other distro with apt
- Install nasm for your WSL
- run `run_bochs.bat` or `run_bochs_debug.bat`

Linux
- Install QEMU
- Install nasm
- run `run_qemu.sh`

Virtualization software other than QEMU and Bochs (like VMware or VirtualBox) is also usable by manually loading the VMDK image from `Images` (generated via `Scripts/build_ultra.{sh/bat}`).
