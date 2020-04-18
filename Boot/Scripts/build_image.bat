@echo off
pushd %~dp0
wsl ./build_image.sh || exit /B 1
popd