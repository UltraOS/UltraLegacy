@echo off
pushd %~dp0
wsl ./build.sh
popd
pause