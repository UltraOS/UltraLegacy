@echo off
call Boot\Scripts\build_image.bat
pushd %~dp0
SET images=%CD%\Boot\Images
popd
bochsdbg -f configuration\bochsrc_debug.bxrc
del bx_enh_dbg.ini