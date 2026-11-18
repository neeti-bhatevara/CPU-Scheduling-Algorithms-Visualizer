@echo off
setlocal EnableDelayedExpansion
title CPU Scheduling and Visualization System - Launcher

:: Navigate to script directory, supporting paths with spaces and brackets
cd /d "%~dp0"

echo ============================================================
echo   CPU Scheduling and Visualization System
echo   College Demonstration Environment Startup
echo ============================================================
echo.

:: 1. Verify Python installation
echo [1/4] Checking Python environment...
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python was not found in your system PATH!
    echo Please install Python 3.8 or higher from https://python.org
    echo Make sure to check "Add Python to PATH" during installation.
    echo.
    pause
    exit /b 1
)
for /f "tokens=*" %%v in ('python --version 2^>^&1') do set "PY_VER=%%v"
echo       Found %PY_VER%

:: 2. Check C executable and source modification times
echo.
echo [2/4] Checking Native C11 Engine (build\cpu-scheduler.exe)...
set "REBUILD_REQUIRED=0"

if not exist "build\cpu-scheduler.exe" (
    echo       Executable build\cpu-scheduler.exe not found. Build required.
    set "REBUILD_REQUIRED=1"
) else (
    :: Check if any C source or header file is newer than the binary
    powershell -NoProfile -ExecutionPolicy Bypass -Command "if ((Get-ChildItem -Path '*.c', '*.h' | Measure-Object -Property LastWriteTime -Maximum).Maximum -gt (Get-Item 'build\cpu-scheduler.exe').LastWriteTime) { exit 1 } else { exit 0 }" >nul 2>&1
    if errorlevel 1 (
        echo       C source files have been modified. Automatic recompilation required.
        set "REBUILD_REQUIRED=1"
    ) else (
        echo       Engine binary is up to date.
    )
)

if "%REBUILD_REQUIRED%"=="1" (
    where gcc >nul 2>&1
    if errorlevel 1 (
        if not exist "build\cpu-scheduler.exe" (
            echo [ERROR] GCC compiler not found in PATH and binary is missing!
            echo Please install MinGW-w64 or GCC and ensure 'gcc' is in your system PATH.
            echo.
            pause
            exit /b 1
        ) else (
            echo [WARNING] GCC not found in PATH. Using existing build\cpu-scheduler.exe binary.
        )
    ) else (
        echo       Compiling C11 scheduling algorithms...
        if not exist "build" mkdir "build"
        gcc -std=c11 -Wall -Wextra main.c fcfs.c sjf.c srtf.c rr.c priority_np.c priority_p.c hrrn.c mlfq.c process.c -o build\cpu-scheduler.exe
        if errorlevel 1 (
            echo [ERROR] GCC compilation failed!
            echo Please inspect source files for syntax errors.
            echo.
            pause
            exit /b 1
        )
        echo       Compilation successful: build\cpu-scheduler.exe ready.
    )
)

:: 3. Check if Python backend is already running (prevent duplicate instances)
echo.
echo [3/4] Checking Backend Server status...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$client = New-Object System.Net.Sockets.TcpClient; try { $client.Connect('127.0.0.1', 8080); exit 0 } catch { exit 1 } finally { $client.Close() }" >nul 2>&1
if errorlevel 1 (
    echo       Starting Python backend server on port 8080...
    start "CPU-Scheduler-Backend" /min python backend\server.py 8080
    :: Wait briefly for server to bind
    timeout /t 2 /nobreak >nul
    echo       Backend server launched successfully.
) else (
    echo       Backend server is already running and active on port 8080.
)

:: 4. Launch web browser with local URL
echo.
echo [4/4] Launching Interactive Visualizer Dashboard...
set "DASHBOARD_URL=http://localhost:8080/frontend/index.html"
echo       Opening %DASHBOARD_URL% in default browser...
start "" "%DASHBOARD_URL%"

echo.
echo ============================================================
echo   SUCCESS: SYSTEM ONLINE AND READY FOR DEMONSTRATION
echo ============================================================
echo   Web Dashboard: %DASHBOARD_URL%
echo   Backend API:   http://localhost:8080/api/health
echo   Engine Binary: build\cpu-scheduler.exe
echo ============================================================
echo   You can minimize this window during your presentation.
echo   Press any key to close this launcher.
echo ============================================================
pause >nul
