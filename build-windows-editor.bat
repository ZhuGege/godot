@echo off
chcp 65001 >nul 2>&1
setlocal

:: ============================================================
::  编译 Godot Windows Editor 并将产物复制到 DGame 项目目录
:: ============================================================

echo ============================================
echo   Compiling Godot (platform=windows) ...
echo ============================================
call scons platform=windows %*
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed! Copy step skipped.
    goto :end
)

echo.
echo ============================================
echo   Build successful. Copying to output ...
echo ============================================

:: 目标路径（相对于本 bat 所在目录的上级）
set "OUTPUT_DIR=..\DGame"

:: 确保输出目录存在
if not exist "%OUTPUT_DIR%" (
    echo [WARN] Output directory not found: %OUTPUT_DIR%
    goto :end
)

:: 复制 exe、dll 和 lib，跳过 .exp 和 obj 目录
set "COPIED=0"
for %%f in ("bin\*.exe" "bin\*.dll" "bin\*.lib") do (
    copy /y "%%f" "%OUTPUT_DIR%\" >nul 2>&1
    if not errorlevel 1 (
        set /a COPIED+=1
        echo   Copied: %%~nxf
    )
)

echo.
echo ============================================
echo   Done! %COPIED% file(s) copied to %OUTPUT_DIR%
echo ============================================

:end
pause
