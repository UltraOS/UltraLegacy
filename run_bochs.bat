@echo off
call Scripts\build_ultra.bat || goto error
pushd %~dp0
set images=%CD%\Images
popd
del ".\Images\UltraHDD-flat.vmdk.lock" 2>NUL
bochs.exe -f Scripts\bochsrc.bxrc
exit 0

:error
popd
exit 1