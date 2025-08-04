@echo off
echo Building Mercury Engine for Emscripten...

cmake --preset emscripten
cmake --build --preset emscripten

echo Build complete!
pause 