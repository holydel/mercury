@echo off
echo Building Mercury Engine for Windows (Visual Studio 2022)...

cmake --preset windows-vs2022
cmake --build --preset windows-vs2022

echo Build complete!
pause 