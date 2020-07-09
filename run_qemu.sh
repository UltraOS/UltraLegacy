#!/bin/bash

/bin/bash Scripts/build_ultra.sh || exit 1
qemu-system-i386 -drive file="Images/UltraHDD.vmdk",index=0,media=disk \
                 -debugcon stdio                                       \
                 -cpu core2duo                                         \
                 -smp 2                                                \
                 -m 128                                                \
                 -no-reboot                                            \
