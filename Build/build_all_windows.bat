@echo off

setlocal EnableExtensions EnableDelayedExpansion



REM Build script for CYDevice - Builds all configurations

REM Output structure: Bin/Windows/{x86|x64}/{MT|MD|MTD|MDD}/{Debug|Release}/



set "SCRIPT_DIR=%~dp0"

for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"

set "BUILD_DIR=%SCRIPT_DIR%cmake_build"

set "BIN_DIR=%PROJECT_ROOT%\Bin"

set "CMAKE_SOURCE_DIR=%PROJECT_ROOT%"



echo ========================================

echo CYDevice Build All Configurations

echo ========================================

echo.



where cmake >nul 2>&1

if errorlevel 1 (

    echo ERROR: CMake is not found in PATH!

    exit /b 1

)



if exist "%BUILD_DIR%" (

    echo Cleaning previous build directory...

    rmdir /s /q "%BUILD_DIR%"

)

mkdir "%BUILD_DIR%"



set /a BUILD_COUNT=0

set /a SUCCESS_COUNT=0

set /a FAIL_COUNT=0



echo Starting builds...

echo.



call :BuildOne x86 Win32 Debug MultiThreadedDebug MTD CYDeviceD.lib

call :BuildOne x86 Win32 Debug MultiThreadedDebugDLL MDD CYDeviceD.lib

call :BuildOne x86 Win32 Release MultiThreaded MT CYDevice.lib

call :BuildOne x86 Win32 Release MultiThreadedDLL MD CYDevice.lib

call :BuildOne x64 x64 Debug MultiThreadedDebug MTD CYDeviceD.lib

call :BuildOne x64 x64 Debug MultiThreadedDebugDLL MDD CYDeviceD.lib

call :BuildOne x64 x64 Release MultiThreaded MT CYDevice.lib

call :BuildOne x64 x64 Release MultiThreadedDLL MD CYDevice.lib



goto :end



:BuildOne

setlocal EnableExtensions EnableDelayedExpansion

set "ARCH=%~1"

set "CMAKE_PLATFORM=%~2"

set "CONFIG_TYPE=%~3"

set "RUNTIME_LIB=%~4"

set "RUNTIME_FOLDER=%~5"

set "EXPECTED_LIB=%~6"



set /a BUILD_COUNT+=1

echo [!BUILD_COUNT!] Building: Windows/!ARCH!/!RUNTIME_FOLDER!/!CONFIG_TYPE!



call :EnsureDependencies "!ARCH!" "!CMAKE_PLATFORM!" "!CONFIG_TYPE!" "!RUNTIME_LIB!" "!RUNTIME_FOLDER!"

if errorlevel 1 (

    echo ERROR: Failed to build dependencies for !ARCH!/!RUNTIME_FOLDER!/!CONFIG_TYPE!

    endlocal & set /a FAIL_COUNT+=1 & echo. & goto :eof

)



set "CONFIG_BUILD_DIR=%BUILD_DIR%\!ARCH!_!RUNTIME_FOLDER!_!CONFIG_TYPE!"

if not exist "!CONFIG_BUILD_DIR!" mkdir "!CONFIG_BUILD_DIR!"



cmake -S "%CMAKE_SOURCE_DIR%" -B "!CONFIG_BUILD_DIR!" -G "Visual Studio 17 2022" -A "!CMAKE_PLATFORM!" -DCMAKE_BUILD_TYPE="!CONFIG_TYPE!" -DCMAKE_MSVC_RUNTIME_LIBRARY="!RUNTIME_LIB!" -DCMAKE_INSTALL_PREFIX="%BIN_DIR%\Windows\!ARCH!\!RUNTIME_FOLDER!\!CONFIG_TYPE!"

if errorlevel 1 (

    echo ERROR: CMake configuration failed for !ARCH!/!RUNTIME_FOLDER!/!CONFIG_TYPE!

    endlocal & set /a FAIL_COUNT+=1 & echo. & goto :eof

)



cmake --build "!CONFIG_BUILD_DIR!" --config "!CONFIG_TYPE!" --target CYDevice --parallel %NUMBER_OF_PROCESSORS%

if errorlevel 1 (

    echo ERROR: Build failed for !ARCH!/!RUNTIME_FOLDER!/!CONFIG_TYPE!

    endlocal & set /a FAIL_COUNT+=1 & echo. & goto :eof

)



set "OUTPUT_DIR=%BIN_DIR%\Windows\!ARCH!\!RUNTIME_FOLDER!\!CONFIG_TYPE!"

if not exist "!OUTPUT_DIR!" mkdir "!OUTPUT_DIR!"



set "FOUND_LIB="

if exist "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!EXPECTED_LIB!" set "FOUND_LIB=!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!EXPECTED_LIB!"

if not defined FOUND_LIB if exist "!CONFIG_BUILD_DIR!\lib\!EXPECTED_LIB!" set "FOUND_LIB=!CONFIG_BUILD_DIR!\lib\!EXPECTED_LIB!"

if not defined FOUND_LIB if exist "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!EXPECTED_LIB!" set "FOUND_LIB=!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!EXPECTED_LIB!"



if not defined FOUND_LIB (

    echo ERROR: Library file not found after build: !EXPECTED_LIB!

    endlocal & set /a FAIL_COUNT+=1 & echo. & goto :eof

)



copy /Y "!FOUND_LIB!" "!OUTPUT_DIR!\" >nul

echo   Success: Copied !EXPECTED_LIB! to !OUTPUT_DIR!

endlocal & set /a SUCCESS_COUNT+=1 & echo   Build completed successfully! & echo. & goto :eof



:EnsureDependencies

setlocal EnableExtensions EnableDelayedExpansion

set "ARCH=%~1"

set "CMAKE_PLATFORM=%~2"

set "CONFIG_TYPE=%~3"

set "RUNTIME_LIB=%~4"

set "RUNTIME_FOLDER=%~5"

set "DEP_ARCH_DIR=%BIN_DIR%\Windows\%~1\%~5\%~3"



echo Checking dependencies for %~1 - %~3 - %~5

if not exist "!DEP_ARCH_DIR!" mkdir "!DEP_ARCH_DIR!"



call :EnsureCYLoggerStack "!ARCH!" "!CONFIG_TYPE!" "!RUNTIME_FOLDER!" "!DEP_ARCH_DIR!"

if errorlevel 1 endlocal & exit /b 1



call :EnsureLibSamplerate "!ARCH!" "!CMAKE_PLATFORM!" "!CONFIG_TYPE!" "!RUNTIME_LIB!" "!RUNTIME_FOLDER!" "!DEP_ARCH_DIR!"

if errorlevel 1 endlocal & exit /b 1



endlocal & exit /b 0



:EnsureCYLoggerStack

setlocal EnableExtensions EnableDelayedExpansion

set "ARCH=%~1"

set "CONFIG_TYPE=%~2"

set "RUNTIME_FOLDER=%~3"

set "DEP_ARCH_DIR=%~4"



if /I "!ARCH!"=="x64" (

    set "LOGGER_ARCH=x64"

    set "LOGGER_OUTPUT_ARCH=x86_64"

) else (

    set "LOGGER_ARCH=x86"

    set "LOGGER_OUTPUT_ARCH=x86"

)



if "!CONFIG_TYPE!"=="Debug" (

    set "LOGGER_LIB_NAME=CYLoggerStaticD.lib"

    set "COMMON_LIB_NAME=CYCommonD.lib"

    set "COROUTINE_LIB_NAME=CYCoroutineD.lib"

) else (

    set "LOGGER_LIB_NAME=CYLoggerStatic.lib"

    set "COMMON_LIB_NAME=CYCommon.lib"

    set "COROUTINE_LIB_NAME=CYCoroutine.lib"

)

set "LOGGER_DST=!DEP_ARCH_DIR!\!LOGGER_LIB_NAME!"

set "COMMON_DST=!DEP_ARCH_DIR!\!COMMON_LIB_NAME!"

set "COROUTINE_DST=!DEP_ARCH_DIR!\!COROUTINE_LIB_NAME!"



if exist "!LOGGER_DST!" if exist "!COMMON_DST!" if exist "!COROUTINE_DST!" (

    endlocal & exit /b 0

)



echo CYLogger/CYCommon/CYCoroutine libraries not found, building via CYLogger script...

cmd /c ""%PROJECT_ROOT%\ThirdParty\CYLogger\Build\build_windows.bat" "!CONFIG_TYPE!" "!LOGGER_ARCH!" "!RUNTIME_FOLDER!""

if errorlevel 1 (

    echo ERROR: CYLogger dependency build failed

    endlocal & exit /b 1

)



set "LOGGER_BIN_DIR=%PROJECT_ROOT%\ThirdParty\CYLogger\Bin\Windows\!LOGGER_OUTPUT_ARCH!\!RUNTIME_FOLDER!\!CONFIG_TYPE!"

set "COMMON_BIN_DIR=!LOGGER_BIN_DIR!"

set "COROUTINE_BIN_DIR=%PROJECT_ROOT%\ThirdParty\CYLogger\ThirdParty\CYCoroutine\Bin\Windows\!LOGGER_OUTPUT_ARCH!\!RUNTIME_FOLDER!\!CONFIG_TYPE!"

set "COROUTINE_FALLBACK_BIN_DIR=!LOGGER_BIN_DIR!"



if not exist "!LOGGER_DST!" (

    if exist "!LOGGER_BIN_DIR!\!LOGGER_LIB_NAME!" copy /Y "!LOGGER_BIN_DIR!\!LOGGER_LIB_NAME!" "!LOGGER_DST!" >nul

)

if not exist "!COMMON_DST!" (

    if exist "!COMMON_BIN_DIR!\!COMMON_LIB_NAME!" copy /Y "!COMMON_BIN_DIR!\!COMMON_LIB_NAME!" "!COMMON_DST!" >nul

)

if not exist "!COROUTINE_DST!" (

    if exist "!COROUTINE_BIN_DIR!\!COROUTINE_LIB_NAME!" copy /Y "!COROUTINE_BIN_DIR!\!COROUTINE_LIB_NAME!" "!COROUTINE_DST!" >nul

)

if not exist "!COROUTINE_DST!" (

    if exist "!COROUTINE_FALLBACK_BIN_DIR!\!COROUTINE_LIB_NAME!" copy /Y "!COROUTINE_FALLBACK_BIN_DIR!\!COROUTINE_LIB_NAME!" "!COROUTINE_DST!" >nul

)

if not exist "!COROUTINE_DST!" (

    for /r "%PROJECT_ROOT%\ThirdParty\CYLogger\Build" %%F in (CYCoroutine*.lib) do if not exist "!COROUTINE_DST!" copy /Y "%%F" "!COROUTINE_DST!" >nul

)



if not exist "!LOGGER_DST!" (

    echo ERROR: CYLogger library not found after build

    endlocal & exit /b 1

)

if not exist "!COMMON_DST!" (

    echo ERROR: CYCommon library not found after build

    endlocal & exit /b 1

)

if not exist "!COROUTINE_DST!" (

    echo ERROR: CYCoroutine library not found after build

    endlocal & exit /b 1

)



echo   CYLogger stack copied to !DEP_ARCH_DIR!

endlocal & exit /b 0



:EnsureLibSamplerate

setlocal EnableExtensions EnableDelayedExpansion

set "ARCH=%~1"

set "CMAKE_PLATFORM=%~2"

set "CONFIG_TYPE=%~3"

set "RUNTIME_LIB=%~4"

set "RUNTIME_FOLDER=%~5"

set "DEP_ARCH_DIR=%~6"



if "!CONFIG_TYPE!"=="Debug" (

    set "SAMPLERATE_LIB_NAME=libsamplerated.lib"

) else (

    set "SAMPLERATE_LIB_NAME=libsamplerate.lib"

)

set "SAMPLERATE_DST=!DEP_ARCH_DIR!\!SAMPLERATE_LIB_NAME!"

if exist "!SAMPLERATE_DST!" (

    endlocal & exit /b 0

)



echo libsamplerate library not found, building it...

set "SAMPLERATE_BUILD_DIR=%BUILD_DIR%\deps_libsamplerate_!ARCH!_!RUNTIME_FOLDER!_!CONFIG_TYPE!"

if not exist "!SAMPLERATE_BUILD_DIR!" mkdir "!SAMPLERATE_BUILD_DIR!"



cmake -S "%PROJECT_ROOT%\ThirdParty\libsamplerate" -B "!SAMPLERATE_BUILD_DIR!" -G "Visual Studio 17 2022" -A "!CMAKE_PLATFORM!" -DCMAKE_BUILD_TYPE="!CONFIG_TYPE!" -DCMAKE_MSVC_RUNTIME_LIBRARY="!RUNTIME_LIB!"

if errorlevel 1 (

    echo ERROR: libsamplerate CMake configuration failed

    endlocal & exit /b 1

)



cmake --build "!SAMPLERATE_BUILD_DIR!" --config "!CONFIG_TYPE!" --target libsamplerate --parallel %NUMBER_OF_PROCESSORS%

if errorlevel 1 (

    echo ERROR: libsamplerate build failed

    endlocal & exit /b 1

)



set "SAMPLERATE_SRC="

if exist "!SAMPLERATE_BUILD_DIR!\Debug\!SAMPLERATE_LIB_NAME!" set "SAMPLERATE_SRC=!SAMPLERATE_BUILD_DIR!\Debug\!SAMPLERATE_LIB_NAME!"

if not defined SAMPLERATE_SRC if exist "!SAMPLERATE_BUILD_DIR!\Release\!SAMPLERATE_LIB_NAME!" set "SAMPLERATE_SRC=!SAMPLERATE_BUILD_DIR!\Release\!SAMPLERATE_LIB_NAME!"

if not defined SAMPLERATE_SRC for /r "!SAMPLERATE_BUILD_DIR!" %%F in (libsamplerate*.lib) do if not defined SAMPLERATE_SRC set "SAMPLERATE_SRC=%%F"

if not defined SAMPLERATE_SRC (

    echo ERROR: libsamplerate library not found after build

    endlocal & exit /b 1

)



copy /Y "!SAMPLERATE_SRC!" "!SAMPLERATE_DST!" >nul

echo   libsamplerate library copied to !SAMPLERATE_DST!

endlocal & exit /b 0



:end

echo ========================================

echo Build Summary

echo ========================================

echo Total builds: %BUILD_COUNT%

echo Successful: %SUCCESS_COUNT%

echo Failed: %FAIL_COUNT%

echo.



if %FAIL_COUNT% EQU 0 (

    echo All builds completed successfully!

    echo Output directory: %BIN_DIR%\Windows\

    exit /b 0

) else (

    echo Some builds failed. Please check the errors above.

    exit /b 1

)

