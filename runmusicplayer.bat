@echo off
setlocal
cd /d "%~dp0"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build --parallel 2
if errorlevel 1 exit /b 1
"%~dp0build\music_player.exe" %*
