@echo off
call Boot\Scripts\build_image.bat
pushd %~dp0
SET images=%CD%\Boot\images
popd
bochs -f Configuration\bochsrc.bxrc