@echo off
call Scripts\build_ultra.bat || goto error
pushd %~dp0
set images=%CD%\Images
popd
bochsdbg -unlock -f configuration\bochsrc_debug.bxrc
del bx_enh_dbg.ini
exit 0

:error
popd
exit 1