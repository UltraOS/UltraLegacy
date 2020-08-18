@echo off
mkdir build
pushd build
cmake ..        || goto error
cmake --build . || goto error
popd
cls
bin\RunTests.exe
pause
exit /B 0

:error
popd
pause
exit /B 1