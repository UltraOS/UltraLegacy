#!/bin/bash

/bin/bash Scripts/build_image.sh || exit 1
qemu-system-i386 -drive file="images/UltraHDD.vmdk",index=0,media=disk
