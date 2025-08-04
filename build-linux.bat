@echo off
echo Building Mercury Engine for Linux (WSL)...

wsl cmake --preset wsl-linux
wsl cmake --build --preset wsl-linux

echo Build complete!
pause 