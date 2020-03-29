@echo off
call Boot\Windows\build_floppy.bat
pushd %~dp0
SET images=%CD%\Boot\images
popd
bochs -f Boot\Windows\bochsrc.bxrc