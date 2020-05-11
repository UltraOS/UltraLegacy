@echo off
call Scripts\build_image.bat || exit /B 1
pushd %~dp0
set images=%CD%\Images
popd
bochsdbg -unlock -f configuration\bochsrc_debug.bxrc
del bx_enh_dbg.ini