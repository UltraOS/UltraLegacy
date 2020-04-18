@echo off
call Boot\Scripts\build_image.bat || exit /B 1
pushd %~dp0
set images=%CD%\Boot\Images
popd
bochsdbg -f configuration\bochsrc_debug.bxrc
del bx_enh_dbg.ini