@echo off
call Scripts\build_ultra.bat || goto error
pushd %~dp0
set images=%CD%\Images
popd
del ".\Images\UltraHDD-flat.vmdk.lock" 2>NUL
bochsdbg.exe -f Scripts\bochsrc_debug.bxrc
del bx_enh_dbg.ini 2>NUL
exit 0

:error
popd
exit 1