@echo off
call Boot\Scripts\build_image.bat
pushd %~dp0
SET images=%CD%\Boot\Images
popd
bochs -f Configuration\bochsrc.bxrc