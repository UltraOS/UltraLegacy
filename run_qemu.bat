@echo off
call Scripts\build_ultra.bat || goto error
qemu-system-x86_64 -drive file="Images/Ultra32HDD.vmdk",index=0,media=disk -debugcon stdio -smp 2 -m 128 -no-reboot
exit 0

:error
popd
exit 1

