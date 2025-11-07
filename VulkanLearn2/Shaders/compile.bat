@echo off
REM Chuyển sang thư mục chứa file .bat (tức là Shaders/)
cd /d "%~dp0"

echo =================================================
echo Compiling all .vert and .frag shaders in %cd%
echo =================================================

REM Xóa các file .spv cũ để tránh nhầm lẫn
del *.spv > nul 2>&1

REM Lặp qua tất cả các file .vert và .frag để biên dịch
for %%f in (*.vert, *.frag) do (
    echo Compiling %%f to %%f.spv...
    glslc "%%f" -o "%%f.spv"
)

if %errorlevel% neq 0 (
    echo ❌ Shader compilation failed!
    pause
    exit /b 1
)

echo ✅ All shaders compiled successfully!
pause