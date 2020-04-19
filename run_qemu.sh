/bin/bash Scripts/build_image.sh || exit 1
qemu-system-i386 -cdrom Images/UltraDisk.iso
