@echo off
pushd %~dp0
wsl ./build_ultra.sh || exit /B 1
popd