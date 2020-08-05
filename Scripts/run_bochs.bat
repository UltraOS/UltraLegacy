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

call build_ultra.bat %arch% || goto error

pushd %~dp0\..\
set image_file=%CD%\Images\Ultra%arch%HDD-flat.vmdk
echo %image_file%
del ".\Images\Ultra%arch%HDD-flat.vmdk.lock" 2>NUL
bochs.exe -f %CD%\Scripts\bochsrc.bxrc
del bx_enh_dbg.ini 2>NUL
popd
exit 0

:error
popd
exit 1