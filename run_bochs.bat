@echo off
call Scripts\build_image.bat || exit /B 1
pushd %~dp0
set images=%CD%\Images
popd
bochs -unlock -f Configuration\bochsrc.bxrc