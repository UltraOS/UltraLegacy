@echo off
pushd %~dp0\..
mkdir Project
pushd Project
cmake .. || goto error
popd
popd
exit 0

:error
popd
pause