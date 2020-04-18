@echo off
pushd %~dp0
wsl ./build.sh || exit /B 1
popd
pause