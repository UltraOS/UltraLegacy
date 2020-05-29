@echo off
pushd %~dp0
wsl ./build.sh || goto error
popd
exit /B 0

:error
popd
pause
exit /B 1