/bin/bash Boot/Scripts/build_image.sh || exit 1
qemu-system-i386 -cdrom Boot/images/UltraDisk.iso
