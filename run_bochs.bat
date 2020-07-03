@echo off
call Scripts\build_ultra.bat || goto error
pushd %~dp0
set images=%CD%\Images
popd
bochs -unlock -f Scripts\bochsrc.bxrc
exit 0

:error
popd
exit 1