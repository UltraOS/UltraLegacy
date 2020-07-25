#!/bin/bash

/bin/bash Scripts/build_ultra.sh || exit 1
qemu-system-x86_64 -drive file="Images/Ultra32HDD.vmdk",index=0,media=disk \
                   -debugcon stdio                                         \
                   -smp 2                                                  \
                   -m 128                                                  \
                   -no-reboot                                              \
