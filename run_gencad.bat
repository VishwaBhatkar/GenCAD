@echo off
setlocal
cd /d "%~dp0"

:: Set Qt 6 runtime libraries in PATH
set "PATH=C:\Qt\6.8.2\msvc2022_64\bin;%PATH%"

echo ========================================================
echo               Launching GenCAD 2D CAD Suite
echo ========================================================

:: 1. Launch FastAPI Cloud Backend in a separate window
echo [1/2] Starting Cloud Backend Server on http://127.0.0.1:8000 ...
start "GenCAD Cloud Backend Server" cmd /k "C:\Users\vishw\AppData\Local\Python\pythoncore-3.14-64\python.exe backend\run_server.py"

:: 2. Wait 1 second for backend initialization
timeout /t 1 /nobreak >nul

:: 3. Launch GenCAD Desktop Application
echo [2/2] Launching GenCAD Desktop Application...
start "" "%~dp0build-release\gencad.exe"

echo GenCAD started successfully!
