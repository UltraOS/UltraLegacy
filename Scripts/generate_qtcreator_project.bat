@echo off
pushd %~dp0
wsl ./generate_qtcreator_project.sh || goto error
popd
exit 0

:error
popd
pause