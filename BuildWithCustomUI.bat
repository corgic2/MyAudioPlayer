@ECHO OFF
setlocal enabledelayedexpansion

:: Set path variables
set "AUDIO_PLAYER_DIR=D:\WorkSpace\MyAudioPlayer"
set "CUSTOM_UI_DIR=D:\WorkSpace\MyCustomUIWidget"

:: Set color output
set "COLOR_SUCCESS=0A"
set "COLOR_ERROR=0C"
set "COLOR_INFO=0F"
set "COLOR_WARNING=0E"

echo.
echo ========================================
echo    Building Custom UI Components...
echo ========================================
echo.

:: Switch to custom UI directory
cd /d "%CUSTOM_UI_DIR%"
if !ERRORLEVEL! neq 0 (
    color %COLOR_ERROR%
    echo ERROR: Cannot switch to custom UI directory: %CUSTOM_UI_DIR%
    pause
    exit /b 1
)

:: Check if UI build script exists
if not exist "BuildVSProject_Debug.bat" (
    color %COLOR_ERROR%
    echo ERROR: Custom UI build script not found: BuildVSProject_Debug.bat
    pause
    exit /b 1
)

echo Executing custom UI build script...
call "BuildVSProject_Debug.bat"
if !ERRORLEVEL! neq 0 (
    color %COLOR_ERROR%
    echo ERROR: Custom UI build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Custom UI Build Completed!
echo ========================================
echo.

:: Switch back to audio player directory
cd /d "%AUDIO_PLAYER_DIR%"
if !ERRORLEVEL! neq 0 (
    color %COLOR_ERROR%
    echo ERROR: Cannot switch back to audio player directory: %AUDIO_PLAYER_DIR%
    pause
    exit /b 1
)

echo.
echo ========================================
echo    Building Audio Player...
echo ========================================
echo.

:: Check if audio player build script exists
if not exist "BuildVSProject_Debug.bat" (
    color %COLOR_ERROR%
    echo ERROR: Audio player build script not found: BuildVSProject_Debug.bat
    pause
    exit /b 1
)

echo Executing audio player build script...
call "BuildVSProject_Debug.bat"
if !ERRORLEVEL! neq 0 (
    color %COLOR_ERROR%
    echo ERROR: Audio player build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo    All Builds Completed Successfully!
echo ========================================
echo.
echo Build Results:
echo   - Custom UI Components: Success
echo   - Audio Player: Success
echo.

color %COLOR_SUCCESS%
echo Press any key to exit...
pause >nul
color