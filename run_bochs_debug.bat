@echo off
call Boot\Windows\build_floppy.bat
pushd %~dp0
SET images=%CD%\Boot\images
popd
bochsdbg -f Boot\Windows\bochsrc_debug.bxrc
del bx_enh_dbg.ini