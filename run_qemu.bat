@echo off

set arch=64

if "%~1" == "" goto arch_picked

IF "%~1" == "32" (
    goto arch_32
) ELSE (
    IF "%~1" == "64" (
    goto arch_picked
) ELSE (
    echo Unknown architecture %1
    pause
    exit 1
)
)

:arch_32
    set arch=32

:arch_picked

call Scripts\build_ultra.bat %arch% || goto error
qemu-system-x86_64 -drive file="Images/Ultra%arch%HDD.vmdk",index=0,media=disk ^
                   -debugcon stdio ^
                   -serial file:Ultra%arch%log.txt ^
                   -smp 4 ^
                   -m 128 ^
                   -no-reboot -no-shutdown ^
                   -vga std
exit 0

:error
popd
exit 1

