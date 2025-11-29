@echo off
setlocal enabledelayedexpansion

REM Build script for CYDevice - Builds all configurations
REM Output structure: Bin/Windows/{x86|x64}/{MD|MT}/{Debug|Release}/

set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"
set PROJECT_ROOT=%~dp0..
cd /d "%PROJECT_ROOT%"
set PROJECT_ROOT=%CD%
set BUILD_DIR=%SCRIPT_DIR%cmake_build
set BIN_DIR=%PROJECT_ROOT%\Bin
cd /d "%SCRIPT_DIR%"
set CMAKE_SOURCE_DIR=%PROJECT_ROOT%

echo ========================================
echo CYDevice Build All Configurations
echo ========================================
echo.

REM Check if CMake is available
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake is not found in PATH!
    echo Please install CMake and add it to your PATH.
    pause
    exit /b 1
)

REM Clean previous build directory
if exist "%BUILD_DIR%" (
    echo Cleaning previous build directory...
    rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
mkdir "%BUILD_DIR%"

REM Define configurations
REM Debug: Only MTD and MDD
REM Release: Only MT and MD
set PLATFORMS=x86 x64

REM Build counter
set BUILD_COUNT=0
set SUCCESS_COUNT=0
set FAIL_COUNT=0

echo Starting builds...
echo.

REM Build all combinations
for %%P in (%PLATFORMS%) do (
    REM Set platform-specific variables
    if "%%P"=="x86" (
        set CMAKE_GENERATOR=Visual Studio 17 2022
        set CMAKE_PLATFORM=Win32
        set OUTPUT_ARCH=x86
    ) else (
        set CMAKE_GENERATOR=Visual Studio 17 2022
        set CMAKE_PLATFORM=x64
        set OUTPUT_ARCH=x64
    )
    
    REM Build Debug configurations (MTD and MDD only)
    REM MTD (MT Debug)
    set /a BUILD_COUNT+=1
    echo [!BUILD_COUNT!] Building: Windows/%%P/MTD/Debug
    
    set RUNTIME_LIB=MultiThreadedDebug
    set RUNTIME_FOR_CONFIG=MTD
    set CONFIG_TYPE=Debug
    set RUNTIME_TYPE=MT
    
    REM Check and build dependencies
    call :EnsureDependencies %%P !CONFIG_TYPE! !RUNTIME_TYPE! !OUTPUT_ARCH! !RUNTIME_FOR_CONFIG!
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: Failed to build dependencies for %%P/MTD/!CONFIG_TYPE!
        set /a FAIL_COUNT+=1
        echo.
        cd /d "%SCRIPT_DIR%"
        goto :continue_mtd
    )
    
    REM Create build directory for this configuration
    set CONFIG_BUILD_DIR=%BUILD_DIR%\%%P_MTD_!CONFIG_TYPE!
    mkdir "!CONFIG_BUILD_DIR!" 2>nul
    
    REM Configure
    cd /d "!CONFIG_BUILD_DIR!"
    cmake -G "!CMAKE_GENERATOR!" ^
        -A "!CMAKE_PLATFORM!" ^
        -DCMAKE_BUILD_TYPE=!CONFIG_TYPE! ^
        -DCMAKE_MSVC_RUNTIME_LIBRARY=!RUNTIME_LIB! ^
        -DCMAKE_INSTALL_PREFIX="%BIN_DIR%\Windows\%%P\MTD\!CONFIG_TYPE!" ^
        "%CMAKE_SOURCE_DIR%"
    
    set BUILD_SUCCESS=1
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: CMake configuration failed for %%P/MTD/!CONFIG_TYPE!
        set /a FAIL_COUNT+=1
        set BUILD_SUCCESS=0
        cd /d "%SCRIPT_DIR%"
        goto :continue_mtd
    ) else (
        REM Build
        cmake --build . --config !CONFIG_TYPE! --target CYDevice
        
        if !ERRORLEVEL! NEQ 0 (
            echo ERROR: Build failed for %%P/MTD/!CONFIG_TYPE!
            set /a FAIL_COUNT+=1
            set BUILD_SUCCESS=0
            cd /d "%SCRIPT_DIR%"
            goto :continue_mtd
        ) else (
            REM Create output directory structure
            set OUTPUT_DIR=%BIN_DIR%\Windows\%%P\MTD\!CONFIG_TYPE!
            if not exist "!OUTPUT_DIR!" (
                mkdir "!OUTPUT_DIR!"
            )
            
            REM Copy library files
            set LIB_NAME=CYDeviceD.lib
            set LIB_FOUND=0
            if exist "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            if !LIB_FOUND! EQU 0 if exist "!CONFIG_BUILD_DIR!\lib\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\lib\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            if !LIB_FOUND! EQU 0 if exist "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            
            if !LIB_FOUND! EQU 1 (
                echo   Success: Copied !LIB_NAME! to !OUTPUT_DIR!
                set /a SUCCESS_COUNT+=1
                echo   Build completed successfully!
            ) else (
                echo   WARNING: Library file not found: !LIB_NAME!
                set /a FAIL_COUNT+=1
            )
        )
    )
    
    :continue_mtd
    cd /d "%SCRIPT_DIR%"
    echo.
    
    REM MDD (MD Debug)
    set /a BUILD_COUNT+=1
    echo [!BUILD_COUNT!] Building: Windows/%%P/MDD/Debug
    
    set RUNTIME_LIB=MultiThreadedDebugDLL
    set RUNTIME_FOR_CONFIG=MDD
    set CONFIG_TYPE=Debug
    set RUNTIME_TYPE=MD
    
    REM Check and build dependencies
    call :EnsureDependencies %%P !CONFIG_TYPE! !RUNTIME_TYPE! !OUTPUT_ARCH! !RUNTIME_FOR_CONFIG!
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: Failed to build dependencies for %%P/MDD/!CONFIG_TYPE!
        set /a FAIL_COUNT+=1
        echo.
        cd /d "%SCRIPT_DIR%"
        goto :continue_mdd
    )
    
    REM Create build directory for this configuration
    set CONFIG_BUILD_DIR=%BUILD_DIR%\%%P_MDD_!CONFIG_TYPE!
    mkdir "!CONFIG_BUILD_DIR!" 2>nul
    
    REM Configure
    cd /d "!CONFIG_BUILD_DIR!"
    cmake -G "!CMAKE_GENERATOR!" ^
        -A "!CMAKE_PLATFORM!" ^
        -DCMAKE_BUILD_TYPE=!CONFIG_TYPE! ^
        -DCMAKE_MSVC_RUNTIME_LIBRARY=!RUNTIME_LIB! ^
        -DCMAKE_INSTALL_PREFIX="%BIN_DIR%\Windows\%%P\MDD\!CONFIG_TYPE!" ^
        "%CMAKE_SOURCE_DIR%"
    
    set BUILD_SUCCESS=1
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: CMake configuration failed for %%P/MDD/!CONFIG_TYPE!
        set /a FAIL_COUNT+=1
        set BUILD_SUCCESS=0
        cd /d "%SCRIPT_DIR%"
        goto :continue_mdd
    ) else (
        REM Build
        cmake --build . --config !CONFIG_TYPE! --target CYDevice
        
        if !ERRORLEVEL! NEQ 0 (
            echo ERROR: Build failed for %%P/MDD/!CONFIG_TYPE!
            set /a FAIL_COUNT+=1
            set BUILD_SUCCESS=0
            cd /d "%SCRIPT_DIR%"
            goto :continue_mdd
        ) else (
            REM Create output directory structure
            set OUTPUT_DIR=%BIN_DIR%\Windows\%%P\MDD\!CONFIG_TYPE!
            if not exist "!OUTPUT_DIR!" (
                mkdir "!OUTPUT_DIR!"
            )
            
            REM Copy library files
            set LIB_NAME=CYDeviceD.lib
            set LIB_FOUND=0
            if exist "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            if !LIB_FOUND! EQU 0 if exist "!CONFIG_BUILD_DIR!\lib\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\lib\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            if !LIB_FOUND! EQU 0 if exist "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            
            if !LIB_FOUND! EQU 1 (
                echo   Success: Copied !LIB_NAME! to !OUTPUT_DIR!
                set /a SUCCESS_COUNT+=1
                echo   Build completed successfully!
            ) else (
                echo   WARNING: Library file not found: !LIB_NAME!
                set /a FAIL_COUNT+=1
            )
        )
    )
    
    :continue_mdd
    cd /d "%SCRIPT_DIR%"
    echo.
    
    REM Build Release configurations (MT and MD only)
    REM MT (MT Release)
    set /a BUILD_COUNT+=1
    echo [!BUILD_COUNT!] Building: Windows/%%P/MT/Release
    
    set RUNTIME_LIB=MultiThreaded
    set RUNTIME_FOR_CONFIG=MT
    set CONFIG_TYPE=Release
    set RUNTIME_TYPE=MT
    
    REM Check and build dependencies
    call :EnsureDependencies %%P !CONFIG_TYPE! !RUNTIME_TYPE! !OUTPUT_ARCH! !RUNTIME_FOR_CONFIG!
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: Failed to build dependencies for %%P/MT/!CONFIG_TYPE!
        set /a FAIL_COUNT+=1
        echo.
        cd /d "%SCRIPT_DIR%"
        goto :continue_mt
    )
    
    REM Create build directory for this configuration
    set CONFIG_BUILD_DIR=%BUILD_DIR%\%%P_MT_!CONFIG_TYPE!
    mkdir "!CONFIG_BUILD_DIR!" 2>nul
    
    REM Configure
    cd /d "!CONFIG_BUILD_DIR!"
    cmake -G "!CMAKE_GENERATOR!" ^
        -A "!CMAKE_PLATFORM!" ^
        -DCMAKE_BUILD_TYPE=!CONFIG_TYPE! ^
        -DCMAKE_MSVC_RUNTIME_LIBRARY=!RUNTIME_LIB! ^
        -DCMAKE_INSTALL_PREFIX="%BIN_DIR%\Windows\%%P\MT\!CONFIG_TYPE!" ^
        "%CMAKE_SOURCE_DIR%"
    
    set BUILD_SUCCESS=1
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: CMake configuration failed for %%P/MT/!CONFIG_TYPE!
        set /a FAIL_COUNT+=1
        set BUILD_SUCCESS=0
        cd /d "%SCRIPT_DIR%"
        goto :continue_mt
    ) else (
        REM Build
        cmake --build . --config !CONFIG_TYPE! --target CYDevice
        
        if !ERRORLEVEL! NEQ 0 (
            echo ERROR: Build failed for %%P/MT/!CONFIG_TYPE!
            set /a FAIL_COUNT+=1
            set BUILD_SUCCESS=0
            cd /d "%SCRIPT_DIR%"
            goto :continue_mt
        ) else (
            REM Create output directory structure
            set OUTPUT_DIR=%BIN_DIR%\Windows\%%P\MT\!CONFIG_TYPE!
            if not exist "!OUTPUT_DIR!" (
                mkdir "!OUTPUT_DIR!"
            )
            
            REM Copy library files
            set LIB_NAME=CYDevice.lib
            set LIB_FOUND=0
            if exist "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            if !LIB_FOUND! EQU 0 if exist "!CONFIG_BUILD_DIR!\lib\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\lib\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            if !LIB_FOUND! EQU 0 if exist "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            
            if !LIB_FOUND! EQU 1 (
                echo   Success: Copied !LIB_NAME! to !OUTPUT_DIR!
                set /a SUCCESS_COUNT+=1
                echo   Build completed successfully!
            ) else (
                echo   WARNING: Library file not found: !LIB_NAME!
                set /a FAIL_COUNT+=1
            )
        )
    )
    
    :continue_mt
    cd /d "%SCRIPT_DIR%"
    echo.
    
    REM MD (MD Release)
    set /a BUILD_COUNT+=1
    echo [!BUILD_COUNT!] Building: Windows/%%P/MD/Release
    
    set RUNTIME_LIB=MultiThreadedDLL
    set RUNTIME_FOR_CONFIG=MD
    set CONFIG_TYPE=Release
    set RUNTIME_TYPE=MD
    
    REM Check and build dependencies
    call :EnsureDependencies %%P !CONFIG_TYPE! !RUNTIME_TYPE! !OUTPUT_ARCH! !RUNTIME_FOR_CONFIG!
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: Failed to build dependencies for %%P/MD/!CONFIG_TYPE!
        set /a FAIL_COUNT+=1
        echo.
        cd /d "%SCRIPT_DIR%"
        goto :continue_md
    )
    
    REM Create build directory for this configuration
    set CONFIG_BUILD_DIR=%BUILD_DIR%\%%P_MD_!CONFIG_TYPE!
    mkdir "!CONFIG_BUILD_DIR!" 2>nul
    
    REM Configure
    cd /d "!CONFIG_BUILD_DIR!"
    cmake -G "!CMAKE_GENERATOR!" ^
        -A "!CMAKE_PLATFORM!" ^
        -DCMAKE_BUILD_TYPE=!CONFIG_TYPE! ^
        -DCMAKE_MSVC_RUNTIME_LIBRARY=!RUNTIME_LIB! ^
        -DCMAKE_INSTALL_PREFIX="%BIN_DIR%\Windows\%%P\MD\!CONFIG_TYPE!" ^
        "%CMAKE_SOURCE_DIR%"
    
    set BUILD_SUCCESS=1
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: CMake configuration failed for %%P/MD/!CONFIG_TYPE!
        set /a FAIL_COUNT+=1
        set BUILD_SUCCESS=0
        cd /d "%SCRIPT_DIR%"
        goto :continue_md
    ) else (
        REM Build
        cmake --build . --config !CONFIG_TYPE! --target CYDevice
        
        if !ERRORLEVEL! NEQ 0 (
            echo ERROR: Build failed for %%P/MD/!CONFIG_TYPE!
            set /a FAIL_COUNT+=1
            set BUILD_SUCCESS=0
            cd /d "%SCRIPT_DIR%"
            goto :continue_md
        ) else (
            REM Create output directory structure
            set OUTPUT_DIR=%BIN_DIR%\Windows\%%P\MD\!CONFIG_TYPE!
            if not exist "!OUTPUT_DIR!" (
                mkdir "!OUTPUT_DIR!"
            )
            
            REM Copy library files
            set LIB_NAME=CYDevice.lib
            set LIB_FOUND=0
            if exist "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\lib\!CONFIG_TYPE!\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            if !LIB_FOUND! EQU 0 if exist "!CONFIG_BUILD_DIR!\lib\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\lib\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            if !LIB_FOUND! EQU 0 if exist "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!LIB_NAME!" (
                copy /Y "!CONFIG_BUILD_DIR!\!CONFIG_TYPE!\!LIB_NAME!" "!OUTPUT_DIR!\" >nul
                set LIB_FOUND=1
            )
            
            if !LIB_FOUND! EQU 1 (
                echo   Success: Copied !LIB_NAME! to !OUTPUT_DIR!
                set /a SUCCESS_COUNT+=1
                echo   Build completed successfully!
            ) else (
                echo   WARNING: Library file not found: !LIB_NAME!
                set /a FAIL_COUNT+=1
            )
        )
    )
    
    :continue_md
    cd /d "%SCRIPT_DIR%"
    echo.
)

goto :end

REM Function to ensure dependencies are built
:EnsureDependencies
setlocal enabledelayedexpansion
echo [DEBUG] Line 219: Entering EnsureDependencies function
REM Re-establish variables from parent scope
echo [DEBUG] Line 221: Setting PROJECT_ROOT
set "PROJECT_ROOT=%PROJECT_ROOT%"
echo [DEBUG] Line 222: Setting BUILD_DIR
set "BUILD_DIR=%BUILD_DIR%"
echo [DEBUG] Line 223: Setting BIN_DIR
set "BIN_DIR=%BIN_DIR%"
echo [DEBUG] Line 224: Setting ARCH_PARAM
set "ARCH_PARAM=%~1"
echo [DEBUG] Line 225: Setting CONFIG_PARAM
set "CONFIG_PARAM=%~2"
echo [DEBUG] Line 226: Setting RUNTIME_PARAM
set "RUNTIME_PARAM=%~3"
echo [DEBUG] Line 227: Setting OUTPUT_ARCH_PARAM
set "OUTPUT_ARCH_PARAM=%~4"
echo [DEBUG] Line 228: Setting RUNTIME_FOR_CONFIG_PARAM
set "RUNTIME_FOR_CONFIG_PARAM=%~5"

echo [DEBUG] Line 230: Setting CMAKE_ARCH_PARAM
echo [DEBUG] Line 231: ARCH_PARAM=!ARCH_PARAM!
REM Set CMAKE_ARCH for CMake -A parameter
set "CMAKE_ARCH_PARAM=!ARCH_PARAM!"
echo [DEBUG] Line 232: Checking if ARCH_PARAM is x86
if /I "!ARCH_PARAM!"=="x86" (
    echo [DEBUG] Line 233: Setting CMAKE_ARCH_PARAM to Win32
    set "CMAKE_ARCH_PARAM=Win32"
) else (
    echo [DEBUG] Line 234: ARCH_PARAM is not x86, keeping CMAKE_ARCH_PARAM as !ARCH_PARAM!
)
echo [DEBUG] Line 235: CMAKE_ARCH_PARAM final=!CMAKE_ARCH_PARAM!

echo [DEBUG] Line 248: Building DEP_ARCH_DIR path
REM Build DEP_ARCH_DIR path - use for loop to avoid syntax errors
echo [DEBUG] Line 249: BIN_DIR=!BIN_DIR!
echo [DEBUG] Line 250: OUTPUT_ARCH_PARAM=!OUTPUT_ARCH_PARAM!
echo [DEBUG] Line 251: RUNTIME_FOR_CONFIG_PARAM=!RUNTIME_FOR_CONFIG_PARAM!
echo [DEBUG] Line 252: CONFIG_PARAM=!CONFIG_PARAM!
echo [DEBUG] Line 253: Starting for loop to build DEP_ARCH_DIR
for %%A in ("!BIN_DIR!") do (
    echo [DEBUG] Line 254: Loop A, value=%%~A
    for %%B in ("Windows") do (
        echo [DEBUG] Line 255: Loop B, value=%%~B
        for %%C in ("!OUTPUT_ARCH_PARAM!") do (
            echo [DEBUG] Line 256: Loop C, value=%%~C
            for %%D in ("!RUNTIME_FOR_CONFIG_PARAM!") do (
                echo [DEBUG] Line 257: Loop D, value=%%~D
                for %%E in ("!CONFIG_PARAM!") do (
                    echo [DEBUG] Line 258: Loop E, value=%%~E
                    echo [DEBUG] Line 259: Setting DEP_ARCH_DIR with values: %%~A, %%~B, %%~C, %%~D, %%~E
                    set "DEP_ARCH_DIR=%%~A\%%~B\%%~C\%%~D\%%~E"
                    echo [DEBUG] Line 260: DEP_ARCH_DIR set to=!DEP_ARCH_DIR!
                )
            )
        )
    )
)
echo [DEBUG] Line 261: DEP_ARCH_DIR final=!DEP_ARCH_DIR!

echo Checking dependencies for !OUTPUT_ARCH_PARAM! - !CONFIG_PARAM! - !RUNTIME_FOR_CONFIG_PARAM!

echo [DEBUG] Line 248: Checking CYCoroutine
REM Check CYCoroutine
echo [DEBUG] Line 249: Setting CYCO_LIB
set "CYCO_LIB=!DEP_ARCH_DIR!\CYCoroutine.lib"
echo [DEBUG] Line 250: CYCO_LIB=!CYCO_LIB!
set "CYCO_EXISTS=0"
echo [DEBUG] Line 251: Checking if CYCO_LIB exists
if exist "!CYCO_LIB!" (
    echo [DEBUG] Line 252: CYCO_LIB exists
    set "CYCO_EXISTS=1"
)
echo [DEBUG] Line 288: Checking CYCO_EXISTS value, CYCO_EXISTS=!CYCO_EXISTS!
echo [DEBUG] Line 289: About to check if CYCO_EXISTS equals 0
set "TEMP_CYCO_EXISTS=!CYCO_EXISTS!"
echo [DEBUG] Line 290: TEMP_CYCO_EXISTS=!TEMP_CYCO_EXISTS!
echo [DEBUG] Line 291: About to execute if statement
REM Use goto to avoid if statement syntax issues
if "!TEMP_CYCO_EXISTS!"=="1" goto :cyco_exists
echo [DEBUG] Line 292: CYCO_EXISTS is not 1, entering build section
echo CYCoroutine library not found, building it...
echo [DEBUG] Line 297: Entering CYCoroutine build section
echo [DEBUG] Line 298: Setting CYCO_DEP_DIR - step 1
set "CYCO_DEP_DIR=!BUILD_DIR!"
echo [DEBUG] Line 276: CYCO_DEP_DIR after step 1=!CYCO_DEP_DIR!
echo [DEBUG] Line 277: Setting CYCO_DEP_DIR - step 2
set "CYCO_DEP_DIR=!CYCO_DEP_DIR!\deps_cycoroutine"
echo [DEBUG] Line 278: CYCO_DEP_DIR after step 2=!CYCO_DEP_DIR!
echo [DEBUG] Line 279: Setting TEMP_VAR for OUTPUT_ARCH_PARAM
set "TEMP_VAR=!OUTPUT_ARCH_PARAM!"
echo [DEBUG] Line 280: TEMP_VAR=!TEMP_VAR!
echo [DEBUG] Line 281: Setting CYCO_DEP_DIR - step 3
set "CYCO_DEP_DIR=!CYCO_DEP_DIR!_!TEMP_VAR!"
echo [DEBUG] Line 282: CYCO_DEP_DIR after step 3=!CYCO_DEP_DIR!
echo [DEBUG] Line 283: Setting TEMP_VAR for RUNTIME_FOR_CONFIG_PARAM
set "TEMP_VAR=!RUNTIME_FOR_CONFIG_PARAM!"
echo [DEBUG] Line 284: TEMP_VAR=!TEMP_VAR!
echo [DEBUG] Line 285: Setting CYCO_DEP_DIR - step 4
set "CYCO_DEP_DIR=!CYCO_DEP_DIR!_!TEMP_VAR!"
echo [DEBUG] Line 286: CYCO_DEP_DIR after step 4=!CYCO_DEP_DIR!
echo [DEBUG] Line 287: Setting TEMP_VAR for CONFIG_PARAM
set "TEMP_VAR=!CONFIG_PARAM!"
echo [DEBUG] Line 288: TEMP_VAR=!TEMP_VAR!
echo [DEBUG] Line 289: Setting CYCO_DEP_DIR - step 5
set "CYCO_DEP_DIR=!CYCO_DEP_DIR!_!TEMP_VAR!"
echo [DEBUG] Line 290: CYCO_DEP_DIR final=!CYCO_DEP_DIR!
echo [DEBUG] Line 291: Creating directory
mkdir "!CYCO_DEP_DIR!" 2>nul
echo [DEBUG] Line 292: Directory created

REM Determine runtime library for CYCoroutine
if "!RUNTIME_PARAM!"=="MT" (
    if "!CONFIG_PARAM!"=="Debug" (
        set "CYCO_RUNTIME=MTD"
    ) else (
        set "CYCO_RUNTIME=MT"
    )
) else (
    if "!CONFIG_PARAM!"=="Debug" (
        set "CYCO_RUNTIME=MDD"
    ) else (
        set "CYCO_RUNTIME=MD"
    )
)

REM Build CYCoroutine
echo [DEBUG] Building CYCoroutine with:
echo [DEBUG]   ARCH_PARAM=!ARCH_PARAM!
echo [DEBUG]   CMAKE_ARCH_PARAM=!CMAKE_ARCH_PARAM!
echo [DEBUG]   CONFIG_PARAM=!CONFIG_PARAM!
echo [DEBUG]   CYCO_RUNTIME=!CYCO_RUNTIME!
echo [DEBUG]   CYCO_DEP_DIR=!CYCO_DEP_DIR!
REM Use temporary variable to avoid expansion issues
set "TEMP_ARCH=!ARCH_PARAM!"
if /I "!TEMP_ARCH!"=="x86" (
    set "TEMP_CMAKE_ARCH=Win32"
) else (
    set "TEMP_CMAKE_ARCH=x64"
)
echo [DEBUG]   TEMP_CMAKE_ARCH=!TEMP_CMAKE_ARCH!
cmake -S "!PROJECT_ROOT!\ThirdParty\CYCoroutine\Build" ^
      -B "!CYCO_DEP_DIR!" ^
      -G "Visual Studio 17 2022" ^
      -A "!TEMP_CMAKE_ARCH!" ^
      -DCMAKE_BUILD_TYPE="!CONFIG_PARAM!" ^
      -DBUILD_SHARED_LIBS=OFF ^
      -DBUILD_STATIC_LIBS=ON ^
      -DBUILD_EXAMPLES=OFF ^
      -DWINDOWS_RUNTIME="!CYCO_RUNTIME!"

if !ERRORLEVEL! NEQ 0 (
    echo ERROR: CYCoroutine CMake configuration failed
    endlocal & exit /b 1
)

cmake --build "!CYCO_DEP_DIR!" --config "!CONFIG_PARAM!" --target CYCoroutine_static --parallel %NUMBER_OF_PROCESSORS%
if !ERRORLEVEL! NEQ 0 (
    echo ERROR: CYCoroutine build failed
    endlocal & exit /b 1
)

REM Copy CYCoroutine library to output directory
if not exist "!DEP_ARCH_DIR!" (
    mkdir "!DEP_ARCH_DIR!"
)

REM Try to find CYCoroutine library in various possible locations
set "CYCO_SRC_LIB="
echo [DEBUG] Searching for CYCoroutine library
echo [DEBUG] OUTPUT_ARCH_PARAM=!OUTPUT_ARCH_PARAM!
echo [DEBUG] RUNTIME_FOR_CONFIG_PARAM=!RUNTIME_FOR_CONFIG_PARAM!
echo [DEBUG] CONFIG_PARAM=!CONFIG_PARAM!

REM Check CYCoroutine's output directory - try multiple path variations
REM 1. Standard path: x64/MD/Release
set "CYCO_OUTPUT_DIR=!PROJECT_ROOT!"
set "CYCO_OUTPUT_DIR=!CYCO_OUTPUT_DIR!\ThirdParty\CYCoroutine\Bin\Windows"
set "TEMP_VAR=!OUTPUT_ARCH_PARAM!"
set "CYCO_OUTPUT_DIR=!CYCO_OUTPUT_DIR!\!TEMP_VAR!"
set "TEMP_VAR=!RUNTIME_FOR_CONFIG_PARAM!"
set "CYCO_OUTPUT_DIR=!CYCO_OUTPUT_DIR!\!TEMP_VAR!"
set "TEMP_VAR=!CONFIG_PARAM!"
set "CYCO_OUTPUT_DIR=!CYCO_OUTPUT_DIR!\!TEMP_VAR!"
echo [DEBUG] Checking: !CYCO_OUTPUT_DIR!\CYCoroutine.lib
if exist "!CYCO_OUTPUT_DIR!\CYCoroutine.lib" (
    set "CYCO_SRC_LIB=!CYCO_OUTPUT_DIR!\CYCoroutine.lib"
    echo [DEBUG] Found in standard path
)

REM 2. Check with x86_64 instead of x64
if "!CYCO_SRC_LIB!"=="" (
    if "!OUTPUT_ARCH_PARAM!"=="x64" (
        set "CYCO_OUTPUT_DIR_X86_64=!PROJECT_ROOT!"
        set "CYCO_OUTPUT_DIR_X86_64=!CYCO_OUTPUT_DIR_X86_64!\ThirdParty\CYCoroutine\Bin\Windows"
        set "CYCO_OUTPUT_DIR_X86_64=!CYCO_OUTPUT_DIR_X86_64!\x86_64"
        set "TEMP_VAR=!RUNTIME_FOR_CONFIG_PARAM!"
        set "CYCO_OUTPUT_DIR_X86_64=!CYCO_OUTPUT_DIR_X86_64!\!TEMP_VAR!"
        set "TEMP_VAR=!CONFIG_PARAM!"
        set "CYCO_OUTPUT_DIR_X86_64=!CYCO_OUTPUT_DIR_X86_64!\!TEMP_VAR!"
        echo [DEBUG] Checking: !CYCO_OUTPUT_DIR_X86_64!\CYCoroutine.lib
        if exist "!CYCO_OUTPUT_DIR_X86_64!\CYCoroutine.lib" (
            set "CYCO_SRC_LIB=!CYCO_OUTPUT_DIR_X86_64!\CYCoroutine.lib"
            echo [DEBUG] Found in x86_64 path
        )
    )
)

REM 3. Check with uppercase CONFIG (RELEASE/DEBUG)
if "!CYCO_SRC_LIB!"=="" (
    set "CYCO_OUTPUT_DIR_UPPER=!PROJECT_ROOT!"
    set "CYCO_OUTPUT_DIR_UPPER=!CYCO_OUTPUT_DIR_UPPER!\ThirdParty\CYCoroutine\Bin\Windows"
    set "TEMP_VAR=!OUTPUT_ARCH_PARAM!"
    set "CYCO_OUTPUT_DIR_UPPER=!CYCO_OUTPUT_DIR_UPPER!\!TEMP_VAR!"
    set "TEMP_VAR=!RUNTIME_FOR_CONFIG_PARAM!"
    set "CYCO_OUTPUT_DIR_UPPER=!CYCO_OUTPUT_DIR_UPPER!\!TEMP_VAR!"
    if "!CONFIG_PARAM!"=="Debug" (
        echo [DEBUG] Checking: !CYCO_OUTPUT_DIR_UPPER!\DEBUG\CYCoroutine.lib
        if exist "!CYCO_OUTPUT_DIR_UPPER!\DEBUG\CYCoroutine.lib" (
            set "CYCO_SRC_LIB=!CYCO_OUTPUT_DIR_UPPER!\DEBUG\CYCoroutine.lib"
            echo [DEBUG] Found in DEBUG subdirectory
        )
    ) else (
        echo [DEBUG] Checking: !CYCO_OUTPUT_DIR_UPPER!\RELEASE\CYCoroutine.lib
        if exist "!CYCO_OUTPUT_DIR_UPPER!\RELEASE\CYCoroutine.lib" (
            set "CYCO_SRC_LIB=!CYCO_OUTPUT_DIR_UPPER!\RELEASE\CYCoroutine.lib"
            echo [DEBUG] Found in RELEASE subdirectory
        )
    )
)

REM 4. Check with x86_64 and uppercase CONFIG
if "!CYCO_SRC_LIB!"=="" (
    if "!OUTPUT_ARCH_PARAM!"=="x64" (
        set "CYCO_OUTPUT_DIR_X86_64_UPPER=!PROJECT_ROOT!"
        set "CYCO_OUTPUT_DIR_X86_64_UPPER=!CYCO_OUTPUT_DIR_X86_64_UPPER!\ThirdParty\CYCoroutine\Bin\Windows"
        set "CYCO_OUTPUT_DIR_X86_64_UPPER=!CYCO_OUTPUT_DIR_X86_64_UPPER!\x86_64"
        set "TEMP_VAR=!RUNTIME_FOR_CONFIG_PARAM!"
        set "CYCO_OUTPUT_DIR_X86_64_UPPER=!CYCO_OUTPUT_DIR_X86_64_UPPER!\!TEMP_VAR!"
        if "!CONFIG_PARAM!"=="Debug" (
            echo [DEBUG] Checking: !CYCO_OUTPUT_DIR_X86_64_UPPER!\DEBUG\CYCoroutine.lib
            if exist "!CYCO_OUTPUT_DIR_X86_64_UPPER!\DEBUG\CYCoroutine.lib" (
                set "CYCO_SRC_LIB=!CYCO_OUTPUT_DIR_X86_64_UPPER!\DEBUG\CYCoroutine.lib"
                echo [DEBUG] Found in x86_64\DEBUG path
            )
        ) else (
            echo [DEBUG] Checking: !CYCO_OUTPUT_DIR_X86_64_UPPER!\RELEASE\CYCoroutine.lib
            if exist "!CYCO_OUTPUT_DIR_X86_64_UPPER!\RELEASE\CYCoroutine.lib" (
                set "CYCO_SRC_LIB=!CYCO_OUTPUT_DIR_X86_64_UPPER!\RELEASE\CYCoroutine.lib"
                echo [DEBUG] Found in x86_64\RELEASE path
            )
        )
    )
)

REM Search in build directory
if "!CYCO_SRC_LIB!"=="" (
    for /r "!CYCO_DEP_DIR!" %%F in (CYCoroutine*.lib) do (
        set "CYCO_SRC_LIB=%%F"
        goto found_cyco
    )
)
:found_cyco

if not "!CYCO_SRC_LIB!"=="" (
    copy /Y "!CYCO_SRC_LIB!" "!CYCO_LIB!" >nul
    echo   CYCoroutine library copied to !CYCO_LIB!
) else (
    echo ERROR: CYCoroutine library not found after build
    echo   Searched in: !CYCO_OUTPUT_DIR!
    echo   Searched in: !CYCO_DEP_DIR!
    endlocal & exit /b 1
)
:cyco_exists
echo [DEBUG] Line 424: CYCoroutine check completed

REM Check libsamplerate
echo [DEBUG] Line 424: Checking libsamplerate
echo [DEBUG] Line 425: CONFIG_PARAM=!CONFIG_PARAM!
set "TEMP_CONFIG=!CONFIG_PARAM!"
if "!TEMP_CONFIG!"=="Debug" (
    echo [DEBUG] Line 426: Setting SAMPLERATE_LIB for Debug
    set "SAMPLERATE_LIB=!DEP_ARCH_DIR!\libsamplerated.lib"
) else (
    echo [DEBUG] Line 428: Setting SAMPLERATE_LIB for Release
    set "SAMPLERATE_LIB=!DEP_ARCH_DIR!\libsamplerate.lib"
)
echo [DEBUG] Line 435: SAMPLERATE_LIB=!SAMPLERATE_LIB!
echo [DEBUG] Line 436: Checking if SAMPLERATE_LIB exists
set "TEMP_SAMPLERATE_LIB=!SAMPLERATE_LIB!"
if exist "!TEMP_SAMPLERATE_LIB!" goto :samplerate_exists
echo [DEBUG] Line 437: SAMPLERATE_LIB not found, entering build section
echo libsamplerate library not found, building it...
set "SAMPLERATE_DEP_DIR=!BUILD_DIR!"
set "SAMPLERATE_DEP_DIR=!SAMPLERATE_DEP_DIR!\deps_libsamplerate"
set "TEMP_VAR=!OUTPUT_ARCH_PARAM!"
set "SAMPLERATE_DEP_DIR=!SAMPLERATE_DEP_DIR!_!TEMP_VAR!"
set "TEMP_VAR=!RUNTIME_FOR_CONFIG_PARAM!"
set "SAMPLERATE_DEP_DIR=!SAMPLERATE_DEP_DIR!_!TEMP_VAR!"
set "TEMP_VAR=!CONFIG_PARAM!"
set "SAMPLERATE_DEP_DIR=!SAMPLERATE_DEP_DIR!_!TEMP_VAR!"
mkdir "!SAMPLERATE_DEP_DIR!" 2>nul

REM Create a simple CMakeLists.txt for libsamplerate if it doesn't exist
if not exist "!PROJECT_ROOT!\ThirdParty\libsamplerate\CMakeLists.txt" (
    echo Creating CMakeLists.txt for libsamplerate...
    (
        echo cmake_minimum_required(VERSION 3.15^)
        echo project(libsamplerate VERSION 0.2.2 LANGUAGES C^)
        echo.
        echo add_library(libsamplerate STATIC
        echo     samplerate.c
        echo     src_linear.c
        echo     src_sinc.c
        echo     src_zoh.c
        echo ^)
        echo.
        echo target_include_directories(libsamplerate PUBLIC .^)
        echo.
        echo if^(CMAKE_BUILD_TYPE STREQUAL "Debug"^)
        echo     set_target_properties^(libsamplerate PROPERTIES DEBUG_POSTFIX "d"^)
        echo endif^(^)
        echo.
        echo if^(MSVC^)
        echo     set_property^(TARGET libsamplerate PROPERTY MSVC_RUNTIME_LIBRARY $${CMAKE_MSVC_RUNTIME_LIBRARY}^)
        echo endif^(^)
    ) > "!PROJECT_ROOT!\ThirdParty\libsamplerate\CMakeLists.txt"
)

REM Determine runtime library for libsamplerate
if "!RUNTIME_PARAM!"=="MT" (
    if "!CONFIG_PARAM!"=="Debug" (
        set "SAMPLERATE_RUNTIME=MultiThreadedDebug"
    ) else (
        set "SAMPLERATE_RUNTIME=MultiThreaded"
    )
) else (
    if "!CONFIG_PARAM!"=="Debug" (
        set "SAMPLERATE_RUNTIME=MultiThreadedDebugDLL"
    ) else (
        set "SAMPLERATE_RUNTIME=MultiThreadedDLL"
    )
)

REM Build libsamplerate
echo [DEBUG] Building libsamplerate with:
echo [DEBUG]   ARCH_PARAM=!ARCH_PARAM!
echo [DEBUG]   CMAKE_ARCH_PARAM=!CMAKE_ARCH_PARAM!
echo [DEBUG]   CONFIG_PARAM=!CONFIG_PARAM!
echo [DEBUG]   SAMPLERATE_RUNTIME=!SAMPLERATE_RUNTIME!
echo [DEBUG]   SAMPLERATE_DEP_DIR=!SAMPLERATE_DEP_DIR!
REM Use temporary variable to avoid expansion issues
set "TEMP_ARCH=!ARCH_PARAM!"
if /I "!TEMP_ARCH!"=="x86" (
    set "TEMP_CMAKE_ARCH=Win32"
) else (
    set "TEMP_CMAKE_ARCH=x64"
)
echo [DEBUG]   TEMP_CMAKE_ARCH=!TEMP_CMAKE_ARCH!
cmake -S "!PROJECT_ROOT!\ThirdParty\libsamplerate" ^
      -B "!SAMPLERATE_DEP_DIR!" ^
      -G "Visual Studio 17 2022" ^
      -A "!TEMP_CMAKE_ARCH!" ^
      -DCMAKE_BUILD_TYPE="!CONFIG_PARAM!" ^
      -DCMAKE_MSVC_RUNTIME_LIBRARY="!SAMPLERATE_RUNTIME!"

if !ERRORLEVEL! NEQ 0 (
    echo ERROR: libsamplerate CMake configuration failed
    endlocal & exit /b 1
)

cmake --build "!SAMPLERATE_DEP_DIR!" --config "!CONFIG_PARAM!" --target libsamplerate --parallel %NUMBER_OF_PROCESSORS%
if !ERRORLEVEL! NEQ 0 (
    echo ERROR: libsamplerate build failed
    endlocal & exit /b 1
)

REM Copy libsamplerate library to output directory
if not exist "!DEP_ARCH_DIR!" (
    mkdir "!DEP_ARCH_DIR!"
)
echo [DEBUG] Searching for libsamplerate library in: !SAMPLERATE_DEP_DIR!
set "SAMPLERATE_SRC_LIB="

REM Try multiple possible locations
REM 1. Check Debug or Release subdirectory
if "!CONFIG_PARAM!"=="Debug" (
    if exist "!SAMPLERATE_DEP_DIR!\Debug\libsamplerated.lib" (
        set "SAMPLERATE_SRC_LIB=!SAMPLERATE_DEP_DIR!\Debug\libsamplerated.lib"
        echo [DEBUG] Found in Debug subdirectory
    )
) else (
    if exist "!SAMPLERATE_DEP_DIR!\Release\libsamplerate.lib" (
        set "SAMPLERATE_SRC_LIB=!SAMPLERATE_DEP_DIR!\Release\libsamplerate.lib"
        echo [DEBUG] Found in Release subdirectory
    )
)

REM 2. Search recursively if not found
if "!SAMPLERATE_SRC_LIB!"=="" (
    echo [DEBUG] Searching recursively for libsamplerate*.lib
    for /r "!SAMPLERATE_DEP_DIR!" %%F in (libsamplerate*.lib) do (
        set "SAMPLERATE_SRC_LIB=%%F"
        echo [DEBUG] Found library: %%F
        goto found_samplerate
    )
)
:found_samplerate

if not "!SAMPLERATE_SRC_LIB!"=="" (
    echo [DEBUG] Copying from: !SAMPLERATE_SRC_LIB!
    echo [DEBUG] Copying to: !SAMPLERATE_LIB!
    copy /Y "!SAMPLERATE_SRC_LIB!" "!SAMPLERATE_LIB!" >nul
    if !ERRORLEVEL! EQU 0 (
        echo   libsamplerate library copied to !SAMPLERATE_LIB!
    ) else (
        echo ERROR: Failed to copy libsamplerate library
        echo   From: !SAMPLERATE_SRC_LIB!
        echo   To: !SAMPLERATE_LIB!
        endlocal & exit /b 1
    )
) else (
    echo ERROR: libsamplerate library not found after build
    echo   Searched in: !SAMPLERATE_DEP_DIR!\Debug\
    echo   Searched in: !SAMPLERATE_DEP_DIR!\Release\
    echo   Searched recursively in: !SAMPLERATE_DEP_DIR!
    endlocal & exit /b 1
)
:samplerate_exists
echo [DEBUG] Line 565: libsamplerate check completed

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
) else (
    echo Some builds failed. Please check the errors above.
)

echo.
pause

