@echo off
REM Chuyển sang thư mục chứa file .bat (tức là Shaders/)
cd /d "%~dp0"

echo ==============================
echo Compiling Vulkan shaders...
echo Current directory: %cd%
echo ==============================

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc mainShader.vert -o mainVert.spv
glslc mainShader.frag -o mainFrag.spv

if %errorlevel% neq 0 (
    echo ❌ Shader compilation failed!
    pause
    exit /b 1
)

echo ✅ Compilation completed successfully!
pause
