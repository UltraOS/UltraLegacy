#!/bin/bash

arch=64

if [ "$1" ]
  then
    if [ $1 != "64" ] && [ $1 != "32" ]
    then
      echo "Unknown architecture $1"
      on_error
    else
      arch="$1"
    fi
fi

/bin/bash Scripts/build_ultra.sh $arch || exit 1
qemu-system-x86_64 -drive file="Images/Ultra${arch}HDD.vmdk",index=0,media=disk \
                   -debugcon stdio                                              \
                   -serial file:Ultra%arch%log.txt                              \
                   -smp 4                                                       \
                   -m 128                                                       \
                   -no-reboot                                                   \
                   --enable-kvm                                                 \
