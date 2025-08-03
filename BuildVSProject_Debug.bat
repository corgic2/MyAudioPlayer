@ECHO OFF
set CURDIR=%~dp0

set "FFMPEGDIR=D:\WorkSpace\ffmpeg-7.1.1-full_build-shared"
set "SDLDIR=D:\WorkSpace\SDL3-3.2.14\x86_64-w64-mingw32"
set "CustomUIDir=D:\WorkSpace\MyCustomUIWidget"
set "SDKDIR=D:\WorkSpace\MySdk"

if not exist "%~dp0include\" mkdir "%~dp0include"
if not exist "%~dp0x64\Debug\" mkdir "%~dp0x64\Debug"

xcopy "%FFMPEGDIR%\include\*" "%~dp0include\" /E /I /Y
xcopy "%SDLDIR%\include\*" "%~dp0include\" /E /I /Y
xcopy "%CustomUIDir%\MuCustomUiWidget\*.h" "%~dp0include\" /E /I /Y
xcopy "%CustomUIDir%\MuCustomUiWidget\*.ui" "%~dp0include\" /E /I /Y
xcopy "%SDKDIR%\include\*.h" "%~dp0include\" /E /I /Y
xcopy "%SDKDIR%\x64\Debug\*" "%~dp0x64\Debug\" /E /I /Y

xcopy "%FFMPEGDIR%\lib\*" "%~dp0x64\Debug\" /E /I /Y
xcopy "%FFMPEGDIR%\bin\*" "%~dp0x64\Debug\" /E /I /Y
xcopy "%CustomUIDir%\x64\Debug\*.dll" "%~dp0x64\Debug\" /E /I /Y
xcopy "%CustomUIDir%\x64\Debug\*.lib" "%~dp0x64\Debug\" /E /I /Y
echo Operation completed.

echo ========================================
echo Starting CMake configuration...
echo ========================================

md build
cd build
cmake -DCMAKE_CONFIGURATION_TYPES=Debug .. -G "Visual Studio 17 2022"
if %ERRORLEVEL% neq 0 (
    echo ========================================
    echo ERROR: CMake configuration failed!
    echo ========================================
    pause
    exit /b 1
)

echo ========================================
echo Starting compilation...
echo ========================================

cmake --build . --config Debug
if %ERRORLEVEL% neq 0 (
    echo ========================================
    echo ERROR: Compilation failed!
    echo ========================================
    pause
    exit /b 1
)

echo ========================================
echo Compilation successful! Running windeployqt...
echo ========================================

@REM windeployqt
if defined QT_DIR (
    set QTDIR=%QT_DIR%
) else (
    set QTDIR=D:/Qt/5.15.2/msvc2019_64
)

cd %CURDIR%
set OUTPUTDIR=%~dp0\x64\Debug
cd %OUTPUTDIR%
for /f %%I in ('dir /b *.exe *.dll') do (
    %QTDIR%\bin\windeployqt.exe --dir %OUTPUTDIR% --libdir %OUTPUTDIR% --plugindir %OUTPUTDIR% --no-translations %OUTPUTDIR%\%%~nxI
)

echo ========================================
echo Build process completed successfully!
echo ========================================
pause 